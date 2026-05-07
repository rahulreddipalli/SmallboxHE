#include "calibration.h"

#include "settings.h"

#define CAL_STABLE_TIME_MS 250U
#define CAL_PRESS_DELTA_ADC 250U

static CalibrationState calibration_state;
static uint32_t state_started_tick;
static uint16_t rest_capture[HALL_BUTTON_COUNT];
static uint16_t min_seen_adc[HALL_BUTTON_COUNT];
static uint16_t max_seen_adc[HALL_BUTTON_COUNT];
static uint8_t button_calibrated[HALL_BUTTON_COUNT];
static uint8_t calibrated_count;

static void Calibration_EnterState(CalibrationState next_state)
{
  calibration_state = next_state;
  state_started_tick = HAL_GetTick();
}

static uint8_t Calibration_HasElapsed(uint32_t duration_ms)
{
  return ((HAL_GetTick() - state_started_tick) >= duration_ms) ? 1U : 0U;
}

static uint16_t Calibration_ReadButtonAdc(HallButtonId button)
{
  uint32_t value = Hall_Buttons_GetAdcValue(button);

  if (value > SETTINGS_ADC_MAX)
  {
    value = SETTINGS_ADC_MAX;
  }

  return (uint16_t)value;
}

static void Calibration_ResetRuntime(void)
{
  uint8_t index;

  calibrated_count = 0U;

  for (index = 0U; index < HALL_BUTTON_COUNT; index++)
  {
    rest_capture[index] = 0U;
    min_seen_adc[index] = SETTINGS_ADC_MAX;
    max_seen_adc[index] = SETTINGS_ADC_MIN;
    button_calibrated[index] = 0U;
  }
}

static void Calibration_CaptureRest(void)
{
  uint8_t index;
  uint16_t adc_value;

  for (index = 0U; index < HALL_BUTTON_COUNT; index++)
  {
    adc_value = Calibration_ReadButtonAdc((HallButtonId)index);
    rest_capture[index] = adc_value;
    min_seen_adc[index] = adc_value;
    max_seen_adc[index] = adc_value;
  }
}

static void Calibration_UpdatePressedCapture(void)
{
  uint8_t index;
  uint16_t adc_value;
  uint16_t delta;
  ButtonCalibration calibration;

  for (index = 0U; index < HALL_BUTTON_COUNT; index++)
  {
    adc_value = Calibration_ReadButtonAdc((HallButtonId)index);

    if (adc_value < min_seen_adc[index])
    {
      min_seen_adc[index] = adc_value;
    }

    if (adc_value > max_seen_adc[index])
    {
      max_seen_adc[index] = adc_value;
    }

    delta = (adc_value > rest_capture[index]) ? (adc_value - rest_capture[index]) : (rest_capture[index] - adc_value);

    if (button_calibrated[index] == 0U && delta >= CAL_PRESS_DELTA_ADC)
    {
      calibration.rest_adc = rest_capture[index];
      calibration.pressed_adc = adc_value;
      calibration.min_seen_adc = min_seen_adc[index];
      calibration.max_seen_adc = max_seen_adc[index];
      calibration.calibrated = 1U;
      Settings_SetButtonCalibration((HallButtonId)index, &calibration);

      button_calibrated[index] = 1U;
      calibrated_count++;
    }
  }
}

void Calibration_Init(void)
{
  Calibration_ResetRuntime();
  Calibration_EnterState(CAL_IDLE);
}

void Calibration_Start(void)
{
  Calibration_ResetRuntime();
  Settings_SetCalibrationModeEnabled(1U);
  Calibration_EnterState(CAL_WAIT_RELEASE);
}

void Calibration_Cancel(void)
{
  Settings_SetCalibrationModeEnabled(0U);
  Calibration_EnterState(CAL_IDLE);
}

void Calibration_Tick(void)
{
  switch (calibration_state)
  {
    case CAL_IDLE:
      break;

    case CAL_WAIT_RELEASE:
      if (Calibration_HasElapsed(CAL_STABLE_TIME_MS) != 0U)
      {
        Calibration_EnterState(CAL_CAPTURE_REST);
      }
      break;

    case CAL_CAPTURE_REST:
      Calibration_CaptureRest();
      Calibration_EnterState(CAL_WAIT_PRESS_ANY_ORDER);
      break;

    case CAL_WAIT_PRESS_ANY_ORDER:
      Calibration_UpdatePressedCapture();
      if (calibrated_count >= HALL_BUTTON_COUNT)
      {
        Calibration_EnterState(CAL_CAPTURE_PRESSED);
      }
      break;

    case CAL_CAPTURE_PRESSED:
      Settings_SetCalibrationModeEnabled(0U);
      Calibration_EnterState(CAL_DONE);
      break;

    case CAL_DONE:
      break;

    default:
      Calibration_Cancel();
      break;
  }
}

CalibrationState Calibration_GetState(void)
{
  return calibration_state;
}

uint8_t Calibration_IsActive(void)
{
  return (calibration_state != CAL_IDLE && calibration_state != CAL_DONE) ? 1U : 0U;
}

uint8_t Calibration_GetCalibratedCount(void)
{
  return calibrated_count;
}
