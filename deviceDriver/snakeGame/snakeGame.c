#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/keyboard.h>
#include <linux/notifier.h>
#include <linux/input.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("To Vien");
MODULE_DESCRIPTION("Kernel module lắng nghe phím bấm");
MODULE_VERSION("1.0");

extern void SSD1306_Write(bool is_cmd, unsigned char data);

static int keyboard_notify(struct notifier_block *nblock, unsigned long code, void *_param)
{
    struct keyboard_notifier_param *param = _param;
    
    if (code == KBD_KEYCODE) {
        // Chỉ xử lý khi phím được nhấn xuống (down = 1)
        if (param->down) {
            printk(KERN_INFO "Key pressed: scancode = %d, keycode = %d\n", 
                   param->value, param->value);
            SSD1306_Write(false, 0x00);
            // In ra ký tự tương ứng nếu là phím chữ cái/số
            if (param->value >= 2 && param->value <= 11) {
                // Phím số 1-0
                char num = '0' + (param->value - 1);
                if (param->value == 11) num = '0'; // Phím 0
                printk(KERN_INFO "Number key: %c\n", num);
            }
            else if (param->value >= 16 && param->value <= 25) {
                // Phím chữ cái Q-P
                char letter = 'Q' + (param->value - 16);
                printk(KERN_INFO "Letter key: %c\n", letter);
            }
            else if (param->value >= 30 && param->value <= 38) {
                // Phím chữ cái A-L
                char letter = 'A' + (param->value - 30);
                printk(KERN_INFO "Letter key: %c\n", letter);
            }
            else if (param->value >= 44 && param->value <= 50) {
                // Phím chữ cái Z-M
                char letter = 'Z' + (param->value - 44);
                printk(KERN_INFO "Letter key: %c\n", letter);
            }
            else {
                // Các phím đặc biệt
                switch (param->value) {
                    case 1:   printk(KERN_INFO "Special key: ESC\n"); break;
                    case 14:  printk(KERN_INFO "Special key: BACKSPACE\n"); break;
                    case 15:  printk(KERN_INFO "Special key: TAB\n"); break;
                    case 28:  printk(KERN_INFO "Special key: ENTER\n"); break;
                    case 29:  printk(KERN_INFO "Special key: LEFT CTRL\n"); break;
                    case 42:  printk(KERN_INFO "Special key: LEFT SHIFT\n"); break;
                    case 54:  printk(KERN_INFO "Special key: RIGHT SHIFT\n"); break;
                    case 56:  printk(KERN_INFO "Special key: LEFT ALT\n"); break;
                    case 57:  printk(KERN_INFO "Special key: SPACE\n"); break;
                    default:  printk(KERN_INFO "Other key: scancode %d\n", param->value); break;
                }
            }
        }
    }
    
    return NOTIFY_OK;
}

static struct notifier_block keyboard_notifier_block = {
    .notifier_call = keyboard_notify,
};

static int __init keyboard_driver_init(void)
{
    int ret;
    
    printk(KERN_INFO "Keyboard driver: Loading module\n");
    
    // Đăng ký keyboard notifier
    ret = register_keyboard_notifier(&keyboard_notifier_block);
    if (ret) {
        printk(KERN_ERR "Keyboard driver: Failed to register keyboard notifier\n");
        return ret;
    }
    
    printk(KERN_INFO "Keyboard driver: Module loaded successfully\n");
    return 0;
}

static void __exit keyboard_driver_exit(void)
{
    printk(KERN_INFO "Keyboard driver: Unloading module\n");
    
    // Hủy đăng ký keyboard notifier
    unregister_keyboard_notifier(&keyboard_notifier_block);
    
    printk(KERN_INFO "Keyboard driver: Module unloaded successfully\n");
}

module_init(keyboard_driver_init);
module_exit(keyboard_driver_exit);
