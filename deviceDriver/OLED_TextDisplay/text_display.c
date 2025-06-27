/***************************************************************************/ /**
                                                                               *  \file       text_display.c
                                                                               *
                                                                               *  \details    Text display driver for SSD1306 OLED using I2C
                                                                               *
                                                                               *  \author     Bui Duc Khanh
                                                                               *
                                                                               * *******************************************************************************/
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <stdbool.h>
#include "font_chars.h"

// External function from I2C driver
extern void SSD1306_Write(bool is_cmd, unsigned char data);

/*
** This function sets the cursor position for text display
*/
static void SSD1306_SetCursor(unsigned char page, unsigned char column)
{
    // Set page address (0-7)
    SSD1306_Write(true, 0xB0 | (page & 0x07));

    // Set column lower address
    SSD1306_Write(true, 0x00 | (column & 0x0F));

    // Set column higher address
    SSD1306_Write(true, 0x10 | ((column >> 4) & 0x0F));
}

/*
** This function clears the OLED display
*/
static void SSD1306_Clear(void)
{
    unsigned char page, column;

    for (page = 0; page < 8; page++)
    {
        SSD1306_SetCursor(page, 0);

        for (column = 0; column < 128; column++)
        {
            SSD1306_Write(false, 0x00);
        }
    }
}

/*
** This function displays a character on the OLED
*/
static void SSD1306_DrawChar(char c)
{
    int font_index = char_to_index(c);
    int i;

    for (i = 0; i < 8; i++)
    {
        SSD1306_Write(false, font_8x8[font_index][i]);
    }
}

/*
** This function displays a string on the OLED
*/
static void SSD1306_DrawString(const char *str, unsigned char page, unsigned char column)
{
    int i = 0;

    SSD1306_SetCursor(page, column);

    while (str[i] != '\0')
    {
        SSD1306_DrawChar(str[i]);
        i++;
        column += 8; // Each character is 8 pixels wide

        // Check if we need to move to the next line
        if (column > 120)
        {
            column = 0;
            page++;
            if (page < 8) // Make sure we don't exceed the display
            {
                SSD1306_SetCursor(page, column);
            }
            else
            {
                break;
            }
        }
    }
}

/*
** This function displays "bui duc khanh" on the OLED
*/
static void display_text(void)
{
    // Clear the display first
    SSD1306_Clear();

    // Center the text on the display (128x64 pixels)
    // Each character is 8x8 pixels
    // "bui duc khanh" is 13 characters, so it's 13*8 = 104 pixels wide
    // To center horizontally: (128-104)/2 = 12
    // To center vertically: around page 3 (middle of 8 pages)
    SSD1306_DrawString("bui duc khanh", 3, 12);
}

/*
** Module Init function
*/
static int __init text_display_init(void)
{
    pr_info("OLED Text Display Module Loaded\n");
    display_text();
    return 0;
}

/*
** Module Exit function
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
MODULE_DESCRIPTION("OLED Text Display for SSD1306");
MODULE_VERSION("1.0");
