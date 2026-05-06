#include "settings.h"

#include "settings_storage.h"

static SettingsState settings;

static const uint8_t default_profile_routes[HALL_BUTTON_COUNT] =
{
  [HALL_BUTTON_LEFT]     = HALL_BUTTON_LEFT,
  [HALL_BUTTON_DOWN]     = HALL_BUTTON_DOWN,
  [HALL_BUTTON_RIGHT]    = HALL_BUTTON_RIGHT,
  [HALL_BUTTON_UP]       = HALL_BUTTON_UP,
  [HALL_BUTTON_SQUARE]   = HALL_BUTTON_SQUARE,
  [HALL_BUTTON_TRIANGLE] = HALL_BUTTON_TRIANGLE,
  [HALL_BUTTON_L1]       = HALL_BUTTON_L1,
  [HALL_BUTTON_R1]       = HALL_BUTTON_R1,
  [HALL_BUTTON_CROSS]    = HALL_BUTTON_CROSS,
  [HALL_BUTTON_CIRCLE]   = HALL_BUTTON_CIRCLE,
  [HALL_BUTTON_L2]       = HALL_BUTTON_L2,
  [HALL_BUTTON_R2]       = HALL_BUTTON_R2,
  [HALL_BUTTON_L3]       = HALL_BUTTON_L3,
  [HALL_BUTTON_R3]       = HALL_BUTTON_R3
};

static uint16_t Settings_ClampActuationDistance(uint16_t distance);

static ControllerProfile *Settings_GetMutableActiveProfile(void)
{
  return &settings.profiles[settings.active_profile_index];
}

static void Settings_NormalizeState(void)
{
  uint8_t profile_index;
  uint8_t button_index;
  ControllerProfile *profile;

  if (settings.active_profile_index >= SETTINGS_PROFILE_COUNT)
  {
    settings.active_profile_index = 0U;
  }

  settings.calibration_mode_enabled = settings.calibration_mode_enabled ? 1U : 0U;

  for (profile_index = 0U; profile_index < SETTINGS_PROFILE_COUNT; profile_index++)
  {
    profile = &settings.profiles[profile_index];

    profile->rapid_trigger_enabled = profile->rapid_trigger_enabled ? 1U : 0U;
    profile->actuation_distance = Settings_ClampActuationDistance(profile->actuation_distance);

    for (button_index = 0U; button_index < HALL_BUTTON_COUNT; button_index++)
    {
      if (profile->output_for_button[button_index] >= HALL_BUTTON_COUNT &&
          profile->output_for_button[button_index] != SETTINGS_ROUTE_DISABLED)
      {
        profile->output_for_button[button_index] = default_profile_routes[button_index];
      }
    }
  }
}

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
  (void)Settings_Storage_Load(&settings);
  Settings_NormalizeState();
}

void Settings_ResetDefaults(void)
{
  uint8_t profile_index;

  settings.active_profile_index = 0U;
  settings.calibration_mode_enabled = 0U;

  for (profile_index = 0U; profile_index < SETTINGS_PROFILE_COUNT; profile_index++)
  {
    Settings_ResetProfileToDefault(profile_index);
  }
}

uint8_t Settings_Save(void)
{
  return Settings_Storage_Save(&settings);
}

const ControllerProfile *Settings_GetActiveProfile(void)
{
  return &settings.profiles[settings.active_profile_index];
}

uint8_t Settings_GetActiveProfileIndex(void)
{
  return settings.active_profile_index;
}

void Settings_SetActiveProfileIndex(uint8_t profile_index)
{
  if (profile_index < SETTINGS_PROFILE_COUNT)
  {
    settings.active_profile_index = profile_index;
  }
}

uint8_t Settings_GetRouteOutput(HallButtonId source_button)
{
  const ControllerProfile *profile = Settings_GetActiveProfile();

  if (source_button >= HALL_BUTTON_COUNT)
  {
    return SETTINGS_ROUTE_DISABLED;
  }

  return profile->output_for_button[source_button];
}

void Settings_SetRouteOutput(HallButtonId source_button, uint8_t output_button)
{
  ControllerProfile *profile = Settings_GetMutableActiveProfile();

  if (source_button >= HALL_BUTTON_COUNT)
  {
    return;
  }

  if (output_button >= HALL_BUTTON_COUNT && output_button != SETTINGS_ROUTE_DISABLED)
  {
    return;
  }

  profile->output_for_button[source_button] = output_button;
}

void Settings_ResetProfileToDefault(uint8_t profile_index)
{
  uint8_t button_index;
  ControllerProfile *profile;

  if (profile_index >= SETTINGS_PROFILE_COUNT)
  {
    return;
  }

  profile = &settings.profiles[profile_index];

  for (button_index = 0U; button_index < HALL_BUTTON_COUNT; button_index++)
  {
    profile->output_for_button[button_index] = default_profile_routes[button_index];
  }

  profile->rapid_trigger_enabled = 0U;
  profile->actuation_distance = SETTINGS_ACTUATION_DISTANCE_DEFAULT;
}

uint8_t Settings_IsRapidTriggerEnabled(void)
{
  return Settings_GetActiveProfile()->rapid_trigger_enabled;
}

void Settings_SetRapidTriggerEnabled(uint8_t enabled)
{
  Settings_GetMutableActiveProfile()->rapid_trigger_enabled = enabled ? 1U : 0U;
}

void Settings_ToggleRapidTrigger(void)
{
  Settings_SetRapidTriggerEnabled(Settings_IsRapidTriggerEnabled() == 0U);
}

uint16_t Settings_GetActuationDistance(void)
{
  return Settings_GetActiveProfile()->actuation_distance;
}

void Settings_SetActuationDistance(uint16_t distance)
{
  Settings_GetMutableActiveProfile()->actuation_distance = Settings_ClampActuationDistance(distance);
}

void Settings_AdjustActuationDistance(int16_t delta)
{
  int32_t next_distance = (int32_t)Settings_GetActuationDistance() + delta;

  if (next_distance < (int32_t)SETTINGS_ACTUATION_DISTANCE_MIN)
  {
    next_distance = (int32_t)SETTINGS_ACTUATION_DISTANCE_MIN;
  }
  else if (next_distance > (int32_t)SETTINGS_ACTUATION_DISTANCE_MAX)
  {
    next_distance = (int32_t)SETTINGS_ACTUATION_DISTANCE_MAX;
  }

  Settings_SetActuationDistance((uint16_t)next_distance);
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
