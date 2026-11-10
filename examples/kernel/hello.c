#include <linux/init.h>
#include <linux/module.h>
static int __init hfs_hello_init(void) { pr_info("hfs_hello: loaded\n"); return 0; }
static void __exit hfs_hello_exit(void) { pr_info("hfs_hello: unloaded\n"); }
module_init(hfs_hello_init);
module_exit(hfs_hello_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Hardware from Software first module");
