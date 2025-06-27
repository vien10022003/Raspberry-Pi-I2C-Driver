/***************************************************************************/ /**
                                                                               *  \file       text_display.c
                                                                               *
                                                                               *  \details    SSD1306 OLED Text Display Driver using I2C interface
                                                                                                                                        ERROR: modpost: "SSD1306_Write" [/home/pi/.../text_display.ko] undefined!                   *
                                                                               * *******************************************************************************/
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include "font_chars.h"

// External function from I2C driver
extern void SSD1306_Write(bool is_cmd, unsigned char data);
extern int SSD1306_Write(const char *buf, size_t count);

/*
** Set the cursor position on OLED
** page: 0-7 (8 pages, each 8 pixels height)
** column: 0-127 (128 pixels width)
*/
void SSD1306_SetCursor(unsigned char page, unsigned char column)
{
    // Set page address
    SSD1306_Write(true, 0xB0 | page);

    // Set column address (lower 4 bits)
    SSD1306_Write(true, 0x00 | (column & 0x0F));

    // Set column address (upper 4 bits)
    SSD1306_Write(true, 0x10 | ((column >> 4) & 0x0F));
}
EXPORT_SYMBOL(SSD1306_SetCursor);

/*
** Clear the entire OLED display
*/
void SSD1306_Clear(void)
{
    int i, j;

    for (i = 0; i < 8; i++)
    {
        SSD1306_SetCursor(i, 0);

        for (j = 0; j < 128; j++)
        {
            SSD1306_Write(false, 0x00);
        }
    }
}
EXPORT_SYMBOL(SSD1306_Clear);

/*
** Draw a single character on the OLED
*/
void SSD1306_DrawChar(char c)
{
    int font_index = char_to_index(c);
    int i;

    for (i = 0; i < 8; i++)
    {
        SSD1306_Write(false, font_8x8[font_index][i]);
    }
}
EXPORT_SYMBOL(SSD1306_DrawChar);

/*
** Draw a string on the OLED at the specified position
** If the string exceeds the display width, it will wrap to the next page
*/
void SSD1306_DrawString(const char *str, unsigned char page, unsigned char column)
{
    int i = 0;
    int current_column = column;
    int current_page = page;

    SSD1306_SetCursor(current_page, current_column);

    while (str[i] != '\0')
    {
        SSD1306_DrawChar(str[i]);
        current_column += 8; // Each character is 8 pixels wide

        // If we reach the end of the line, move to next page
        if (current_column > 120)
        {
            current_column = 0;
            current_page++;

            if (current_page > 7)
            {
                break; // Out of display bounds
            }

            SSD1306_SetCursor(current_page, current_column);
        }

        i++;
    }
}
EXPORT_SYMBOL(SSD1306_DrawString);

/*
** Display the text "bui duc khanh" on the OLED
*/
void SSD1306_DisplayBuiDucKhanh(void)
{
    SSD1306_Clear();
    SSD1306_DrawString("bui duc khanh", 3, 16); // Center on the display
}
EXPORT_SYMBOL(SSD1306_DisplayBuiDucKhanh);

/*
** Module initialization function
*/
static int __init text_display_init(void)
{
    pr_info("OLED Text Display Module Loaded\n");
    SSD1306_DisplayBuiDucKhanh();
    return 0;
}

/*
** Module exit function
*/
static void __exit text_display_exit(void)
{
    SSD1306_Clear();
    pr_info("OLED Text Display Module Unloaded\n");
}

module_init(text_display_init);
module_exit(text_display_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Bui Duc Khanh");
MODULE_DESCRIPTION("OLED Text Display Driver for SSD1306");
MODULE_VERSION("1.0");
