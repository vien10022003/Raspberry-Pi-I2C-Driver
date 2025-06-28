#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include "font_chars.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Vertical scrolling text display on OLED");
MODULE_VERSION("1.0");

// External function from the OLED driver
extern void SSD1306_Write(bool is_cmd, unsigned char data);

// Text to scroll
static char *scroll_text = "HELLO WORLD";
module_param(scroll_text, charp, 0644);
MODULE_PARM_DESC(scroll_text, "Text to scroll vertically");

// Workqueue for animation
static struct workqueue_struct *scroll_wq;
static struct delayed_work scroll_work;
static bool stop_scrolling = false;

// OLED display parameters
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define CHAR_WIDTH 8
#define CHAR_HEIGHT 8

static void oled_clear(void)
{
    printk(KERN_INFO "VerticalScroll: Clearing display\n");
    // Will implement actual clearing code in next phase
}

static int __init vertical_scroll_init(void)
{
    printk(KERN_INFO "VerticalScroll: Module loaded successfully\n");
    printk(KERN_INFO "VerticalScroll: Text to display: %s\n", scroll_text);

    // Initialize workqueue (for future animation)
    scroll_wq = create_singlethread_workqueue("scroll_workqueue");
    if (!scroll_wq)
    {
        printk(KERN_ERR "VerticalScroll: Failed to create workqueue\n");
        return -ENOMEM;
    }

    printk(KERN_INFO "VerticalScroll: Initialization complete\n");
    return 0;
}

static void __exit vertical_scroll_exit(void)
{
    // Set flag to stop scrolling
    stop_scrolling = true;

    // Cleanup workqueue
    if (scroll_wq)
    {
        cancel_delayed_work_sync(&scroll_work);
        destroy_workqueue(scroll_wq);
    }

    printk(KERN_INFO "VerticalScroll: Module unloaded successfully\n");
}

module_init(vertical_scroll_init);
module_exit(vertical_scroll_exit);