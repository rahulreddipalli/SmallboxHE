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
#define SETTINGS_ADC_MIN 0U
#define SETTINGS_ADC_MAX 4095U
#define SETTINGS_TRAVEL_MIN 0U
#define SETTINGS_TRAVEL_MAX 1000U
#define SETTINGS_RAPID_TRIGGER_DELTA_DEFAULT 50U

typedef struct
{
  uint16_t press_threshold;
  uint16_t release_threshold;
  uint16_t rapid_trigger_press_delta;
  uint16_t rapid_trigger_release_delta;
  uint16_t deadzone_low;
  uint16_t deadzone_high;
  uint8_t rapid_trigger_enabled;
  uint8_t invert_axis;
} ButtonSettings;

typedef struct
{
  uint16_t rest_adc;
  uint16_t pressed_adc;
  uint16_t min_seen_adc;
  uint16_t max_seen_adc;
  uint8_t calibrated;
} ButtonCalibration;

typedef struct
{
  uint8_t output_for_button[HALL_BUTTON_COUNT];
  ButtonSettings button_settings[HALL_BUTTON_COUNT];
  ButtonCalibration button_calibration[HALL_BUTTON_COUNT];
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
void Settings_CopyProfile(uint8_t source_profile_index, uint8_t destination_profile_index);

const ButtonSettings *Settings_GetButtonSettings(HallButtonId button);
void Settings_SetButtonSettings(HallButtonId button, const ButtonSettings *button_settings);
void Settings_ResetButtonSettings(HallButtonId button);
void Settings_SetButtonPressThreshold(HallButtonId button, uint16_t threshold);
void Settings_SetButtonReleaseThreshold(HallButtonId button, uint16_t threshold);
void Settings_SetButtonDeadzoneLow(HallButtonId button, uint16_t deadzone);
void Settings_SetButtonDeadzoneHigh(HallButtonId button, uint16_t deadzone);
void Settings_SetButtonInvertAxis(HallButtonId button, uint8_t invert_axis);

const ButtonCalibration *Settings_GetButtonCalibration(HallButtonId button);
void Settings_SetButtonCalibration(HallButtonId button, const ButtonCalibration *calibration);
void Settings_ResetButtonCalibration(HallButtonId button);

uint8_t Settings_IsRapidTriggerEnabled(void);
void Settings_SetRapidTriggerEnabled(uint8_t enabled);
void Settings_ToggleRapidTrigger(void);
uint8_t Settings_IsButtonRapidTriggerEnabled(HallButtonId button);
void Settings_SetButtonRapidTriggerEnabled(HallButtonId button, uint8_t enabled);
void Settings_ToggleButtonRapidTrigger(HallButtonId button);
void Settings_SetButtonRapidTriggerPressDelta(HallButtonId button, uint16_t delta);
void Settings_SetButtonRapidTriggerReleaseDelta(HallButtonId button, uint16_t delta);

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
