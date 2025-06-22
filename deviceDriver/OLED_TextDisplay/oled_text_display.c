/***************************************************************************/ /**
                                                                               *  \file       oled_text_display.c
                                                                               *
                                                                               *  \details    OLED Text Display Driver with Line Navigation and Auto-scrolling
                                                                               *
                                                                               *  \author     Nhom 5
                                                                               *
                                                                               *  \Tested with Linux raspberrypi 5.4.51-v7l+
                                                                               *
                                                                               *******************************************************************************/
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/timer.h>
#include <linux/string.h>

#define I2C_BUS_AVAILABLE (1)          // I2C Bus available in our Raspberry Pi
#define SLAVE_DEVICE_NAME ("ETX_OLED") // Device and Driver Name
#define SSD1306_SLAVE_ADDR (0x3C)      // SSD1306 OLED Slave Address

#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define OLED_PAGES 8 // 64 pixels / 8 bits per page = 8 pages

#define CHAR_WIDTH 6
#define CHAR_HEIGHT 8
#define MAX_CHARS_PER_LINE 21 // 128 / 6 = 21.33, so we can fit 21 characters per line

#define MAX_LINES 6    // Number of lines we want to display
#define SCROLL_SPEED 5 // Delay between scrolling steps (in jiffies)

static struct i2c_adapter *etx_i2c_adapter = NULL;    // I2C Adapter Structure
static struct i2c_client *etx_i2c_client_oled = NULL; // I2C Client Structure (OLED)
static struct proc_dir_entry *oled_proc_entry;        // Proc file entry

// Font data - 5x7 font (first 128 ASCII characters)
extern unsigned char font5x7[];

// The text lines to display
static char *text_lines[MAX_LINES] = {
    "Driver - L01",
    "NHOM 5",
    "Thanh vien 1: Bui Duc Khanh CT060119",
    "Thanh vien 2: Bui Duc Khanh CT060119",
    "Thanh vien 3: Bui Duc Khanh CT060119",
    "Bui Duc Khanh CT060119"};

// Current state variables
static int current_line = 0;                  // Currently selected line (0 to MAX_LINES-1)
static int scroll_positions[MAX_LINES] = {0}; // Horizontal scroll position for each line
static struct timer_list scroll_timer;        // Timer for scrolling

// Buffer for the OLED display
static unsigned char display_buffer[OLED_WIDTH * OLED_PAGES];

/*
** This function writes the data into the I2C client
**
**  Arguments:
**      buff -> buffer to be sent
**      len  -> Length of the data
**
*/
static int I2C_Write(unsigned char *buf, unsigned int len)
{
    /*
    ** Sending Start condition, Slave address with R/W bit,
    ** ACK/NACK and Stop condtions will be handled internally.
    */
    int ret = i2c_master_send(etx_i2c_client_oled, buf, len);

    return ret;
}

/*
** This function is specific to the SSD_1306 OLED.
** This function sends the command/data to the OLED.
**
**  Arguments:
**      is_cmd -> true = command, false = data
**      data   -> data to be written
**
*/
static void SSD1306_Write(bool is_cmd, unsigned char data)
{
    unsigned char buf[2] = {0};
    int ret;

    /*
    ** First byte is always control byte. Data is followed after that.
    **
    ** There are two types of data in SSD_1306 OLED.
    ** 1. Command
    ** 2. Data
    **
    ** Control byte decides that the next byte is, command or data.
    **
    ** -------------------------------------------------------
    ** |              Control byte's | 6th bit  |   7th bit  |
    ** |-----------------------------|----------|------------|
    ** |   Command                   |   0      |     0      |
    ** |-----------------------------|----------|------------|
    ** |   data                      |   1      |     0      |
    ** |-----------------------------|----------|------------|
    **
    ** Please refer the datasheet for more information.
    **
    */
    if (is_cmd == true)
    {
        buf[0] = 0x00;
    }
    else
    {
        buf[0] = 0x40;
    }

    buf[1] = data;

    ret = I2C_Write(buf, 2);
}

/*
** This function sends multiple data bytes to the OLED
*/
static void SSD1306_WriteMultipleData(unsigned char *data, size_t size)
{
    unsigned char *buf = kmalloc(size + 1, GFP_KERNEL);

    if (buf)
    {
        buf[0] = 0x40; // Control byte for data
        memcpy(buf + 1, data, size);

        I2C_Write(buf, size + 1);
        kfree(buf);
    }
}

/*
** This function sends the commands that need to initialize the OLED.
*/
static int SSD1306_DisplayInit(void)
{
    msleep(100); // delay

    /*
    ** Commands to initialize the SSD_1306 OLED Display
    */
    SSD1306_Write(true, 0xAE); // Entire Display OFF
    SSD1306_Write(true, 0xD5); // Set Display Clock Divide Ratio and Oscillator Frequency
    SSD1306_Write(true, 0x80); // Default Setting for Display Clock Divide Ratio and Oscillator Frequency
    SSD1306_Write(true, 0xA8); // Set Multiplex Ratio
    SSD1306_Write(true, 0x3F); // 64 COM lines
    SSD1306_Write(true, 0xD3); // Set display offset
    SSD1306_Write(true, 0x00); // 0 offset
    SSD1306_Write(true, 0x40); // Set first line as the start line of the display
    SSD1306_Write(true, 0x8D); // Charge pump
    SSD1306_Write(true, 0x14); // Enable charge dump during display on
    SSD1306_Write(true, 0x20); // Set memory addressing mode
    SSD1306_Write(true, 0x00); // Horizontal addressing mode
    SSD1306_Write(true, 0xA1); // Set segment remap with column address 127 mapped to segment 0
    SSD1306_Write(true, 0xC8); // Set com output scan direction, scan from com63 to com 0
    SSD1306_Write(true, 0xDA); // Set com pins hardware configuration
    SSD1306_Write(true, 0x12); // Alternative com pin configuration, disable com left/right remap
    SSD1306_Write(true, 0x81); // Set contrast control
    SSD1306_Write(true, 0x80); // Set Contrast to 128
    SSD1306_Write(true, 0xD9); // Set pre-charge period
    SSD1306_Write(true, 0xF1); // Phase 1 period of 15 DCLK, Phase 2 period of 1 DCLK
    SSD1306_Write(true, 0xDB); // Set Vcomh deselect level
    SSD1306_Write(true, 0x20); // Vcomh deselect level ~ 0.77 Vcc
    SSD1306_Write(true, 0xA4); // Entire display ON, resume to RAM content display
    SSD1306_Write(true, 0xA6); // Set Display in Normal Mode, 1 = ON, 0 = OFF
    SSD1306_Write(true, 0x2E); // Deactivate scroll
    SSD1306_Write(true, 0xAF); // Display ON in normal mode

    return 0;
}

/*
** Clear the display buffer
*/
static void clear_display_buffer(void)
{
    memset(display_buffer, 0, sizeof(display_buffer));
}

/*
** Send the display buffer to the OLED
*/
static void update_display(void)
{
    // Set column address range from 0 to 127
    SSD1306_Write(true, 0x21);
    SSD1306_Write(true, 0x00);
    SSD1306_Write(true, 0x7F);

    // Set page address range from 0 to 7
    SSD1306_Write(true, 0x22);
    SSD1306_Write(true, 0x00);
    SSD1306_Write(true, 0x07);

    // Send the buffer data
    SSD1306_WriteMultipleData(display_buffer, sizeof(display_buffer));
}

/*
** Draw a character at the specified position
*/
static void draw_char(int x, int y, char c, bool is_selected)
{
    int i, j;
    unsigned char *glyph;

    // Check if character is out of visible area
    if (x < -CHAR_WIDTH || x >= OLED_WIDTH || y < 0 || y >= OLED_HEIGHT)
    {
        return;
    }

    // Get the character glyph from the font table
    glyph = &font5x7[(c - 32) * 5];

    // Draw the character
    for (i = 0; i < 5; i++)
    {
        if (x + i >= 0 && x + i < OLED_WIDTH)
        {
            for (j = 0; j < 8; j++)
            {
                if (glyph[i] & (1 << j))
                {
                    // Calculate the position in the buffer
                    int pos = x + i + ((y + j) / 8) * OLED_WIDTH;
                    if (pos >= 0 && pos < sizeof(display_buffer))
                    {
                        display_buffer[pos] |= 1 << ((y + j) % 8);
                    }
                }
            }
        }
    }

    // If this is the selected line and not scrolling, add dot markers
    if (is_selected && (strlen(text_lines[current_line]) <= MAX_CHARS_PER_LINE))
    {
        // Draw "..." at the end of the line
        if (x + 5 < OLED_WIDTH - 10)
        { // Make sure we have space for ...
            for (i = 0; i < 3; i++)
            {
                int dot_x = OLED_WIDTH - 10 + (i * 3);
                int dot_y = y + 4; // Middle of character height
                int pos = dot_x + (dot_y / 8) * OLED_WIDTH;
                if (pos >= 0 && pos < sizeof(display_buffer))
                {
                    display_buffer[pos] |= 1 << (dot_y % 8);
                }
            }
        }
    }
}

/*
** Draw a text string at the specified position
*/
static void draw_string(int x, int y, char *str, bool is_selected)
{
    int i = 0;

    while (str[i] != '\0')
    {
        draw_char(x + (i * CHAR_WIDTH), y, str[i], is_selected);
        i++;
    }
}

/*
** Update the display with the current state
*/
static void refresh_display(void)
{
    int i;
    int y_position;

    // Clear the display buffer
    clear_display_buffer();

    // Draw each line of text
    for (i = 0; i < MAX_LINES; i++)
    {
        y_position = i * CHAR_HEIGHT;

        if (i == current_line)
        {
            // This is the selected line, apply scroll position if needed
            draw_string(-scroll_positions[i], y_position, text_lines[i], true);
        }
        else
        {
            // Non-selected lines are always displayed from the beginning
            draw_string(0, y_position, text_lines[i], false);
        }
    }

    // Update the physical display
    update_display();
}

/*
** Timer function for scrolling text
*/
static void scroll_timer_function(struct timer_list *t)
{
    int text_length = strlen(text_lines[current_line]);
    int max_scroll = (text_length * CHAR_WIDTH) - OLED_WIDTH + 10; // +10 for padding

    // Only scroll if the text is longer than the display width
    if (text_length > MAX_CHARS_PER_LINE)
    {
        // Update scroll position
        scroll_positions[current_line] += 1;

        // Reset scroll position when reaching the end
        if (scroll_positions[current_line] > max_scroll)
        {
            scroll_positions[current_line] = 0;
        }

        // Refresh the display with new scroll position
        refresh_display();
    }

    // Restart the timer
    mod_timer(&scroll_timer, jiffies + SCROLL_SPEED);
}

/*
** Process file operation for proc interface
*/
static ssize_t oled_proc_write(struct file *file, const char __user *buf,
                               size_t count, loff_t *ppos)
{
    char command;

    if (count < 1)
        return -EINVAL;

    if (copy_from_user(&command, buf, 1))
        return -EFAULT;

    switch (command)
    {
    case 'u': // Move up
        if (current_line > 0)
        {
            current_line--;
            refresh_display();
        }
        break;

    case 'd': // Move down
        if (current_line < MAX_LINES - 1)
        {
            current_line++;
            refresh_display();
        }
        break;

    default:
        break;
    }

    return count;
}

// Proc file operations structure
static const struct proc_ops oled_proc_fops = {
    .proc_write = oled_proc_write,
};

/*
** This function getting called when the slave has been found
** Note : This will be called only once when we load the driver.
*/
static int etx_oled_probe(struct i2c_client *client,
                          const struct i2c_device_id *id)
{
    // Initialize the OLED display
    SSD1306_DisplayInit();

    // Initialize the scroll timer
    timer_setup(&scroll_timer, scroll_timer_function, 0);
    mod_timer(&scroll_timer, jiffies + SCROLL_SPEED);

    // Initial display update
    refresh_display();

    // Create proc interface for user commands
    oled_proc_entry = proc_create("oled_control", 0666, NULL, &oled_proc_fops);
    if (!oled_proc_entry)
    {
        pr_err("Failed to create proc entry\n");
        return -ENOMEM;
    }

    pr_info("OLED Text Display Driver Probed!!!\n");
    pr_info("Use 'echo u > /proc/oled_control' to move up\n");
    pr_info("Use 'echo d > /proc/oled_control' to move down\n");

    return 0;
}

/*
** This function getting called when the slave has been removed
** Note : This will be called only once when we unload the driver.
*/
static void etx_oled_remove(struct i2c_client *client)
{
    // Clear the display
    clear_display_buffer();
    update_display();

    // Delete the timer
    del_timer_sync(&scroll_timer);

    // Remove proc entry
    proc_remove(oled_proc_entry);

    pr_info("OLED Text Display Driver Removed!!!\n");
}

/*
** Structure that has slave device id
*/
static const struct i2c_device_id etx_oled_id[] = {
    {SLAVE_DEVICE_NAME, 0},
    {}};
MODULE_DEVICE_TABLE(i2c, etx_oled_id);

/*
** I2C driver Structure that has to be added to linux
*/
static struct i2c_driver etx_oled_driver = {
    .driver = {
        .name = SLAVE_DEVICE_NAME,
        .owner = THIS_MODULE,
    },
    .probe = etx_oled_probe,
    .remove = etx_oled_remove,
    .id_table = etx_oled_id,
};

/*
** I2C Board Info structure
*/
static struct i2c_board_info oled_i2c_board_info = {
    I2C_BOARD_INFO(SLAVE_DEVICE_NAME, SSD1306_SLAVE_ADDR)};

/*
** Module Init function
*/
static int __init etx_driver_init(void)
{
    int ret = -1;
    etx_i2c_adapter = i2c_get_adapter(I2C_BUS_AVAILABLE);

    if (etx_i2c_adapter != NULL)
    {
        etx_i2c_client_oled = i2c_new_client_device(etx_i2c_adapter, &oled_i2c_board_info);

        if (etx_i2c_client_oled != NULL)
        {
            i2c_add_driver(&etx_oled_driver);
            ret = 0;
        }

        i2c_put_adapter(etx_i2c_adapter);
    }

    pr_info("OLED Text Display Driver Added!!!\n");
    return ret;
}

/*
** Module Exit function
*/
static void __exit etx_driver_exit(void)
{
    i2c_unregister_device(etx_i2c_client_oled);
    i2c_del_driver(&etx_oled_driver);
    pr_info("OLED Text Display Driver Removed!!!\n");
}

module_init(etx_driver_init);
module_exit(etx_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nhom 5");
MODULE_DESCRIPTION("OLED Text Display Driver with Line Navigation and Auto-scrolling");
MODULE_VERSION("1.0");