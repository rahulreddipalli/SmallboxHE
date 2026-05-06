#include "hall_buttons.h"

typedef struct
{
  ADC_HandleTypeDef **hall_adc;
  uint32_t hall_adc_channel;
  GPIO_TypeDef *brook_output_port;
  uint16_t brook_output_pin;
  uint32_t press_threshold;
  uint32_t release_threshold;
  uint32_t hall_adc_value;
  GPIO_PinState brook_output_state;
} HallButtonMap;

static ADC_HandleTypeDef *hall_adc1 = NULL;
static ADC_HandleTypeDef *hall_adc2 = NULL;

static const uint32_t HALL_PRESS_THRESHOLD = 2200;
static const uint32_t HALL_RELEASE_THRESHOLD = 1800;

#define HALL_BUTTON_MAP(hall_adc, adc_channel, output_port, output_pin) \
  { hall_adc, adc_channel, output_port, output_pin, HALL_PRESS_THRESHOLD, HALL_RELEASE_THRESHOLD, 0, GPIO_PIN_RESET }

static HallButtonMap hall_button_map[HALL_BUTTON_COUNT] =
{
  [HALL_BUTTON_LEFT]     = HALL_BUTTON_MAP(&hall_adc2, ADC_CHANNEL_17, GPIOA, GPIO_PIN_10), /* LEFT_HE -> LEFT_NPN */
  [HALL_BUTTON_DOWN]     = HALL_BUTTON_MAP(&hall_adc2, ADC_CHANNEL_13, GPIOA, GPIO_PIN_9),  /* DOWN_HE -> DOWN_NPN */
  [HALL_BUTTON_RIGHT]    = HALL_BUTTON_MAP(&hall_adc2, ADC_CHANNEL_3,  GPIOA, GPIO_PIN_8),  /* RIGHT_HE -> RIGHT_NPN */
  [HALL_BUTTON_UP]       = HALL_BUTTON_MAP(&hall_adc2, ADC_CHANNEL_4,  GPIOC, GPIO_PIN_6),  /* UP_HE -> UP_NPN */
  [HALL_BUTTON_SQUARE]   = HALL_BUTTON_MAP(&hall_adc1, ADC_CHANNEL_1,  GPIOC, GPIO_PIN_7),  /* SQUARE_HE -> SQUARE_NPN */
  [HALL_BUTTON_TRIANGLE] = HALL_BUTTON_MAP(&hall_adc1, ADC_CHANNEL_2,  GPIOB, GPIO_PIN_3),  /* TRIANGLE_HE -> TRIANGLE_NPN */
  [HALL_BUTTON_L1]       = HALL_BUTTON_MAP(&hall_adc1, ADC_CHANNEL_3,  GPIOB, GPIO_PIN_4),  /* L1_HE -> L1_NPN */
  [HALL_BUTTON_R1]       = HALL_BUTTON_MAP(&hall_adc1, ADC_CHANNEL_4,  GPIOB, GPIO_PIN_5),  /* R1_HE -> R1_NPN */
  [HALL_BUTTON_CROSS]    = HALL_BUTTON_MAP(&hall_adc1, ADC_CHANNEL_6,  GPIOB, GPIO_PIN_6),  /* CROSS_HE -> CROSS_NPN */
  [HALL_BUTTON_CIRCLE]   = HALL_BUTTON_MAP(&hall_adc1, ADC_CHANNEL_7,  GPIOB, GPIO_PIN_7),  /* CIRCLE_HE -> CIRCLE_NPN */
  [HALL_BUTTON_L2]       = HALL_BUTTON_MAP(&hall_adc1, ADC_CHANNEL_8,  GPIOC, GPIO_PIN_9),  /* L2_HE -> L2_NPN */
  [HALL_BUTTON_R2]       = HALL_BUTTON_MAP(&hall_adc1, ADC_CHANNEL_9,  GPIOC, GPIO_PIN_8),  /* R2_HE -> R2_NPN */
  [HALL_BUTTON_L3]       = HALL_BUTTON_MAP(&hall_adc2, ADC_CHANNEL_5,  GPIOA, GPIO_PIN_11), /* LMOD_HE -> L3_NPN */
  [HALL_BUTTON_R3]       = HALL_BUTTON_MAP(&hall_adc2, ADC_CHANNEL_11, GPIOB, GPIO_PIN_9)   /* RMOD_HE -> R3_NPN */
};

static uint32_t Hall_ReadADC(ADC_HandleTypeDef *hadc, uint32_t channel)
{
  ADC_ChannelConfTypeDef sConfig = {0};
  uint32_t value = 0;

  sConfig.Channel = channel;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_2CYCLES_5;
  sConfig.SingleDiff = ADC_SINGLE_ENDED;
  sConfig.OffsetNumber = ADC_OFFSET_NONE;
  sConfig.Offset = 0;

  if (HAL_ADC_ConfigChannel(hadc, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_ADC_Start(hadc) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_ADC_PollForConversion(hadc, 10) == HAL_OK)
  {
    value = HAL_ADC_GetValue(hadc);
  }

  HAL_ADC_Stop(hadc);

  return value;
}

static void Hall_Button_Update(HallButtonMap *button)
{
  ADC_HandleTypeDef *adc = *button->hall_adc;

  if (adc == NULL)
  {
    Error_Handler();
  }

  button->hall_adc_value = Hall_ReadADC(adc, button->hall_adc_channel);

  if (button->brook_output_state == GPIO_PIN_RESET && button->hall_adc_value >= button->press_threshold)
  {
    button->brook_output_state = GPIO_PIN_SET;
  }
  else if (button->brook_output_state == GPIO_PIN_SET && button->hall_adc_value <= button->release_threshold)
  {
    button->brook_output_state = GPIO_PIN_RESET;
  }

  HAL_GPIO_WritePin(button->brook_output_port, button->brook_output_pin, button->brook_output_state);
}

void Hall_Buttons_Init(ADC_HandleTypeDef *adc1, ADC_HandleTypeDef *adc2)
{
  hall_adc1 = adc1;
  hall_adc2 = adc2;
}

void Hall_Buttons_UpdateAll(void)
{
  uint32_t index;

  for (index = 0; index < HALL_BUTTON_COUNT; index++)
  {
    Hall_Button_Update(&hall_button_map[index]);
  }
}
