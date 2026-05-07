#include "app.h"

#include "board.h"
#include "hall_buttons.h"
#include "input_events.h"
#include "settings.h"
#include "ui.h"
#include "ui_render.h"

void App_Init(void)
{
  Settings_Init();
  Hall_Buttons_Init(Board_GetHallAdc1Samples(), Board_GetHallAdc2Samples());
  Ui_Init();
  InputEvents_Init();
  UiRender_Init();
}

void App_RunOnce(void)
{
  UiEvent event;

  Hall_Buttons_UpdateAll();
  InputEvents_Update();

  do
  {
    event = InputEvents_Pop();
    Ui_HandleEvent(event);
  } while (event != UI_EVT_NONE);

  Ui_Tick();
  Ui_Render();
}
