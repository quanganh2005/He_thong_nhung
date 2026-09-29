#ifndef INC_RS485_PHY_H_
#define INC_RS485_PHY_H_

#include "main.h"

#define RS485_RX_BUFFER_SIZE 256

typedef void (*RS485_FrameRxCallback_t)(uint8_t *data, uint16_t length);

void RS485_Init(UART_HandleTypeDef *huart, TIM_HandleTypeDef *htim, 
                GPIO_TypeDef *dir_port, uint16_t dir_pin);
void RS485_RegisterCallback(RS485_FrameRxCallback_t callback);
void RS485_Send(uint8_t *data, uint16_t length);

void RS485_UART_RxByteCallback(void);
void RS485_UART_TxCpltCallback(void);
void RS485_Timer_PeriodElapsedCallback(void);

#endif