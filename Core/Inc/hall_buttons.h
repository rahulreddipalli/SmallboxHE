#ifndef HALL_BUTTONS_H
#define HALL_BUTTONS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

typedef enum
{
  HALL_BUTTON_LEFT = 0,
  HALL_BUTTON_DOWN,
  HALL_BUTTON_RIGHT,
  HALL_BUTTON_UP,
  HALL_BUTTON_SQUARE,
  HALL_BUTTON_TRIANGLE,
  HALL_BUTTON_L1,
  HALL_BUTTON_R1,
  HALL_BUTTON_CROSS,
  HALL_BUTTON_CIRCLE,
  HALL_BUTTON_L2,
  HALL_BUTTON_R2,
  HALL_BUTTON_L3,
  HALL_BUTTON_R3,
  HALL_BUTTON_COUNT
} HallButtonId;

void Hall_Buttons_Init(const volatile uint16_t *adc1_samples, const volatile uint16_t *adc2_samples);
void Hall_Buttons_UpdateAll(void);
void Hall_Buttons_ApplySettings(void);
uint32_t Hall_Buttons_GetAdcValue(HallButtonId button);
uint16_t Hall_Buttons_GetNormalizedTravel(HallButtonId button);
uint8_t Hall_Buttons_GetPressedState(HallButtonId button);
GPIO_PinState Hall_Buttons_GetGateState(HallButtonId button);

#ifdef __cplusplus
}
#endif

#endif /* HALL_BUTTONS_H */
