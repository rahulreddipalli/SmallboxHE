#include "app.h"

#include "board.h"
#include "hall_buttons.h"

void App_Init(void)
{
  Hall_Buttons_Init(Board_GetHallAdc1(), Board_GetHallAdc2());
}

void App_RunOnce(void)
{
  Hall_Buttons_UpdateAll();
  HAL_Delay(1);
}
