#include "hall_buttons.h"

#include "settings.h"

typedef enum
{
  HALL_ADC_SOURCE_ADC1 = 0,
  HALL_ADC_SOURCE_ADC2
} HallAdcSource;

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

static const volatile uint16_t *hall_adc1_samples = NULL;
static const volatile uint16_t *hall_adc2_samples = NULL;

static const uint32_t HALL_PRESS_THRESHOLD = 2200U;
static const uint32_t HALL_RELEASE_THRESHOLD = 1800U;
static const uint32_t HALL_THRESHOLD_CENTER = (HALL_PRESS_THRESHOLD + HALL_RELEASE_THRESHOLD) / 2U;

#define HALL_BUTTON_MAP(adc_source, sample_index, output_port, output_pin) \
  { adc_source, sample_index, output_port, output_pin, HALL_PRESS_THRESHOLD, HALL_RELEASE_THRESHOLD, 0, GPIO_PIN_RESET }

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

static GPIO_PinState Hall_OrGateState(GPIO_PinState first, GPIO_PinState second)
{
  return (first == GPIO_PIN_SET || second == GPIO_PIN_SET) ? GPIO_PIN_SET : GPIO_PIN_RESET;
}

static void Hall_WriteGate(HallButtonId button, GPIO_PinState state)
{
  HAL_GPIO_WritePin(hall_button_map[button].gate_port, hall_button_map[button].gate_pin, state);
}

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

void Hall_Buttons_Init(const volatile uint16_t *adc1_samples, const volatile uint16_t *adc2_samples)
{
  hall_adc1_samples = adc1_samples;
  hall_adc2_samples = adc2_samples;
  Hall_Buttons_ApplySettings();
}

void Hall_Buttons_UpdateAll(void)
{
  uint32_t index;

  for (index = 0; index < HALL_BUTTON_COUNT; index++)
  {
    Hall_Button_UpdateState(&hall_button_map[index]);
  }

  Hall_WriteOutputs();
}

uint32_t Hall_Buttons_GetAdcValue(HallButtonId button)
{
  if (button >= HALL_BUTTON_COUNT)
  {
    return 0U;
  }

  return hall_button_map[button].hall_adc_value;
}

GPIO_PinState Hall_Buttons_GetGateState(HallButtonId button)
{
  if (button >= HALL_BUTTON_COUNT)
  {
    return GPIO_PIN_RESET;
  }

  return hall_button_map[button].gate_state;
}
