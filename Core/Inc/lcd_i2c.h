#ifndef LCD_I2C_H
#define LCD_I2C_H

#include "main.h"

/* PCF8574 I2C address shifted left by 1: 0x27<<1 or 0x3F<<1 */
#ifndef LCD_ADDR
#define LCD_ADDR  (0x27 << 1)
#endif

void LCD_Init(I2C_HandleTypeDef *hi2c);
void LCD_Clear(void);
void LCD_SetCursor(uint8_t row, uint8_t col);
void LCD_Print(const char *str);

#endif
