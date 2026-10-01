\# BẢN ĐẶC TẢ KỸ THUẬT HỆ THỐNG MẠNG RS485 MODBUS RTU (ĐỀ TÀI V04)



\## 1. Sơ đồ khối kiến trúc hệ thống

Hệ thống gồm 3 Node kết nối qua mạng bus RS485 vi sai 2 dây (A, B):

\- \*\*Node 1 (Master):\*\* Đóng vai trò điều khiển, giám sát, hiển thị LCD và gửi lệnh qua bus.

\- \*\*Node 2 (Slave 1):\*\* Thu thập dữ liệu cảm biến (Nhiệt độ/Độ ẩm DHT11 hoặc biến trở ADC).

\- \*\*Node 3 (Slave 2):\*\* Điều khiển cơ cấu chấp hành (Relay, LED trạng thái).



!\[Sơ đồ khối hệ thống](he\_thong\_block\_diagram.png)



\---



\## 2. Bảng phân bổ chân kết nối phần cứng (Pinout)



\### A. Node 1: Master (Điều khiển \& Giám sát)

| Chân STM32 | Ngoại vi kết nối | Mô tả chức năng |

| :--- | :--- | :--- |

| \*\*PA9\*\* | MAX485 - DI | USART1\_TX (Phát dữ liệu UART) |

| \*\*PA10\*\* | MAX485 - RO | USART1\_RX (Nhận dữ liệu UART) |

| \*\*PA8\*\* | MAX485 - DE / \~RE | GPIO Output (1: Phát, 0: Nhận) |

| \*\*PB6\*\* | LCD I2C - SCL | I2C1\_SCL |

| \*\*PB7\*\* | LCD I2C - SDA | I2C1\_SDA |

| \*\*PA0\*\* | Nút bấm 1 | Điều khiển bật/tắt thiết bị Slave 2 (EXTI / Input) |

| \*\*PA13 / PA14\*\* | Mạch nạp ST-Link | SWDIO / SWCLK (Nạp và Debug) |



\### B. Node 2: Slave 1 (Đo lường Cảm biến - ID: 0x01)

| Chân STM32 | Ngoại vi kết nối | Mô tả chức năng |

| :--- | :--- | :--- |

| \*\*PA9\*\* | MAX485 - DI | USART1\_TX |

| \*\*PA10\*\* | MAX485 - RO | USART1\_RX |

| \*\*PA8\*\* | MAX485 - DE / \~RE | GPIO Output (1: Phát, 0: Nhận) |

| \*\*PA1\*\* | Cảm biến DHT11 | Giao tiếp 1-Wire (Đọc nhiệt độ / độ ẩm) |

| \*\*PA0\*\* | Biến trở | ADC1\_IN0 (Đo điện áp tương tự) |

| \*\*PA13 / PA14\*\* | Mạch nạp ST-Link | SWDIO / SWCLK |



\### C. Node 3: Slave 2 (Chấp hành Relay - ID: 0x02)

| Chân STM32 | Ngoại vi kết nối | Mô tả chức năng |

| :--- | :--- | :--- |

| \*\*PA9\*\* | MAX485 - DI | USART1\_TX |

| \*\*PA10\*\* | MAX485 - RO | USART1\_RX |

| \*\*PA8\*\* | MAX485 - DE / \~RE | GPIO Output (1: Phát, 0: Nhận) |

| \*\*PB0\*\* | Module Relay | GPIO Output (Đóng / ngắt thiết bị) |

| \*\*PC13\*\* | LED tích hợp | Báo trạng thái hoạt động |

| \*\*PA13 / PA14\*\* | Mạch nạp ST-Link | SWDIO / SWCLK |



\---



\## 3. Bảng thanh ghi Modbus (Modbus Register Map)



\- Tốc độ truyền thông: \*\*9600 bps\*\*, Khung: \*\*8-N-1\*\* (8 Data bits, No Parity, 1 Stop bit).

\- Định dạng dữ liệu thanh ghi: 16-bit (Big-Endian).



\### A. Slave 1 (Địa chỉ Modbus: `0x01`)

| Địa chỉ thanh ghi (Hex) | Loại thanh ghi | Quyền | Mô tả dữ liệu | Đơn vị |

| :---: | :---: | :---: | :--- | :---: |

| `0x0000` | Holding / Input | Đọc (Function `0x03` / `0x04`) | Giá trị nhiệt độ DHT11 | 0.1 °C |

| `0x0001` | Holding / Input | Đọc (Function `0x03` / `0x04`) | Giá trị độ ẩm DHT11 | 0.1 %RH |

| `0x0002` | Holding / Input | Đọc (Function `0x03` / `0x04`) | Giá trị đọc ADC biến trở | 0 - 4095 |



\### B. Slave 2 (Địa chỉ Modbus: `0x02`)

| Địa chỉ thanh ghi (Hex) | Loại thanh ghi | Quyền | Mô tả dữ liệu | Giá trị quy ước |

| :---: | :---: | :---: | :--- | :---: |

| `0x0000` | Holding Register | Đọc / Ghi (`0x03` / `0x06`) | Trạng thái điều khiển Relay | `0`: Tắt, `1`: Bật |

| `0x0001` | Holding Register | Đọc / Ghi (`0x03` / `0x06`) | Ngưỡng nhiệt độ cảnh báo tự động | °C |



\---



\## 4. Đặc tả API Module Tầng vật lý (`rs485\_phy`)



Module `rs485\_phy` đảm bảo truyền nhận bất đồng bộ qua bus RS485 và phát hiện kết thúc khung bằng Timer khoảng lặng 3.5 ký tự (3.7 ms).



\### 1. `void RS485\_Init(UART\_HandleTypeDef \*huart, TIM\_HandleTypeDef \*htim, GPIO\_TypeDef \*dir\_port, uint16\_t dir\_pin)`

\- \*\*Mục đích:\*\* Gán con trỏ phần cứng UART, Timer và cấu hình chân DE/RE về mức LOW (lắng nghe).



\### 2. `void RS485\_RegisterCallback(RS485\_FrameRxCallback\_t callback)`

\- \*\*Mục đích:\*\* Đăng ký hàm callback bàn giao mảng dữ liệu nhận được lên tầng Modbus xử lý khi Timer phát hiện kết thúc khung.



\### 3. `void RS485\_Send(uint8\_t \*data, uint16\_t length)`

\- \*\*Mục đích:\*\* Kéo chân DE/RE lên HIGH và kích hoạt truyền khối byte qua ngắt `HAL\_UART\_Transmit\_IT`.

