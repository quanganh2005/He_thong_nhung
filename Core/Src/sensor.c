#include "sensor.h"
#include "modbus_slave.h"

extern ADC_HandleTypeDef hadc1; // Dùng cho Slave 2 đọc biến trở (chân PA0)

static uint16_t temperature = 250; // Nhiệt độ giả lập hoặc đọc từ DHT11 (nhân 10)
static uint16_t humidity = 600;    // Độ ẩm giả lập hoặc đọc từ DHT11 (nhân 10)
static uint8_t relay_status = 0;   // 0: Tắt, 1: Bật

void SENSOR_Init(void) {
    // Mặc định tắt Relay/LED khi khởi động (chân PB1 hoặc tùy chọn)
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET);
}

void SENSOR_ReadData(void) {
    if (SLAVE_ID == 1) {
        // === SLAVE 1: Đọc cảm biến nhiệt độ / độ ẩm (VD: DHT11 trên chân PA1) ===
        // Bạn có thể viết code đọc DHT11 ở đây và gán vào biến temperature, humidity
        temperature = 285; // Ví dụ 28.5 độ C
        humidity = 650;    // Ví dụ 65.0%
    }
    else if (SLAVE_ID == 2) {
        // === SLAVE 2: Đọc biến trở qua ADC (chân PA0) ===
        HAL_ADC_Start(&hadc1);
        if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK) {
            uint32_t adc_val = HAL_ADC_GetValue(&hadc1);
            temperature = (uint16_t)((adc_val * 330) / 4095); // Quy đổi giả lập
        }
        HAL_ADC_Stop(&hadc1);
    }
}

uint16_t SENSOR_GetTemperature(void) {
    return temperature;
}

uint16_t SENSOR_GetHumidity(void) {
    return humidity;
}

void RELAY_SetState(uint8_t state) {
    relay_status = state;
    if (state == 1) {
        // Bật Relay (Chân PB1 hoặc chân bạn cấu hình)
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_SET);
    } else {
        // Tắt Relay
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET);
    }
}

uint8_t RELAY_GetState(void) {
    return relay_status;
}
