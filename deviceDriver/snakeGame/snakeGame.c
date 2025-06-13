#include <linux/module.h>
#include <linux/init.h>
#include <linux/keyboard.h>
#include <linux/input.h>

extern void SSD1306_Write(bool is_cmd, unsigned char data);

static int my_keyboard_notifier(struct notifier_block *nb,
                                unsigned long action,
                                void *data)
{
    struct keyboard_notifier_param *param = data;

    if (action == KBD_KEYSYM && param->down) {
        printk(KERN_INFO "Key pressed: code=%u (char=%c)\n",
               param->value,
               (param->value >= 32 && param->value <= 126) ? param->value : '?');
        // Đây là nơi bạn có thể gọi hàm gửi lệnh qua I2C
        // SSD1306_SendChar((char)param->value); <-- nếu bạn export symbol
        SSD1306_Write(false, 0x00); // Gọi hàm SSD1306_Write để gửi ký tự
    }

    return NOTIFY_OK;
}

static struct notifier_block nb = {
    .notifier_call = my_keyboard_notifier
};

static int __init kb_init(void)
{
    printk(KERN_INFO "Keyboard notifier loaded\n");
    return register_keyboard_notifier(&nb);
}

static void __exit kb_exit(void)
{
    unregister_keyboard_notifier(&nb);
    printk(KERN_INFO "Keyboard notifier unloaded\n");
}

module_init(kb_init);
module_exit(kb_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("To Vien");
MODULE_DESCRIPTION("Kernel module lắng nghe phím bấm");
MODULE_VERSION("1.0");