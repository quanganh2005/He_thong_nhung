#ifndef INC_MODBUS_SLAVE_H_
#define INC_MODBUS_SLAVE_H_

#include "main.h"

// ========================================================
// QUAN TRỌNG:
// - Nạp cho Board Slave 1 thì để: #define SLAVE_ID  1
// - Nạp cho Board Slave 2 thì sửa thành: #define SLAVE_ID  2
// ========================================================
#define SLAVE_ID  1

void Modbus_Init(void);
void RS485_SendResponse(uint8_t *data, uint16_t len);
void Modbus_ProcessIncomingData(uint8_t *rx_buf, uint16_t length);

#endif /* INC_MODBUS_SLAVE_H_ */
