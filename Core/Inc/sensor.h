#ifndef INC_SENSOR_H_
#define INC_SENSOR_H_

#include "main.h"

void SENSOR_Init(void);
void SENSOR_ReadData(void);
uint16_t SENSOR_GetTemperature(void);
uint16_t SENSOR_GetHumidity(void);
void RELAY_SetState(uint8_t state);
uint8_t RELAY_GetState(void);

#endif /* INC_SENSOR_H_ */
