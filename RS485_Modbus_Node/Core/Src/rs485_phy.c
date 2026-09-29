#include "rs485_phy.h"

static UART_HandleTypeDef *g_huart;
static TIM_HandleTypeDef  *g_htim;
static GPIO_TypeDef       *g_dir_port;
static uint16_t            g_dir_pin;

static uint8_t  rx_byte;
static uint8_t  rx_buffer[RS485_RX_BUFFER_SIZE];
static uint16_t rx_index = 0;

static RS485_FrameRxCallback_t app_rx_callback = NULL;

void RS485_Init(UART_HandleTypeDef *huart, TIM_HandleTypeDef *htim, 
                GPIO_TypeDef *dir_port, uint16_t dir_pin) {
    g_huart = huart;
    g_htim = htim;
    g_dir_port = dir_port;
    g_dir_pin = dir_pin;

    HAL_GPIO_WritePin(g_dir_port, g_dir_pin, GPIO_PIN_RESET);
    HAL_UART_Receive_IT(g_huart, &rx_byte, 1);
}

void RS485_RegisterCallback(RS485_FrameRxCallback_t callback) {
    app_rx_callback = callback;
}

void RS485_Send(uint8_t *data, uint16_t length) {
    if (length == 0 || data == NULL) return;

    HAL_GPIO_WritePin(g_dir_port, g_dir_pin, GPIO_PIN_SET);
    HAL_UART_Transmit_IT(g_huart, data, length);
}

void RS485_UART_RxByteCallback(void) {
    if (rx_index < RS485_RX_BUFFER_SIZE) {
        rx_buffer[rx_index++] = rx_byte;
    }

    __HAL_TIM_SET_COUNTER(g_htim, 0);
    HAL_TIM_Base_Start_IT(g_htim);

    HAL_UART_Receive_IT(g_huart, &rx_byte, 1);
}

void RS485_UART_TxCpltCallback(void) {
    HAL_GPIO_WritePin(g_dir_port, g_dir_pin, GPIO_PIN_RESET);
}

void RS485_Timer_PeriodElapsedCallback(void) {
    HAL_TIM_Base_Stop_IT(g_htim);

    if (rx_index > 0 && app_rx_callback != NULL) {
        app_rx_callback(rx_buffer, rx_index);
    }

    rx_index = 0;
}