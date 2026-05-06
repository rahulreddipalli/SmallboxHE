#ifndef BOARD_H
#define BOARD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

#define BOARD_HALL_ADC1_SAMPLE_COUNT 8U
#define BOARD_HALL_ADC2_SAMPLE_COUNT 6U

void Board_Init(void);
const volatile uint16_t *Board_GetHallAdc1Samples(void);
const volatile uint16_t *Board_GetHallAdc2Samples(void);
void Board_HallAdc1DmaIrqHandler(void);
void Board_HallAdc2DmaIrqHandler(void);

#ifdef __cplusplus
}
#endif

#endif /* BOARD_H */
