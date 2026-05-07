#ifndef UI_RENDER_H
#define UI_RENDER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

void UiRender_Init(void);
void UiRender_Clear(void);
void UiRender_Text(uint8_t x, uint8_t y, const char *text);
void UiRender_Bar(uint8_t x, uint8_t y, uint8_t width, uint16_t value, uint16_t max_value);
void UiRender_Flush(void);

#ifdef __cplusplus
}
#endif

#endif /* UI_RENDER_H */
