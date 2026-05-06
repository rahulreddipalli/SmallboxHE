#include "app.h"

#include "board.h"
#include "hall_buttons.h"
#include "settings.h"

void App_Init(void)
{
  Settings_Init();
  Hall_Buttons_Init(Board_GetHallAdc1Samples(), Board_GetHallAdc2Samples());
}

void App_RunOnce(void)
{
  Hall_Buttons_UpdateAll();
}
