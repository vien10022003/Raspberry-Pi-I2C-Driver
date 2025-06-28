#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/moduleparam.h> // Add this missing header
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

// Function to clear the OLED display
static void oled_clear(void)
{
    int i, j;
    printk(KERN_INFO "VerticalScroll: Clearing display\n");

    for (i = 0; i < 8; i++) // 8 pages
    {
        SSD1306_Write(true, 0xB0 + i); // Set page address
        SSD1306_Write(true, 0x00);     // Set lower column address
        SSD1306_Write(true, 0x10);     // Set higher column address

        for (j = 0; j < 128; j++) // 128 columns
        {
            SSD1306_Write(false, 0x00); // Clear each pixel
        }
    }
}

// Work queue handler function (stub for now)
static void scroll_work_handler(struct work_struct *work)
{
    // Will implement scrolling logic in next phase
    if (!stop_scrolling)
    {
        queue_delayed_work(scroll_wq, &scroll_work, msecs_to_jiffies(100));
    }
}

static int __init vertical_scroll_init(void)
{
    printk(KERN_INFO "VerticalScroll: Module loaded successfully\n");
    printk(KERN_INFO "VerticalScroll: Text to display: %s\n", scroll_text);

    // Clear the OLED display
    oled_clear();

    // Initialize workqueue for future animation
    scroll_wq = create_singlethread_workqueue("scroll_workqueue");
    if (!scroll_wq)
    {
        printk(KERN_ERR "VerticalScroll: Failed to create workqueue\n");
        return -ENOMEM;
    }

    // Initialize the delayed work
    INIT_DELAYED_WORK(&scroll_work, scroll_work_handler);

    printk(KERN_INFO "VerticalScroll: OLED I2C initialized\n");
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

    // Clear the display when unloading
    oled_clear();

    printk(KERN_INFO "VerticalScroll: Module unloaded successfully\n");
}

module_init(vertical_scroll_init);
module_exit(vertical_scroll_exit);