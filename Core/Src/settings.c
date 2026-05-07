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
static uint16_t Settings_ClampAdcThreshold(int32_t threshold);
static uint16_t Settings_ClampAdcValue(uint16_t value);
static uint16_t Settings_ClampTravelValue(uint16_t value);

static ControllerProfile *Settings_GetMutableActiveProfile(void)
{
  return &settings.profiles[settings.active_profile_index];
}

static ButtonSettings *Settings_GetMutableButtonSettings(HallButtonId button)
{
  return &Settings_GetMutableActiveProfile()->button_settings[button];
}

static ButtonCalibration *Settings_GetMutableButtonCalibration(HallButtonId button)
{
  return &Settings_GetMutableActiveProfile()->button_calibration[button];
}

static void Settings_SetDefaultButtonSettings(ButtonSettings *button_settings)
{
  if (button_settings == NULL)
  {
    return;
  }

  button_settings->press_threshold = 2200U;
  button_settings->release_threshold = 1800U;
  button_settings->rapid_trigger_press_delta = SETTINGS_RAPID_TRIGGER_DELTA_DEFAULT;
  button_settings->rapid_trigger_release_delta = SETTINGS_RAPID_TRIGGER_DELTA_DEFAULT;
  button_settings->deadzone_low = SETTINGS_TRAVEL_MIN;
  button_settings->deadzone_high = SETTINGS_TRAVEL_MAX;
  button_settings->rapid_trigger_enabled = 0U;
  button_settings->invert_axis = 0U;
}

static void Settings_SetDefaultButtonCalibration(ButtonCalibration *calibration)
{
  if (calibration == NULL)
  {
    return;
  }

  calibration->rest_adc = SETTINGS_ADC_MIN;
  calibration->pressed_adc = SETTINGS_ADC_MAX;
  calibration->min_seen_adc = SETTINGS_ADC_MAX;
  calibration->max_seen_adc = SETTINGS_ADC_MIN;
  calibration->calibrated = 0U;
}

static void Settings_ApplyGlobalActuationDistance(ControllerProfile *profile)
{
  uint8_t button_index;
  uint32_t half_distance;
  int32_t press_threshold;
  int32_t release_threshold;

  if (profile == NULL)
  {
    return;
  }

  half_distance = (uint32_t)profile->actuation_distance / 2U;
  press_threshold = 2000 + (int32_t)half_distance;
  release_threshold = 2000 - (int32_t)half_distance;

  for (button_index = 0U; button_index < HALL_BUTTON_COUNT; button_index++)
  {
    profile->button_settings[button_index].press_threshold = Settings_ClampAdcThreshold(press_threshold);
    profile->button_settings[button_index].release_threshold = Settings_ClampAdcThreshold(release_threshold);
  }
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
      profile->button_settings[button_index].press_threshold = Settings_ClampAdcValue(profile->button_settings[button_index].press_threshold);
      profile->button_settings[button_index].release_threshold = Settings_ClampAdcValue(profile->button_settings[button_index].release_threshold);
      profile->button_settings[button_index].rapid_trigger_press_delta = Settings_ClampAdcValue(profile->button_settings[button_index].rapid_trigger_press_delta);
      profile->button_settings[button_index].rapid_trigger_release_delta = Settings_ClampAdcValue(profile->button_settings[button_index].rapid_trigger_release_delta);
      profile->button_settings[button_index].deadzone_low = Settings_ClampTravelValue(profile->button_settings[button_index].deadzone_low);
      profile->button_settings[button_index].deadzone_high = Settings_ClampTravelValue(profile->button_settings[button_index].deadzone_high);
      profile->button_settings[button_index].rapid_trigger_enabled = profile->button_settings[button_index].rapid_trigger_enabled ? 1U : 0U;
      profile->button_settings[button_index].invert_axis = profile->button_settings[button_index].invert_axis ? 1U : 0U;
      profile->button_calibration[button_index].rest_adc = Settings_ClampAdcValue(profile->button_calibration[button_index].rest_adc);
      profile->button_calibration[button_index].pressed_adc = Settings_ClampAdcValue(profile->button_calibration[button_index].pressed_adc);
      profile->button_calibration[button_index].min_seen_adc = Settings_ClampAdcValue(profile->button_calibration[button_index].min_seen_adc);
      profile->button_calibration[button_index].max_seen_adc = Settings_ClampAdcValue(profile->button_calibration[button_index].max_seen_adc);
      profile->button_calibration[button_index].calibrated = profile->button_calibration[button_index].calibrated ? 1U : 0U;

      if (profile->button_settings[button_index].release_threshold > profile->button_settings[button_index].press_threshold)
      {
        profile->button_settings[button_index].release_threshold = profile->button_settings[button_index].press_threshold;
      }

      if (profile->button_settings[button_index].deadzone_low > profile->button_settings[button_index].deadzone_high)
      {
        profile->button_settings[button_index].deadzone_low = profile->button_settings[button_index].deadzone_high;
      }

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

static uint16_t Settings_ClampAdcValue(uint16_t value)
{
  if (value > SETTINGS_ADC_MAX)
  {
    return SETTINGS_ADC_MAX;
  }

  return value;
}

static uint16_t Settings_ClampAdcThreshold(int32_t threshold)
{
  if (threshold < (int32_t)SETTINGS_ADC_MIN)
  {
    return SETTINGS_ADC_MIN;
  }

  if (threshold > (int32_t)SETTINGS_ADC_MAX)
  {
    return SETTINGS_ADC_MAX;
  }

  return (uint16_t)threshold;
}

static uint16_t Settings_ClampTravelValue(uint16_t value)
{
  if (value > SETTINGS_TRAVEL_MAX)
  {
    return SETTINGS_TRAVEL_MAX;
  }

  return value;
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
    Settings_SetDefaultButtonSettings(&profile->button_settings[button_index]);
    Settings_SetDefaultButtonCalibration(&profile->button_calibration[button_index]);
  }

  profile->rapid_trigger_enabled = 0U;
  profile->actuation_distance = SETTINGS_ACTUATION_DISTANCE_DEFAULT;
}

void Settings_CopyProfile(uint8_t source_profile_index, uint8_t destination_profile_index)
{
  if (source_profile_index >= SETTINGS_PROFILE_COUNT || destination_profile_index >= SETTINGS_PROFILE_COUNT)
  {
    return;
  }

  if (source_profile_index == destination_profile_index)
  {
    return;
  }

  settings.profiles[destination_profile_index] = settings.profiles[source_profile_index];
}

const ButtonSettings *Settings_GetButtonSettings(HallButtonId button)
{
  if (button >= HALL_BUTTON_COUNT)
  {
    return NULL;
  }

  return &Settings_GetActiveProfile()->button_settings[button];
}

void Settings_SetButtonSettings(HallButtonId button, const ButtonSettings *button_settings)
{
  if (button >= HALL_BUTTON_COUNT || button_settings == NULL)
  {
    return;
  }

  *Settings_GetMutableButtonSettings(button) = *button_settings;
  Settings_NormalizeState();
}

void Settings_ResetButtonSettings(HallButtonId button)
{
  if (button >= HALL_BUTTON_COUNT)
  {
    return;
  }

  Settings_SetDefaultButtonSettings(Settings_GetMutableButtonSettings(button));
}

void Settings_SetButtonPressThreshold(HallButtonId button, uint16_t threshold)
{
  ButtonSettings *button_settings;

  if (button >= HALL_BUTTON_COUNT)
  {
    return;
  }

  button_settings = Settings_GetMutableButtonSettings(button);
  button_settings->press_threshold = Settings_ClampAdcValue(threshold);

  if (button_settings->release_threshold > button_settings->press_threshold)
  {
    button_settings->release_threshold = button_settings->press_threshold;
  }
}

void Settings_SetButtonReleaseThreshold(HallButtonId button, uint16_t threshold)
{
  ButtonSettings *button_settings;

  if (button >= HALL_BUTTON_COUNT)
  {
    return;
  }

  button_settings = Settings_GetMutableButtonSettings(button);
  button_settings->release_threshold = Settings_ClampAdcValue(threshold);

  if (button_settings->release_threshold > button_settings->press_threshold)
  {
    button_settings->press_threshold = button_settings->release_threshold;
  }
}

void Settings_SetButtonDeadzoneLow(HallButtonId button, uint16_t deadzone)
{
  ButtonSettings *button_settings;

  if (button >= HALL_BUTTON_COUNT)
  {
    return;
  }

  button_settings = Settings_GetMutableButtonSettings(button);
  button_settings->deadzone_low = Settings_ClampTravelValue(deadzone);

  if (button_settings->deadzone_low > button_settings->deadzone_high)
  {
    button_settings->deadzone_high = button_settings->deadzone_low;
  }
}

void Settings_SetButtonDeadzoneHigh(HallButtonId button, uint16_t deadzone)
{
  ButtonSettings *button_settings;

  if (button >= HALL_BUTTON_COUNT)
  {
    return;
  }

  button_settings = Settings_GetMutableButtonSettings(button);
  button_settings->deadzone_high = Settings_ClampTravelValue(deadzone);

  if (button_settings->deadzone_high < button_settings->deadzone_low)
  {
    button_settings->deadzone_low = button_settings->deadzone_high;
  }
}

void Settings_SetButtonInvertAxis(HallButtonId button, uint8_t invert_axis)
{
  if (button >= HALL_BUTTON_COUNT)
  {
    return;
  }

  Settings_GetMutableButtonSettings(button)->invert_axis = invert_axis ? 1U : 0U;
}

const ButtonCalibration *Settings_GetButtonCalibration(HallButtonId button)
{
  if (button >= HALL_BUTTON_COUNT)
  {
    return NULL;
  }

  return &Settings_GetActiveProfile()->button_calibration[button];
}

void Settings_SetButtonCalibration(HallButtonId button, const ButtonCalibration *calibration)
{
  if (button >= HALL_BUTTON_COUNT || calibration == NULL)
  {
    return;
  }

  *Settings_GetMutableButtonCalibration(button) = *calibration;
  Settings_NormalizeState();
}

void Settings_ResetButtonCalibration(HallButtonId button)
{
  if (button >= HALL_BUTTON_COUNT)
  {
    return;
  }

  Settings_SetDefaultButtonCalibration(Settings_GetMutableButtonCalibration(button));
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

uint8_t Settings_IsButtonRapidTriggerEnabled(HallButtonId button)
{
  const ButtonSettings *button_settings = Settings_GetButtonSettings(button);

  if (button_settings == NULL)
  {
    return 0U;
  }

  return (Settings_IsRapidTriggerEnabled() && button_settings->rapid_trigger_enabled) ? 1U : 0U;
}

void Settings_SetButtonRapidTriggerEnabled(HallButtonId button, uint8_t enabled)
{
  if (button >= HALL_BUTTON_COUNT)
  {
    return;
  }

  Settings_GetMutableButtonSettings(button)->rapid_trigger_enabled = enabled ? 1U : 0U;

  if (enabled != 0U)
  {
    Settings_SetRapidTriggerEnabled(1U);
  }
}

void Settings_ToggleButtonRapidTrigger(HallButtonId button)
{
  if (button >= HALL_BUTTON_COUNT)
  {
    return;
  }

  Settings_SetButtonRapidTriggerEnabled(button, Settings_GetMutableButtonSettings(button)->rapid_trigger_enabled == 0U);
}

void Settings_SetButtonRapidTriggerPressDelta(HallButtonId button, uint16_t delta)
{
  if (button >= HALL_BUTTON_COUNT)
  {
    return;
  }

  Settings_GetMutableButtonSettings(button)->rapid_trigger_press_delta = Settings_ClampAdcValue(delta);
}

void Settings_SetButtonRapidTriggerReleaseDelta(HallButtonId button, uint16_t delta)
{
  if (button >= HALL_BUTTON_COUNT)
  {
    return;
  }

  Settings_GetMutableButtonSettings(button)->rapid_trigger_release_delta = Settings_ClampAdcValue(delta);
}

uint16_t Settings_GetActuationDistance(void)
{
  return Settings_GetActiveProfile()->actuation_distance;
}

void Settings_SetActuationDistance(uint16_t distance)
{
  ControllerProfile *profile = Settings_GetMutableActiveProfile();

  profile->actuation_distance = Settings_ClampActuationDistance(distance);
  Settings_ApplyGlobalActuationDistance(profile);
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
