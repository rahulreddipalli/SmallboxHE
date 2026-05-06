#ifndef SETTINGS_H
#define SETTINGS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

#define SETTINGS_ACTUATION_DISTANCE_MIN 1U
#define SETTINGS_ACTUATION_DISTANCE_MAX 4095U
#define SETTINGS_ACTUATION_DISTANCE_DEFAULT 400U

typedef struct
{
  uint8_t rapid_trigger_enabled;
  uint16_t actuation_distance;
  uint8_t calibration_mode_enabled;
} SettingsState;

void Settings_Init(void);
void Settings_ResetDefaults(void);

uint8_t Settings_IsRapidTriggerEnabled(void);
void Settings_SetRapidTriggerEnabled(uint8_t enabled);
void Settings_ToggleRapidTrigger(void);

uint16_t Settings_GetActuationDistance(void);
void Settings_SetActuationDistance(uint16_t distance);
void Settings_AdjustActuationDistance(int16_t delta);

uint8_t Settings_IsCalibrationModeEnabled(void);
void Settings_SetCalibrationModeEnabled(uint8_t enabled);
void Settings_ToggleCalibrationMode(void);

const SettingsState *Settings_GetState(void);

#ifdef __cplusplus
}
#endif

#endif /* SETTINGS_H */
