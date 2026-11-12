// Educational latest-state device, Linux 6.12+ API baseline, 64-bit Pi OS.
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/gpio/consumer.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/poll.h>
#include <linux/uaccess.h>
#include <linux/kref.h>
#include <linux/mutex.h>
#include <linux/slab.h>

struct hfs_button {
	struct kref ref;
	struct miscdevice misc;
	struct gpio_desc *button, *led;
	int irq;
	struct delayed_work debounce;
	spinlock_t lock;
	wait_queue_head_t wait;
	bool online, pressed, light;
	u64 sequence;
};
struct hfs_reader {
	struct hfs_button *device;
	struct mutex lock;
	u64 seen; // READ_ONCE/WRITE_ONCE; book targets 64-bit Arm.
};
static void hfs_free(struct kref *ref)
{
	kfree(container_of(ref, struct hfs_button, ref));
}
static bool hfs_ready(struct hfs_reader *reader)
{
	struct hfs_button *button_device = reader->device;
	unsigned long irq_flags;
	bool ready;
	spin_lock_irqsave(&button_device->lock, irq_flags);
	ready = !button_device->online || button_device->sequence != READ_ONCE(reader->seen);
	spin_unlock_irqrestore(&button_device->lock, irq_flags);
	return ready;
}
static void hfs_debounce(struct work_struct *work)
{
	struct hfs_button *button_device =
	    container_of(to_delayed_work(work), struct hfs_button, debounce);
	int pressed =
	    gpiod_get_value_cansleep(button_device->button); // logical, active-low translated
	unsigned long irq_flags;
	bool notify = false, light;
	if (pressed < 0) {
		return;
	}
	spin_lock_irqsave(&button_device->lock, irq_flags);
	if (button_device->online && button_device->pressed != !!pressed) {
		button_device->pressed = !!pressed;
		if (pressed) {
			++button_device->sequence;
			button_device->light = !button_device->light;
			notify = true;
		}
	}
	light = button_device->light;
	spin_unlock_irqrestore(&button_device->lock, irq_flags);
	if (notify) {
		gpiod_set_value_cansleep(button_device->led, light);
		wake_up_interruptible(&button_device->wait);
	}
}
static irqreturn_t hfs_irq(int irq, void *data)
{
	struct hfs_button *button_device = data;
	mod_delayed_work(system_wq, &button_device->debounce, msecs_to_jiffies(20));
	return IRQ_HANDLED;
}
static int hfs_open(struct inode *inode, struct file *file)
{
	struct miscdevice *misc = file->private_data;
	struct hfs_button *button_device = container_of(misc, struct hfs_button, misc);
	struct hfs_reader *reader;
	unsigned long irq_flags;
	reader = kzalloc(sizeof(*reader), GFP_KERNEL);
	if (!reader) {
		return -ENOMEM;
	}
	spin_lock_irqsave(&button_device->lock, irq_flags);
	if (!button_device->online) {
		spin_unlock_irqrestore(&button_device->lock, irq_flags);
		kfree(reader);
		return -ENODEV;
	}
	kref_get(&button_device->ref);
	reader->device = button_device;
	reader->seen = button_device->sequence; // New opens wait for a subsequent accepted press.
	spin_unlock_irqrestore(&button_device->lock, irq_flags);
	mutex_init(&reader->lock);
	file->private_data = reader;
	return nonseekable_open(inode, file);
}
static ssize_t hfs_read(struct file *file, char __user *buffer, size_t size, loff_t *offset)
{
	struct hfs_reader *reader = file->private_data;
	struct hfs_button *button_device = reader->device;
	unsigned long irq_flags;
	u64 sequence;
	bool light;
	char record[48];
	int record_length, result;
	if (!size) {
		return 0;
	}
	result = mutex_lock_interruptible(&reader->lock);
	if (result) {
		return result;
	}
	for (;;) {
		spin_lock_irqsave(&button_device->lock, irq_flags);
		if (!button_device->online) {
			spin_unlock_irqrestore(&button_device->lock, irq_flags);
			result = -ENODEV;
			goto out;
		}
		sequence = button_device->sequence;
		light = button_device->light;
		spin_unlock_irqrestore(&button_device->lock, irq_flags);
		if (sequence != READ_ONCE(reader->seen)) {
			break;
		}
		if (file->f_flags & O_NONBLOCK) {
			result = -EAGAIN;
			goto out;
		}
		result = wait_event_interruptible(button_device->wait, hfs_ready(reader));
		if (result) {
			goto out;
		}
	}
	record_length = scnprintf(record, sizeof(record), "%llu %u\n", sequence, light);
	if (size < record_length) {
		result = -EINVAL; // No event consumption; caller retries with a full buffer.
		goto out;
	}
	if (copy_to_user(buffer, record, record_length)) {
		result = -EFAULT;
		goto out;
	}
	WRITE_ONCE(reader->seen, sequence);
	result = record_length;
out:
	mutex_unlock(&reader->lock);
	return result;
}
static __poll_t hfs_poll(struct file *file, poll_table *table)
{
	struct hfs_reader *reader = file->private_data;
	struct hfs_button *button_device = reader->device;
	unsigned long irq_flags;
	__poll_t events = 0;
	poll_wait(file, &button_device->wait, table);
	spin_lock_irqsave(&button_device->lock, irq_flags);
	if (!button_device->online) {
		events = EPOLLHUP | EPOLLERR;
	} else if (button_device->sequence != READ_ONCE(reader->seen)) {
		events = EPOLLIN | EPOLLRDNORM;
	}
	spin_unlock_irqrestore(&button_device->lock, irq_flags);
	return events;
}
static int hfs_release(struct inode *inode, struct file *file)
{
	struct hfs_reader *reader = file->private_data;
	kref_put(&reader->device->ref, hfs_free);
	kfree(reader);
	return 0;
}
static const struct file_operations hfs_fops = {
    .owner = THIS_MODULE,
    .open = hfs_open,
    .read = hfs_read,
    .poll = hfs_poll,
    .release = hfs_release,
    .llseek = NULL,
};
static int hfs_probe(struct platform_device *pdev)
{
	struct hfs_button *button_device;
	int result;
	button_device = kzalloc(sizeof(*button_device), GFP_KERNEL);
	if (!button_device) {
		return -ENOMEM;
	}
	kref_init(&button_device->ref);
	spin_lock_init(&button_device->lock);
	init_waitqueue_head(&button_device->wait);
	INIT_DELAYED_WORK(&button_device->debounce, hfs_debounce);
	button_device->button = devm_gpiod_get(&pdev->dev, "button", GPIOD_IN);
	if (IS_ERR(button_device->button)) {
		result = PTR_ERR(button_device->button);
		goto free;
	}
	button_device->led = devm_gpiod_get(&pdev->dev, "led", GPIOD_OUT_LOW);
	if (IS_ERR(button_device->led)) {
		result = PTR_ERR(button_device->led);
		goto free;
	}
	result = gpiod_get_value_cansleep(button_device->button);
	if (result < 0) {
		goto free;
	}
	button_device->pressed = !!result;
	button_device->irq = gpiod_to_irq(button_device->button);
	if (button_device->irq < 0) {
		result = button_device->irq;
		goto free;
	}
	button_device->online = true;
	result =
	    devm_request_threaded_irq(&pdev->dev, button_device->irq, NULL, hfs_irq,
				      IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
				      "hfs-mybutton", button_device);
	if (result) {
		goto free;
	}
	button_device->misc.minor = MISC_DYNAMIC_MINOR;
	button_device->misc.name = "mybutton";
	button_device->misc.fops = &hfs_fops;
	button_device->misc.parent = &pdev->dev;
	result = misc_register(&button_device->misc);
	if (result) {
		devm_free_irq(&pdev->dev, button_device->irq, button_device);
		cancel_delayed_work_sync(&button_device->debounce);
		gpiod_set_value_cansleep(button_device->led, 0);
		goto free;
	}
	platform_set_drvdata(pdev, button_device);
	return 0;
free:
	kref_put(&button_device->ref, hfs_free);
	return result;
}
static void hfs_remove(struct platform_device *pdev)
{
	struct hfs_button *button_device = platform_get_drvdata(pdev);
	unsigned long irq_flags;
	spin_lock_irqsave(&button_device->lock, irq_flags);
	button_device->online = false;
	spin_unlock_irqrestore(&button_device->lock, irq_flags);
	wake_up_interruptible(&button_device->wait);
	// Stop future opens; misc core serialises open against deregistration.
	misc_deregister(&button_device->misc);
	devm_free_irq(&pdev->dev, button_device->irq,
		      button_device); // No more work can be queued after return.
	cancel_delayed_work_sync(&button_device->debounce);
	gpiod_set_value_cansleep(button_device->led, 0);
	// Open descriptors retain only the device's offline state, never its devm GPIOs.
	kref_put(&button_device->ref, hfs_free);
}
static const struct of_device_id hfs_match[] = {{.compatible = "hfs,mybutton"}, {}};
MODULE_DEVICE_TABLE(of, hfs_match);
static struct platform_driver hfs_driver = {
    .probe = hfs_probe,
    .remove = hfs_remove,
    .driver = {.name = "hfs-mybutton", .of_match_table = hfs_match},
};
module_platform_driver(hfs_driver);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Educational GPIO button latest-state device with safe unbind lifetime");
