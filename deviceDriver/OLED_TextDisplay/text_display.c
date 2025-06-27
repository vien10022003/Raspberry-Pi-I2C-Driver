#include <linux/init.h>
#include <linux/module.h>

MODULE_LICENSE("GPL"); // <-- Phải có để tránh lỗi khi build

static int __init text_display_init(void)
{
    printk(KERN_INFO "OLED Text Display init\n");
    return 0;
}

static void __exit text_display_exit(void)
{
    printk(KERN_INFO "OLED Text Display exit\n");
}

module_init(text_display_init);
module_exit(text_display_exit);
