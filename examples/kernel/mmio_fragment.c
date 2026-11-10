/* Contextual fragment for an existing platform driver's probe, not a module
 * to load on the Pi. The platform device MUST own the resource it describes.
 * GPIO on a running Pi already belongs to its controller driver. */
#include <linux/platform_device.h>
#include <linux/io.h>
static int example_probe(struct platform_device *pdev)
{
    void __iomem *registers = devm_platform_ioremap_resource(pdev, 0);
    u32 value;
    if (IS_ERR(registers))
        return PTR_ERR(registers);
    /* offset 0 is illustrative; an actual device datasheet must define this
     * as readable without side effects before this access is valid. */
    value = readl(registers + 0);
    dev_dbg(&pdev->dev, "documented register value: %08x\n", value);
    return 0;
}
