#ifndef INC_RS485_PHY_H_
#define INC_RS485_PHY_H_

#include "main.h"
/* ========================================================================== */
/*                           CAU HÌNH HE THONG                                */
/* ========================================================================== */
// Kích thuoc toi da cua bo dem nhan (Buffer Rx), Modbus RTU toi da 256 bytes

#define RS485_RX_BUFFER_SIZE 256
/* ========================================================================== */
/*                           Ð?NH NGHIA KIEU DU LIEU                          */
/* ========================================================================== */
/**
 * @brief Con tro hàm Callback báo nhan tron ven 1 khung tin Modbus RTU.
 * @param data: Con tro tro toi mang du lieu vua nhan duoc.
 * @param length: So luong byte thuc te cua khung tin.
 */
typedef void (*RS485_FrameRxCallback_t)(uint8_t *data, uint16_t length);
/* ========================================================================== */
/*                    CÁC HÀM GIAO TIEP TANG UNG DUNG (API)                   */
/* ========================================================================== */

/**
 * @brief  Khoi tao module RS485 tang vat lý.
 * @param  huart: Quan lý UART (ví du &huart1).
 * @param  htim: Quan lý Timer do timeout 3.5 ký tu (ví du &htim2).
 * @param  dir_port: Port GPIO dieu khien chân DE/RE (ví du GPIOA).
 * @param  dir_pin: Pin GPIO dieu khien chân DE/RE (ví du GPIO_PIN_4).
 */
void RS485_Init(UART_HandleTypeDef *huart, TIM_HandleTypeDef *htim, 
                GPIO_TypeDef *dir_port, uint16_t dir_pin);
/**
 * @brief  Ðang ký hàm xu lý khi nhan xong 1 khung tin hoàn chinh.
 * @param  callback: Tên hàm o tang trên muon nhan du lieu (ví du hàm Echo hoac Modbus Parser).
 */
void RS485_RegisterCallback(RS485_FrameRxCallback_t callback);
/**
 * @brief  Gui mot gói du lieu qua bus RS485 (Bán song công).
 * @param  data: Con tro chua mang du lieu can truyen di.
 * @param  length: So luong byte can truyen.
 */
void RS485_Send(uint8_t *data, uint16_t length);

/* ========================================================================== */
/*                CÁC HÀM XU LÝ NGAT NGOAI VI (GOI TU HAL CALLBACK)           */
/* ========================================================================== */

// Xu lý khi nhan duoc 1 byte tu UART
void RS485_UART_RxByteCallback(void);
// Xu lý khi UART truyen xong toàn bo khung du lieu
void RS485_UART_TxCpltCallback(void);
// Xu lý khi Timer tràn (phát hien khoang rong >= 3.5 ký tu, ket thúc khung)
void RS485_Timer_PeriodElapsedCallback(void);
// Xu lý khi UART gap loi phan cung (Overrun, Noise) de khôi phuc nhan
void RS485_UART_ErrorCallback(void);
#endif