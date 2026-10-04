# Phần 4: Giao diện Master

## Chức năng
- LCD I2C 16x2 hiển thị dữ liệu của 2 Slave (mỗi Slave một dòng, cập nhật mỗi 300 ms).
- Slave mất kết nối: dòng tương ứng hiện `MAT KET NOI`.
- BTN1 / BTN2: đảo trạng thái relay của Slave 1 / Slave 2 (Master gửi hàm 06, ghi thanh ghi 2).

## Cấu hình CubeMX cho phần này
- I2C1: PB6 (SCL), PB7 (SDA), 100 kHz.
- PB0 nhãn `BTN1`, PB1 nhãn `BTN2`: GPIO Input, Pull-up (nút nối xuống GND).
- Địa chỉ LCD: sửa `LCD_ADDR` trong `lcd_i2c.h` (0x27<<1 hoặc 0x3F<<1).

## Gọi trong main.c
    #include "app_master.h"
    /* USER CODE BEGIN 2 */
    Master_Init(&huart1, &htim2, &hi2c1);
    /* USER CODE BEGIN 3 (trong while(1)) */
    Master_Task();

## Phụ thuộc (phần của các thành viên khác)
`app_master.c` dùng `modbus_rtu.h` (Modbus_BuildRead, Modbus_BuildWriteSingle,
Modbus_ParseReadResp, Modbus_ParseWriteResp) và `rs485.h` (RS485_Init, RS485_Send,
RS485_FrameReady, RS485_GetFrame). Nếu tên hàm trong project chung khác, chỉnh lại
cho khớp trong `app_master.c`.

## Cài đặt thử nghiệm (Modbus Poll / QModMaster)
Tháo Master khỏi bus, để PC làm Master qua mạch USB-RS485.
9600, 8 bit dữ liệu, không parity, 1 stop bit, chế độ RTU.
Đọc: Function 03, địa chỉ 0, số lượng 3. Ghi: Function 06, địa chỉ 2, giá trị 0/1.

## Bảng kiểm thử (điền kết quả và chụp ảnh)
| STT | Thao tác | Kết quả mong đợi | Kết quả thực tế | Đạt |
|---|---|---|---|---|
| 1 | Đọc FC03, ID 1, địa chỉ 0, số lượng 3 | ADC, phần trăm, relay; xoay biến trở thì giá trị đổi | | |
| 2 | Như trên với ID 2 | Tương tự | | |
| 3 | Ghi FC06 địa chỉ 2 = 1 rồi 0 | Relay đóng rồi nhả | | |
| 4 | Ghi địa chỉ 2 giá trị 5 | Lỗi ngoại lệ 03 | | |
| 5 | Đọc hoặc ghi địa chỉ 10 | Lỗi ngoại lệ 02 | | |
| 6 | Nối lại Master, quan sát LCD | Hai dòng hiện dữ liệu S1, S2 | | |
| 7 | Nhấn BTN1, rồi BTN2 | Relay tương ứng đổi, LCD đổi `R:` | | |
| 8 | Rút dây một Slave | Sau khoảng nửa giây hiện `MAT KET NOI` | | |
| 9 | Cắm lại Slave | Dữ liệu hiện lại, không cần reset | | |
