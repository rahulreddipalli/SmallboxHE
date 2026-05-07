#ifndef UI_H
#define UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "input_events.h"

typedef enum
{
  UI_SCREEN_DASHBOARD = 0,
  UI_SCREEN_MAIN_MENU,
  UI_SCREEN_BUTTON_SELECT,
  UI_SCREEN_BUTTON_SETUP,
  UI_SCREEN_RAPID_TRIGGER,
  UI_SCREEN_PROFILES,
  UI_SCREEN_CALIBRATION,
  UI_SCREEN_DISPLAY,
  UI_SCREEN_RAW_ADC,
  UI_SCREEN_SYSTEM,
  UI_SCREEN_CONFIRM_DIALOG
} UiScreen;

void Ui_Init(void);
void Ui_HandleEvent(UiEvent event);
void Ui_Tick(void);
void Ui_Render(void);
UiScreen Ui_GetScreen(void);

#ifdef __cplusplus
}
#endif

#endif /* UI_H */
