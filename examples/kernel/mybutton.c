// Educational latest-state device, Linux 6.12+ API baseline, 64-bit Pi OS.
#include <linux/module.h>
#include <linux/platform_device.h>
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
    struct hfs_button *b = reader->device;
    unsigned long flags;
    bool ready;
    spin_lock_irqsave(&b->lock, flags);
    ready = !b->online || b->sequence != READ_ONCE(reader->seen);
    spin_unlock_irqrestore(&b->lock, flags);
    return ready;
}
static void hfs_debounce(struct work_struct *work)
{
    struct hfs_button *b = container_of(to_delayed_work(work), struct hfs_button, debounce);
    int pressed = gpiod_get_value_cansleep(b->button); // logical, active-low translated
    unsigned long flags;
    bool notify = false, light;
    if (pressed < 0)
        return;
    spin_lock_irqsave(&b->lock, flags);
    if (b->online && b->pressed != !!pressed) {
        b->pressed = !!pressed;
        if (pressed) {
            ++b->sequence;
            b->light = !b->light;
            notify = true;
        }
    }
    light = b->light;
    spin_unlock_irqrestore(&b->lock, flags);
    if (notify) {
        gpiod_set_value_cansleep(b->led, light);
        wake_up_interruptible(&b->wait);
    }
}
static irqreturn_t hfs_irq(int irq, void *data)
{
    struct hfs_button *b = data;
    mod_delayed_work(system_wq, &b->debounce, msecs_to_jiffies(20));
    return IRQ_HANDLED;
}
static int hfs_open(struct inode *inode, struct file *file)
{
    struct miscdevice *misc = file->private_data;
    struct hfs_button *b = container_of(misc, struct hfs_button, misc);
    struct hfs_reader *reader;
    unsigned long flags;
    reader = kzalloc(sizeof(*reader), GFP_KERNEL);
    if (!reader)
        return -ENOMEM;
    spin_lock_irqsave(&b->lock, flags);
    if (!b->online) {
        spin_unlock_irqrestore(&b->lock, flags);
        kfree(reader);
        return -ENODEV;
    }
    kref_get(&b->ref);
    reader->device = b;
    reader->seen = b->sequence; // New opens wait for a subsequent accepted press.
    spin_unlock_irqrestore(&b->lock, flags);
    mutex_init(&reader->lock);
    file->private_data = reader;
    return nonseekable_open(inode, file);
}
static ssize_t hfs_read(struct file *file, char __user *buffer, size_t size, loff_t *offset)
{
    struct hfs_reader *reader = file->private_data;
    struct hfs_button *b = reader->device;
    unsigned long flags;
    u64 sequence;
    bool light;
    char record[48];
    int len, ret;
    if (!size)
        return 0;
    ret = mutex_lock_interruptible(&reader->lock);
    if (ret)
        return ret;
    for (;;) {
        spin_lock_irqsave(&b->lock, flags);
        if (!b->online) {
            spin_unlock_irqrestore(&b->lock, flags);
            ret = -ENODEV;
            goto out;
        }
        sequence = b->sequence;
        light = b->light;
        spin_unlock_irqrestore(&b->lock, flags);
        if (sequence != READ_ONCE(reader->seen))
            break;
        if (file->f_flags & O_NONBLOCK) {
            ret = -EAGAIN;
            goto out;
        }
        ret = wait_event_interruptible(b->wait, hfs_ready(reader));
        if (ret)
            goto out;
    }
    len = scnprintf(record, sizeof(record), "%llu %u\n", sequence, light);
    if (size < len) {
        ret = -EINVAL; // No event consumption; caller retries with a full buffer.
        goto out;
    }
    if (copy_to_user(buffer, record, len)) {
        ret = -EFAULT;
        goto out;
    }
    WRITE_ONCE(reader->seen, sequence);
    ret = len;
out:
    mutex_unlock(&reader->lock);
    return ret;
}
static __poll_t hfs_poll(struct file *file, poll_table *table)
{
    struct hfs_reader *reader = file->private_data;
    struct hfs_button *b = reader->device;
    unsigned long flags;
    __poll_t events = 0;
    poll_wait(file, &b->wait, table);
    spin_lock_irqsave(&b->lock, flags);
    if (!b->online)
        events = EPOLLHUP | EPOLLERR;
    else if (b->sequence != READ_ONCE(reader->seen))
        events = EPOLLIN | EPOLLRDNORM;
    spin_unlock_irqrestore(&b->lock, flags);
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
    .owner = THIS_MODULE, .open = hfs_open, .read = hfs_read,
    .poll = hfs_poll, .release = hfs_release, .llseek = no_llseek,
};
static int hfs_probe(struct platform_device *pdev)
{
    struct hfs_button *b;
    int ret;
    b = kzalloc(sizeof(*b), GFP_KERNEL);
    if (!b)
        return -ENOMEM;
    kref_init(&b->ref);
    spin_lock_init(&b->lock);
    init_waitqueue_head(&b->wait);
    INIT_DELAYED_WORK(&b->debounce, hfs_debounce);
    b->button = devm_gpiod_get(&pdev->dev, "button", GPIOD_IN);
    if (IS_ERR(b->button)) { ret = PTR_ERR(b->button); goto free; }
    b->led = devm_gpiod_get(&pdev->dev, "led", GPIOD_OUT_LOW);
    if (IS_ERR(b->led)) { ret = PTR_ERR(b->led); goto free; }
    ret = gpiod_get_value_cansleep(b->button);
    if (ret < 0)
        goto free;
    b->pressed = !!ret;
    b->irq = gpiod_to_irq(b->button);
    if (b->irq < 0) { ret = b->irq; goto free; }
    b->online = true;
    ret = devm_request_threaded_irq(&pdev->dev, b->irq, NULL, hfs_irq,
        IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
        "hfs-mybutton", b);
    if (ret)
        goto free;
    b->misc.minor = MISC_DYNAMIC_MINOR;
    b->misc.name = "mybutton";
    b->misc.fops = &hfs_fops;
    b->misc.parent = &pdev->dev;
    ret = misc_register(&b->misc);
    if (ret) {
        devm_free_irq(&pdev->dev, b->irq, b);
        cancel_delayed_work_sync(&b->debounce);
        goto free;
    }
    platform_set_drvdata(pdev, b);
    return 0;
free:
    kref_put(&b->ref, hfs_free);
    return ret;
}
static void hfs_remove(struct platform_device *pdev)
{
    struct hfs_button *b = platform_get_drvdata(pdev);
    unsigned long flags;
    spin_lock_irqsave(&b->lock, flags);
    b->online = false;
    spin_unlock_irqrestore(&b->lock, flags);
    wake_up_interruptible(&b->wait);
    // Stop future opens; misc core serialises open against deregistration.
    misc_deregister(&b->misc);
    devm_free_irq(&pdev->dev, b->irq, b); // No more work can be queued after return.
    cancel_delayed_work_sync(&b->debounce);
    gpiod_set_value_cansleep(b->led, 0);
    // Open descriptors retain only b's offline state, never its devm GPIOs.
    kref_put(&b->ref, hfs_free);
}
static const struct of_device_id hfs_match[] = { { .compatible = "hfs,mybutton" }, {} };
MODULE_DEVICE_TABLE(of, hfs_match);
static struct platform_driver hfs_driver = {
    .probe = hfs_probe, .remove = hfs_remove,
    .driver = { .name = "hfs-mybutton", .of_match_table = hfs_match },
};
module_platform_driver(hfs_driver);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Educational GPIO button latest-state device with safe unbind lifetime");
