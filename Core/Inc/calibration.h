#ifndef CALIBRATION_H
#define CALIBRATION_H

#ifdef __cplusplus
extern "C" {
#endif

#include "hall_buttons.h"

typedef enum
{
  CAL_IDLE = 0,
  CAL_WAIT_RELEASE,
  CAL_CAPTURE_REST,
  CAL_WAIT_PRESS_ANY_ORDER,
  CAL_CAPTURE_PRESSED,
  CAL_DONE
} CalibrationState;

void Calibration_Init(void);
void Calibration_Start(void);
void Calibration_Cancel(void);
void Calibration_Tick(void);
CalibrationState Calibration_GetState(void);
uint8_t Calibration_IsActive(void);
uint8_t Calibration_GetCalibratedCount(void);

#ifdef __cplusplus
}
#endif

#endif /* CALIBRATION_H */
