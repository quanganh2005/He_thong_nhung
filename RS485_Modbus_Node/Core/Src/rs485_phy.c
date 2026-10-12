#include "rs485_phy.h"

/* ========================================================================== */
/*                         BIEN NOI BO CUA MODULE                             */
/* ========================================================================== */
static UART_HandleTypeDef *g_huart;// Luu cau hình UART giao tiep MAX485
static TIM_HandleTypeDef  *g_htim;// Luu cau hình Timer tính timeout 3.5 ký tu
static GPIO_TypeDef       *g_dir_port;// Port dieu khien chân DE/RE
static uint16_t            g_dir_pin;// Pin dieu khien chân DE/RE

static uint8_t  rx_byte;// Bo dem chua 1 byte nhan tam thoi qua ngat
static uint8_t  rx_buffer[RS485_RX_BUFFER_SIZE];// Mang chua toàn bo khung du lieu Modbus
static uint16_t rx_index = 0;// Chi so byte hien tai trong mang rx_buffer

static RS485_FrameRxCallback_t app_rx_callback = NULL;// Con tro luu hàm callback cua tang trên

/* ========================================================================== */
/*                         TRIEN KHAI CÁC HÀM API                             */
/* ========================================================================== */

/**
 * @brief Khoi tao module: gán phan cung, dat che do nhan mac dinh và bat ngat UART.
 */
void RS485_Init(UART_HandleTypeDef *huart, TIM_HandleTypeDef *htim, 
                GPIO_TypeDef *dir_port, uint16_t dir_pin) {
    g_huart = huart;
    g_htim = htim;
    g_dir_port = dir_port;
    g_dir_pin = dir_pin;

// 1. Kéo chân DE/RE xuong muc LOW (MAX485 o che do NHAN du lieu)								
    HAL_GPIO_WritePin(g_dir_port, g_dir_pin, GPIO_PIN_RESET);
									
// 2. Kích hoat ngat nhan byte dau tiên									
    HAL_UART_Receive_IT(g_huart, &rx_byte, 1);
}

/**
 * @brief Ðang ký hàm goi nguoc (callback) khi có khung du lieu hoàn chinh.
 */
void RS485_RegisterCallback(RS485_FrameRxCallback_t callback) {
    app_rx_callback = callback;
}

/**
 * @brief Gui du lieu: bat chân DE/RE lên muc HIGH de phát, truyen du lieu qua ngat UART.
 */
void RS485_Send(uint8_t *data, uint16_t length) {
	// Kiem tra du lieu hop le truoc khi gui
    if (length == 0 || data == NULL) return;

// 1. Bat DE/RE lên muc HIGH (MAX485 o che do TRUYEN du lieu)	
    HAL_GPIO_WritePin(g_dir_port, g_dir_pin, GPIO_PIN_SET);
// 2. Kích hoat truyen du lieu bang ngat UART	
    HAL_UART_Transmit_IT(g_huart, data, length);
}

/* ========================================================================== */
/*                    TRIEN KHAI XU LÝ NGAT NGOAI VI                          */
/* ========================================================================== */

/**
 * @brief Xu lý khi nhan duoc tung byte du lieu tu UART.
 *        - Luu byte vào buffer.
 *        - Ðat lai Timer ve 0 và kích hoat lai Timer de canh chung timeout 3.5 ký tu.
 */
void RS485_UART_RxByteCallback(void) {
	// 1. Luu byte vua nhan vào mang neu chua day bo dem
    if (rx_index < RS485_RX_BUFFER_SIZE) {
        rx_buffer[rx_index++] = rx_byte;
    }

// 2. Reset Timer ve 0 de bat dau do lai khoang lang (idle) sau byte này
    __HAL_TIM_SET_COUNTER(g_htim, 0);
// 3. Xóa co ngat tràn cu tránh truong hop Timer nhay ngat som ngoài ý muon		
		__HAL_TIM_CLEAR_FLAG(g_htim, TIM_FLAG_UPDATE);
// 4. Bat ngat Timer dem thoi gian rong		
    HAL_TIM_Base_Start_IT(g_htim);
// 5. Mo lai ngat UART de dón byte tiep theo
    HAL_UART_Receive_IT(g_huart, &rx_byte, 1);
}

/**
 * @brief Xu lý khi UART truyen xong toàn bo khung:
 *        - Ðoi co TC (Transmission Complete) de dam bao bit STOP cuoi cùng dã ra khoi dây.
 *        - Kéo chân DE/RE ve muc LOW de chuyen ngay sang che do nhan.
 */
void RS485_UART_TxCpltCallback(void) {
	// Cho co TC bat (cho day 100% bit STOP cuoi cùng ra bus RS485)
	  while (__HAL_UART_GET_FLAG(g_huart, UART_FLAG_TC) == RESET)
			{
			}
// Ha chân DE/RE xuong LOW de chuyen chip MAX485 ve che do dón du lieu
    HAL_GPIO_WritePin(g_dir_port, g_dir_pin, GPIO_PIN_RESET);
}

/**
 * @brief Xu lý khi Timer tràn (Het thoi gian cho 3.5 ký tu):
 *        - Ðuong truyen dã rong >= 3.5 ký tu -> ket thúc 1 khung tin Modbus RTU.
 *        - Gui toàn bo khung nhan duoc lên tang xu lý ung dung.
 */
void RS485_Timer_PeriodElapsedCallback(void) {
	// 1. Dung Timer vì khung tin dã truyen xong
    HAL_TIM_Base_Stop_IT(g_htim);

	// 2. Neu có du lieu và dã dang ký callback, gui gói tin lên tang trên xu lý
    if (rx_index > 0 && app_rx_callback != NULL) {
        app_rx_callback(rx_buffer, rx_index);
    }

// 3. Reset lai chi so mang ve 0 de chuan bi nhan khung Modbus tiep theo		
    rx_index = 0;
}

/**
 * @brief Xu lý lai phan cung UART (Loi tràn ORE, loi nhieu NE):
 *        - Xóa co loi de giai phóng UART.
 *        - Bat lai ngat nhan tránh tình trang UART bi treo cung khi cam rút dây RS485.
 */
void RS485_UART_ErrorCallback(void) {
	// Xóa co loi tràn du lieu (Overrun Error)
    __HAL_UART_CLEAR_OREFLAG(g_huart);
	// Mo lai ngat nhan byte tiep theo
    HAL_UART_Receive_IT(g_huart, &rx_byte, 1);
}