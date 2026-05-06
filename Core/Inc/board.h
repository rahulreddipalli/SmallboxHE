#ifndef BOARD_H
#define BOARD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

void Board_Init(void);
ADC_HandleTypeDef *Board_GetHallAdc1(void);
ADC_HandleTypeDef *Board_GetHallAdc2(void);

#ifdef __cplusplus
}
#endif

#endif /* BOARD_H */
