#ifndef INPUT_EVENTS_H
#define INPUT_EVENTS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

typedef enum
{
  UI_EVT_NONE = 0,
  UI_EVT_ROTATE_CW,
  UI_EVT_ROTATE_CCW,
  UI_EVT_CLICK,
  UI_EVT_DOUBLE_CLICK,
  UI_EVT_LONG_PRESS,
  UI_EVT_BACK
} UiEvent;

void InputEvents_Init(void);
void InputEvents_Update(void);
uint8_t InputEvents_Push(UiEvent event);
UiEvent InputEvents_Pop(void);

#ifdef __cplusplus
}
#endif

#endif /* INPUT_EVENTS_H */
