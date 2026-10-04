/* Master application: polls 2 Slaves with Modbus RTU (FC03), writes relay
 * (FC06) on button press, shows data on I2C LCD 16x2.
 *
 * CubeMX user labels required:  RS485_DE (output), BTN1, BTN2 (inputs, pull-up). */

#include "app_master.h"
#include "lcd_i2c.h"
#include "modbus_rtu.h"
#include "rs485.h"
#include <stdio.h>
#include <string.h>

/* ---------------- configuration ---------------- */
#define SLAVE_COUNT        2
#define SLAVE1_ADDR        1
#define SLAVE2_ADDR        2

#define POLL_REG_COUNT     3      /* regs 0..2 : ADC, percent, relay */
#define REG_RELAY          2

#define RESP_TIMEOUT_MS    100    /* wait for a response */
#define GAP_MS             10     /* idle gap between two transactions (> 3.5 chars) */
#define ATTEMPTS_ONLINE    3      /* 1 try + 2 retries while slave is online */
#define ATTEMPTS_OFFLINE   1      /* probe an offline slave once per cycle */

#define LCD_PERIOD_MS      300
#define BTN_PERIOD_MS      20

/* ---------------- data ---------------- */
typedef struct {
    uint8_t  addr;
    uint16_t adc;        /* reg 0 */
    uint16_t percent;    /* reg 1 */
    uint8_t  relay;      /* reg 2 */
    uint8_t  online;
    uint8_t  failCount;  /* consecutive failed transactions */
} SlaveData_t;

typedef struct {
    uint8_t request;
    uint8_t value;
} PendingWrite_t;

typedef struct {
    GPIO_TypeDef *port;
    uint16_t      pin;
    uint8_t       last;
    uint8_t       event;
} Button_t;

static SlaveData_t    slave[SLAVE_COUNT];
static PendingWrite_t pend[SLAVE_COUNT];
static Button_t       btn[SLAVE_COUNT];

typedef enum { ST_IDLE, ST_WAIT } MasterState_t;
static MasterState_t st = ST_IDLE;
static uint8_t  cur = 0;          /* next slave to poll */
static uint8_t  target = 0;       /* slave of the transaction in flight */
static uint8_t  isWrite = 0;
static uint16_t writeVal = 0;
static uint8_t  failedTries = 0;
static uint32_t tSend = 0, tIdle = 0;

/* ---------------- transaction handling ---------------- */
static void SendCurrent(void)
{
    uint8_t  buf[16];
    uint16_t n;

    if (isWrite) n = Modbus_BuildWriteSingle(slave[target].addr, REG_RELAY, writeVal, buf);
    else         n = Modbus_BuildRead(slave[target].addr, 0, POLL_REG_COUNT, buf);

    RS485_Send(buf, n);
    tSend = HAL_GetTick();
}

static void StartTransaction(void)
{
    uint8_t i;

    isWrite = 0;
    target  = cur;
    for (i = 0; i < SLAVE_COUNT; i++) {          /* pending writes first */
        if (pend[i].request) {
            isWrite  = 1;
            target   = i;
            writeVal = pend[i].value;
            break;
        }
    }
    failedTries = 0;
    SendCurrent();
    st = ST_WAIT;
}

static void Finish(uint8_t ok)
{
    SlaveData_t *s = &slave[target];

    if (ok) { s->online = 1; s->failCount = 0; }
    else    { s->online = 0; if (s->failCount < 255) s->failCount++; }

    if (isWrite) pend[target].request = 0;
    else         cur = (uint8_t)((cur + 1) % SLAVE_COUNT);

    st    = ST_IDLE;
    tIdle = HAL_GetTick();
}

static void OnFailedTry(void)
{
    uint8_t maxTries = slave[target].online ? ATTEMPTS_ONLINE : ATTEMPTS_OFFLINE;

    failedTries++;
    if (failedTries >= maxTries) Finish(0);
    else                         SendCurrent();       /* retry */
}

static uint8_t ProcessResponse(const uint8_t *f, uint16_t n)
{
    SlaveData_t *s = &slave[target];
    uint16_t r[POLL_REG_COUNT];

    if (isWrite) {
        if (Modbus_ParseWriteResp(f, n, s->addr, REG_RELAY, writeVal) != MB_OK) return 0;
        s->relay = (uint8_t)writeVal;
        return 1;
    }
    if (Modbus_ParseReadResp(f, n, s->addr, POLL_REG_COUNT, r) != MB_OK) return 0;
    s->adc     = r[0];
    s->percent = r[1];
    s->relay   = (uint8_t)r[2];
    return 1;
}

/* ---------------- buttons ---------------- */
static void Buttons_Task(void)
{
    static uint32_t tick = 0;
    uint8_t i, now;

    if (HAL_GetTick() - tick < BTN_PERIOD_MS) return;
    tick = HAL_GetTick();

    for (i = 0; i < SLAVE_COUNT; i++) {
        now = (HAL_GPIO_ReadPin(btn[i].port, btn[i].pin) == GPIO_PIN_RESET);
        if (now && !btn[i].last) btn[i].event = 1;    /* falling edge = press */
        btn[i].last = now;

        if (btn[i].event) {
            btn[i].event = 0;
            if (slave[i].online && !pend[i].request) {
                pend[i].value   = slave[i].relay ? 0 : 1;
                pend[i].request = 1;
            }
        }
    }
}

/* ---------------- LCD ---------------- */
static void Lcd_Task(void)
{
    static uint32_t tick = 0;
    char tmp[24], line[20];
    uint8_t i;

    if (HAL_GetTick() - tick < LCD_PERIOD_MS) return;
    tick = HAL_GetTick();

    for (i = 0; i < SLAVE_COUNT; i++) {
        if (slave[i].online)
            snprintf(tmp, sizeof(tmp), "S%u ADC:%4u R:%u", (unsigned)(i + 1),
                     (unsigned)slave[i].adc, (unsigned)slave[i].relay);
        else
            snprintf(tmp, sizeof(tmp), "S%u MAT KET NOI", (unsigned)(i + 1));

        snprintf(line, sizeof(line), "%-16.16s", tmp);   /* pad to 16 chars */
        LCD_SetCursor(i, 0);
        LCD_Print(line);
    }
}

/* ---------------- public ---------------- */
void Master_Init(UART_HandleTypeDef *huart, TIM_HandleTypeDef *htim, I2C_HandleTypeDef *hi2c)
{
    memset(slave, 0, sizeof(slave));
    memset(pend,  0, sizeof(pend));
    slave[0].addr = SLAVE1_ADDR;
    slave[1].addr = SLAVE2_ADDR;

    btn[0].port = BTN1_GPIO_Port; btn[0].pin = BTN1_Pin;
    btn[1].port = BTN2_GPIO_Port; btn[1].pin = BTN2_Pin;

    LCD_Init(hi2c);
    LCD_SetCursor(0, 0); LCD_Print("RS485 Modbus RTU");
    LCD_SetCursor(1, 0); LCD_Print("Master ready    ");
    HAL_Delay(1000);

    RS485_Init(huart, htim, RS485_DE_GPIO_Port, RS485_DE_Pin);
    tIdle = HAL_GetTick();
}

void Master_Task(void)
{
    uint8_t  frame[RS485_BUF_SIZE];
    uint16_t n;

    Buttons_Task();

    if (st == ST_IDLE) {
        if (HAL_GetTick() - tIdle >= GAP_MS) StartTransaction();
    } else {                                   /* ST_WAIT */
        if (RS485_FrameReady()) {              /* check the frame BEFORE the timeout */
            n = RS485_GetFrame(frame);
            if (n > 0 && ProcessResponse(frame, n)) Finish(1);
            else                                    OnFailedTry();
        } else if (HAL_GetTick() - tSend >= RESP_TIMEOUT_MS) {
            OnFailedTry();
        }
    }

    Lcd_Task();
}
