#ifndef SETTINGS_H
#define SETTINGS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "hall_buttons.h"

#define SETTINGS_ACTUATION_DISTANCE_MIN 1U
#define SETTINGS_ACTUATION_DISTANCE_MAX 4095U
#define SETTINGS_ACTUATION_DISTANCE_DEFAULT 400U
#define SETTINGS_PROFILE_COUNT 4U
#define SETTINGS_ROUTE_DISABLED 0xFFU

typedef struct
{
  uint8_t output_for_button[HALL_BUTTON_COUNT];
  uint8_t rapid_trigger_enabled;
  uint16_t actuation_distance;
} ControllerProfile;

typedef struct
{
  uint8_t active_profile_index;
  uint8_t calibration_mode_enabled;
  ControllerProfile profiles[SETTINGS_PROFILE_COUNT];
} SettingsState;

void Settings_Init(void);
void Settings_ResetDefaults(void);
uint8_t Settings_Save(void);

const ControllerProfile *Settings_GetActiveProfile(void);
uint8_t Settings_GetActiveProfileIndex(void);
void Settings_SetActiveProfileIndex(uint8_t profile_index);

uint8_t Settings_GetRouteOutput(HallButtonId source_button);
void Settings_SetRouteOutput(HallButtonId source_button, uint8_t output_button);
void Settings_ResetProfileToDefault(uint8_t profile_index);

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
