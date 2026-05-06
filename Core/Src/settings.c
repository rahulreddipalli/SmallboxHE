#include "settings.h"

static SettingsState settings;

static uint16_t Settings_ClampActuationDistance(uint16_t distance)
{
  if (distance < SETTINGS_ACTUATION_DISTANCE_MIN)
  {
    return SETTINGS_ACTUATION_DISTANCE_MIN;
  }

  if (distance > SETTINGS_ACTUATION_DISTANCE_MAX)
  {
    return SETTINGS_ACTUATION_DISTANCE_MAX;
  }

  return distance;
}

void Settings_Init(void)
{
  Settings_ResetDefaults();
}

void Settings_ResetDefaults(void)
{
  settings.rapid_trigger_enabled = 0U;
  settings.actuation_distance = SETTINGS_ACTUATION_DISTANCE_DEFAULT;
  settings.calibration_mode_enabled = 0U;
}

uint8_t Settings_IsRapidTriggerEnabled(void)
{
  return settings.rapid_trigger_enabled;
}

void Settings_SetRapidTriggerEnabled(uint8_t enabled)
{
  settings.rapid_trigger_enabled = enabled ? 1U : 0U;
}

void Settings_ToggleRapidTrigger(void)
{
  Settings_SetRapidTriggerEnabled(settings.rapid_trigger_enabled == 0U);
}

uint16_t Settings_GetActuationDistance(void)
{
  return settings.actuation_distance;
}

void Settings_SetActuationDistance(uint16_t distance)
{
  settings.actuation_distance = Settings_ClampActuationDistance(distance);
}

void Settings_AdjustActuationDistance(int16_t delta)
{
  int32_t next_distance = (int32_t)settings.actuation_distance + delta;

  if (next_distance < (int32_t)SETTINGS_ACTUATION_DISTANCE_MIN)
  {
    next_distance = (int32_t)SETTINGS_ACTUATION_DISTANCE_MIN;
  }
  else if (next_distance > (int32_t)SETTINGS_ACTUATION_DISTANCE_MAX)
  {
    next_distance = (int32_t)SETTINGS_ACTUATION_DISTANCE_MAX;
  }

  settings.actuation_distance = (uint16_t)next_distance;
}

uint8_t Settings_IsCalibrationModeEnabled(void)
{
  return settings.calibration_mode_enabled;
}

void Settings_SetCalibrationModeEnabled(uint8_t enabled)
{
  settings.calibration_mode_enabled = enabled ? 1U : 0U;
}

void Settings_ToggleCalibrationMode(void)
{
  Settings_SetCalibrationModeEnabled(settings.calibration_mode_enabled == 0U);
}

const SettingsState *Settings_GetState(void)
{
  return &settings;
}
