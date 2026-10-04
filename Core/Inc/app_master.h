#ifndef APP_MASTER_H
#define APP_MASTER_H

#include "main.h"

/* Call once after MX_xxx_Init(). */
void Master_Init(UART_HandleTypeDef *huart, TIM_HandleTypeDef *htim, I2C_HandleTypeDef *hi2c);

/* Call as often as possible from the main loop (non-blocking, except LCD refresh). */
void Master_Task(void);

#endif
