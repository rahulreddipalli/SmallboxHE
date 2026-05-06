#ifndef HALL_BUTTONS_H
#define HALL_BUTTONS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

#define HALL_BUTTON_COUNT 14U

void Hall_Buttons_Init(ADC_HandleTypeDef *adc1, ADC_HandleTypeDef *adc2);
void Hall_Buttons_UpdateAll(void);

#ifdef __cplusplus
}
#endif

#endif /* HALL_BUTTONS_H */
