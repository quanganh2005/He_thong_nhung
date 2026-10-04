#include "lcd_i2c.h"

#define LCD_BL  0x08   /* backlight */
#define LCD_EN  0x04
#define LCD_RS  0x01

static I2C_HandleTypeDef *lcd_i2c;

static void LCD_WriteNibble(uint8_t nibble, uint8_t rs)
{
    uint8_t data = (uint8_t)((nibble & 0xF0) | LCD_BL | rs);
    uint8_t buf[2];

    buf[0] = (uint8_t)(data | LCD_EN);   /* EN = 1 */
    buf[1] = data;                       /* EN = 0 -> latch */
    HAL_I2C_Master_Transmit(lcd_i2c, LCD_ADDR, buf, 2, 10);
}

static void LCD_Send(uint8_t value, uint8_t rs)
{
    LCD_WriteNibble((uint8_t)(value & 0xF0), rs);
    LCD_WriteNibble((uint8_t)((value << 4) & 0xF0), rs);
}

static void LCD_Cmd(uint8_t cmd)  { LCD_Send(cmd, 0);      }
static void LCD_Data(uint8_t d)   { LCD_Send(d, LCD_RS);   }

void LCD_Init(I2C_HandleTypeDef *hi2c)
{
    lcd_i2c = hi2c;
    HAL_Delay(50);

    LCD_WriteNibble(0x30, 0); HAL_Delay(5);
    LCD_WriteNibble(0x30, 0); HAL_Delay(1);
    LCD_WriteNibble(0x30, 0); HAL_Delay(1);
    LCD_WriteNibble(0x20, 0); HAL_Delay(1);   /* switch to 4-bit mode */

    LCD_Cmd(0x28);   /* 4-bit, 2 lines, 5x8 font */
    LCD_Cmd(0x0C);   /* display on, cursor off */
    LCD_Cmd(0x06);   /* auto increment */
    LCD_Clear();
}

void LCD_Clear(void)
{
    LCD_Cmd(0x01);
    HAL_Delay(2);
}

void LCD_SetCursor(uint8_t row, uint8_t col)
{
    static const uint8_t base[4] = {0x00, 0x40, 0x14, 0x54};
    LCD_Cmd((uint8_t)(0x80 | (base[row & 3] + col)));
}

void LCD_Print(const char *str)
{
    while (*str) LCD_Data((uint8_t)*str++);
}
