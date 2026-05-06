#include "hall_buttons.h"

#include "settings.h"

typedef enum
{
  HALL_ADC_SOURCE_ADC1 = 0,
  HALL_ADC_SOURCE_ADC2
} HallAdcSource;

/*
 * Per-button mapping data.
 * Each entry describes where the Hall ADC data comes from,
 * which GPIO controls the gate output, and the current thresholds.
 */
typedef struct
{
  HallAdcSource hall_adc_source;
  uint32_t hall_adc_sample_index;
  GPIO_TypeDef *gate_port;
  uint16_t gate_pin;
  uint32_t press_threshold;
  uint32_t release_threshold;
  uint32_t hall_adc_value;
  GPIO_PinState gate_state;
} HallButtonMap;

/* Pointers to the DMA-backed ADC sample buffers owned by board.c. */
static const volatile uint16_t *hall_adc1_samples = NULL;
static const volatile uint16_t *hall_adc2_samples = NULL;

/* Default global threshold values used before settings are applied. */
static const uint32_t HALL_PRESS_THRESHOLD = 2200U;
static const uint32_t HALL_RELEASE_THRESHOLD = 1800U;
static const uint32_t HALL_THRESHOLD_CENTER = (HALL_PRESS_THRESHOLD + HALL_RELEASE_THRESHOLD) / 2U;

#define HALL_BUTTON_MAP(adc_source, sample_index, output_port, output_pin) \
  { adc_source, sample_index, output_port, output_pin, HALL_PRESS_THRESHOLD, HALL_RELEASE_THRESHOLD, 0, GPIO_PIN_RESET }

/*
 * Static mapping of physical Hall inputs to gate outputs.
 * The sample_index is the position inside the ADC DMA buffer for that ADC instance.
 */
static HallButtonMap hall_button_map[HALL_BUTTON_COUNT] =
{
  [HALL_BUTTON_LEFT]     = HALL_BUTTON_MAP(HALL_ADC_SOURCE_ADC2, 0U, GPIOA, GPIO_PIN_10), /* LEFT_HE -> LEFT_GATE */
  [HALL_BUTTON_DOWN]     = HALL_BUTTON_MAP(HALL_ADC_SOURCE_ADC2, 1U, GPIOA, GPIO_PIN_9),  /* DOWN_HE -> DOWN_GATE */
  [HALL_BUTTON_RIGHT]    = HALL_BUTTON_MAP(HALL_ADC_SOURCE_ADC2, 2U, GPIOA, GPIO_PIN_8),  /* RIGHT_HE -> RIGHT_GATE */
  [HALL_BUTTON_UP]       = HALL_BUTTON_MAP(HALL_ADC_SOURCE_ADC2, 3U, GPIOC, GPIO_PIN_6),  /* UP_HE -> UP_GATE */
  [HALL_BUTTON_SQUARE]   = HALL_BUTTON_MAP(HALL_ADC_SOURCE_ADC1, 0U, GPIOC, GPIO_PIN_7),  /* SQUARE_HE -> SQUARE_GATE */
  [HALL_BUTTON_TRIANGLE] = HALL_BUTTON_MAP(HALL_ADC_SOURCE_ADC1, 1U, GPIOB, GPIO_PIN_3),  /* TRIANGLE_HE -> TRIANGLE_GATE */
  [HALL_BUTTON_L1]       = HALL_BUTTON_MAP(HALL_ADC_SOURCE_ADC1, 2U, GPIOB, GPIO_PIN_4),  /* L1_HE -> L1_GATE */
  [HALL_BUTTON_R1]       = HALL_BUTTON_MAP(HALL_ADC_SOURCE_ADC1, 3U, GPIOB, GPIO_PIN_5),  /* R1_HE -> R1_GATE */
  [HALL_BUTTON_CROSS]    = HALL_BUTTON_MAP(HALL_ADC_SOURCE_ADC1, 4U, GPIOB, GPIO_PIN_6),  /* CROSS_HE -> CROSS_GATE */
  [HALL_BUTTON_CIRCLE]   = HALL_BUTTON_MAP(HALL_ADC_SOURCE_ADC1, 5U, GPIOB, GPIO_PIN_7),  /* CIRCLE_HE -> CIRCLE_GATE */
  [HALL_BUTTON_L2]       = HALL_BUTTON_MAP(HALL_ADC_SOURCE_ADC1, 6U, GPIOC, GPIO_PIN_9),  /* L2_HE -> L2_GATE */
  [HALL_BUTTON_R2]       = HALL_BUTTON_MAP(HALL_ADC_SOURCE_ADC1, 7U, GPIOC, GPIO_PIN_8),  /* R2_HE -> R2_GATE */
  [HALL_BUTTON_L3]       = HALL_BUTTON_MAP(HALL_ADC_SOURCE_ADC2, 4U, GPIOA, GPIO_PIN_11), /* LMOD_HE -> L3_GATE */
  [HALL_BUTTON_R3]       = HALL_BUTTON_MAP(HALL_ADC_SOURCE_ADC2, 5U, GPIOB, GPIO_PIN_9)   /* RMOD_HE -> R3_GATE */
};

/*
 * Clamp a threshold into the valid 12-bit ADC range.
 * This keeps addition/subtraction from overflowing the ADC limits.
 */
static uint32_t Hall_ClampThreshold(int32_t threshold)
{
  if (threshold < 0)
  {
    return 0U;
  }

  if (threshold > 4095)
  {
    return 4095U;
  }

  return (uint32_t)threshold;
}

/*
 * Apply runtime settings to every button mapping.
 * Current implementation uses a single global actuation distance,
 * producing symmetric press/release thresholds around the midpoint.
 */
void Hall_Buttons_ApplySettings(void)
{
  uint32_t index;
  uint16_t actuation_distance = Settings_GetActuationDistance();
  uint32_t half_distance = (uint32_t)actuation_distance / 2U;
  uint32_t press_threshold = Hall_ClampThreshold((int32_t)HALL_THRESHOLD_CENTER + (int32_t)half_distance);
  uint32_t release_threshold = Hall_ClampThreshold((int32_t)HALL_THRESHOLD_CENTER - (int32_t)half_distance);

  for (index = 0; index < HALL_BUTTON_COUNT; index++)
  {
    hall_button_map[index].press_threshold = press_threshold;
    hall_button_map[index].release_threshold = release_threshold;
  }
}

/*
 * Read the latest DMA sample for the given button.
 * This returns the current raw ADC reading for the selected ADC instance.
 */
static uint32_t Hall_ReadSample(const HallButtonMap *button)
{
  if (button->hall_adc_source == HALL_ADC_SOURCE_ADC1)
  {
    if (hall_adc1_samples == NULL)
    {
      Error_Handler();
    }

    return hall_adc1_samples[button->hall_adc_sample_index];
  }

  if (hall_adc2_samples == NULL)
  {
    Error_Handler();
  }

  return hall_adc2_samples[button->hall_adc_sample_index];
}

/*
 * Update a single button's gate state based on the current ADC value.
 * Press and release thresholds implement simple hysteresis.
 */
static void Hall_Button_UpdateState(HallButtonMap *button)
{
  button->hall_adc_value = Hall_ReadSample(button);

  if (button->gate_state == GPIO_PIN_RESET && button->hall_adc_value >= button->press_threshold)
  {
    button->gate_state = GPIO_PIN_SET;
  }
  else if (button->gate_state == GPIO_PIN_SET && button->hall_adc_value <= button->release_threshold)
  {
    button->gate_state = GPIO_PIN_RESET;
  }
}

/*
 * OR-combine two gate states, so multiple sources can drive the same output.
 */
static GPIO_PinState Hall_OrGateState(GPIO_PinState first, GPIO_PinState second)
{
  return (first == GPIO_PIN_SET || second == GPIO_PIN_SET) ? GPIO_PIN_SET : GPIO_PIN_RESET;
}

/*
 * Drive the physical GPIO gate pin for one button.
 */
static void Hall_WriteGate(HallButtonId button, GPIO_PinState state)
{
  HAL_GPIO_WritePin(hall_button_map[button].gate_port, hall_button_map[button].gate_pin, state);
}

/*
 * Compute the routed output state for every gate and write them.
 * This applies profile routing from settings, allowing one-to-one or shared outputs.
 */
static void Hall_WriteOutputs(void)
{
  GPIO_PinState output_states[HALL_BUTTON_COUNT] = {GPIO_PIN_RESET};
  uint32_t source_index;
  uint8_t output_button;

  for (source_index = 0U; source_index < HALL_BUTTON_COUNT; source_index++)
  {
    output_button = Settings_GetRouteOutput((HallButtonId)source_index);

    if (output_button < HALL_BUTTON_COUNT)
    {
      output_states[output_button] = Hall_OrGateState(output_states[output_button], hall_button_map[source_index].gate_state);
    }
  }

  for (source_index = 0U; source_index < HALL_BUTTON_COUNT; source_index++)
  {
    Hall_WriteGate((HallButtonId)source_index, output_states[source_index]);
  }
}

/*
 * Initialize the hall button module with pointers to the DMA ADC buffers.
 * Also apply current thresholds from settings.
 */
void Hall_Buttons_Init(const volatile uint16_t *adc1_samples, const volatile uint16_t *adc2_samples)
{
  hall_adc1_samples = adc1_samples;
  hall_adc2_samples = adc2_samples;
  Hall_Buttons_ApplySettings();
}

/*
 * Update every button once and then write the computed outputs.
 * This is the main per-loop entry point for hall button processing.
 */
void Hall_Buttons_UpdateAll(void)
{
  uint32_t index;

  for (index = 0; index < HALL_BUTTON_COUNT; index++)
  {
    Hall_Button_UpdateState(&hall_button_map[index]);
  }

  Hall_WriteOutputs();
}

/* Return the most recent raw ADC value for a button. */
uint32_t Hall_Buttons_GetAdcValue(HallButtonId button)
{
  if (button >= HALL_BUTTON_COUNT)
  {
    return 0U;
  }

  return hall_button_map[button].hall_adc_value;
}

/* Return the current projected gate output state for a button. */
GPIO_PinState Hall_Buttons_GetGateState(HallButtonId button)
{
  if (button >= HALL_BUTTON_COUNT)
  {
    return GPIO_PIN_RESET;
  }

  return hall_button_map[button].gate_state;
}
