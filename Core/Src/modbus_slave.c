#include "modbus_slave.h"
#include "sensor.h"

extern UART_HandleTypeDef huart1;

// Bảng thanh ghi Modbus
static uint16_t input_registers[2] = {0};   // Dành cho dữ liệu cảm biến (Chỉ đọc)
static uint16_t holding_registers[2] = {0}; // Dành cho trạng thái Relay (Đọc/Ghi)

// Hàm tính mã CRC16 chuẩn Modbus RTU
uint16_t Calculate_CRC16(uint8_t *buffer, uint16_t length) {
    uint16_t crc = 0xFFFF;
    for (uint16_t pos = 0; pos < length; pos++) {
        crc ^= (uint16_t)buffer[pos];
        for (int i = 8; i != 0; i--) {
            if ((crc & 0x0001) != 0) {
                crc >>= 1;
                crc ^= 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

void Modbus_Init(void) {
    input_registers[0] = 0;
    input_registers[1] = 0;
    holding_registers[0] = 0;
}

// Hàm gửi phản hồi qua RS485 có điều khiển chân DE/RE (chân PA8)
void RS485_SendResponse(uint8_t *data, uint16_t len) {
    // 1. Kéo DE/RE lên Mức Cao (Bật chế độ PHÁT)
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);

    // 2. Gửi dữ liệu qua UART1
    HAL_UART_Transmit(&huart1, data, len, 1000);

    // 3. Chờ truyền xong hoàn toàn (cờ TC)
    while(__HAL_UART_GET_FLAG(&huart1, UART_FLAG_TC) == RESET);

    // 4. Kéo DE/RE về Mức Thấp (Trả về chế độ NHẬN)
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);
}

// Xử lý khung truyền nhận từ Master gửi tới
void Modbus_ProcessIncomingData(uint8_t *rx_buf, uint16_t length) {
    if (length < 4) return;

    uint8_t address = rx_buf[0];
    uint8_t function_code = rx_buf[1];

    // Nếu không phải địa chỉ của Slave này thì bỏ qua
    if (address != SLAVE_ID) return;

    // Kiểm tra mã CRC16
    uint16_t received_crc = (rx_buf[length - 1] << 8) | rx_buf[length - 2];
    uint16_t calculated_crc = Calculate_CRC16(rx_buf, length - 2);
    if (received_crc != calculated_crc) return;

    uint8_t tx_buf[32];
    uint16_t tx_len = 0;

    // --- MÃ HÀM 03: Đọc thanh ghi ---
    if (function_code == 0x03) {
        uint16_t start_addr = (rx_buf[2] << 8) | rx_buf[3]-1;
        uint16_t num_regs = (rx_buf[4] << 8) | rx_buf[5];

        tx_buf[0] = SLAVE_ID;
        tx_buf[1] = 0x03;
        tx_buf[2] = num_regs * 2;

        uint8_t idx = 3;
        for (uint16_t i = 0; i < num_regs; i++) {
            uint16_t reg_val = 0;
            if (start_addr + i < 10) {
                input_registers[0] = SENSOR_GetTemperature();
                input_registers[1] = SENSOR_GetHumidity();
                reg_val = input_registers[start_addr + i];
            } else {
                holding_registers[0] = RELAY_GetState();
                reg_val = holding_registers[(start_addr + i) - 10];
            }

            tx_buf[idx++] = (reg_val >> 8) & 0xFF;
            tx_buf[idx++] = reg_val & 0xFF;
        }

        uint16_t crc = Calculate_CRC16(tx_buf, idx);
        tx_buf[idx++] = crc & 0xFF;
        tx_buf[idx++] = (crc >> 8) & 0xFF;
        tx_len = idx;

        RS485_SendResponse(tx_buf, tx_len);
    }
    // --- MÃ HÀM 06: Ghi 1 thanh ghi đơn (Điều khiển Relay) ---
    else if (function_code == 0x06) {
        uint16_t reg_addr = (rx_buf[2] << 8) | rx_buf[3];
        uint16_t reg_value = (rx_buf[4] << 8) | rx_buf[5];

        if (reg_addr >= 10) {
            holding_registers[reg_addr - 10] = reg_value;
            RELAY_SetState((uint8_t)reg_value);
        }

        for (int i = 0; i < 6; i++) {
            tx_buf[i] = rx_buf[i];
        }

        uint16_t crc = Calculate_CRC16(tx_buf, 6);
        tx_buf[6] = crc & 0xFF;
        tx_buf[7] = (crc >> 8) & 0xFF;
        tx_len = 8;

        RS485_SendResponse(tx_buf, tx_len);
    }
}
