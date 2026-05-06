#include "hall_buttons.h"

typedef struct
{
  ADC_HandleTypeDef **adc;
  uint32_t channel;
  GPIO_TypeDef *output_port;
  uint16_t output_pin;
  uint32_t press_threshold;
  uint32_t release_threshold;
  uint32_t value;
  GPIO_PinState output_state;
} HallButton;

static ADC_HandleTypeDef *hall_adc1 = NULL;
static ADC_HandleTypeDef *hall_adc2 = NULL;

static const uint32_t HALL_PRESS_THRESHOLD = 2200;
static const uint32_t HALL_RELEASE_THRESHOLD = 1800;

static HallButton hall_buttons[HALL_BUTTON_COUNT] =
{
  { &hall_adc2, ADC_CHANNEL_17, GPIOA, GPIO_PIN_8,  HALL_PRESS_THRESHOLD, HALL_RELEASE_THRESHOLD, 0, GPIO_PIN_RESET },
  { &hall_adc1, ADC_CHANNEL_1,  GPIOA, GPIO_PIN_9,  HALL_PRESS_THRESHOLD, HALL_RELEASE_THRESHOLD, 0, GPIO_PIN_RESET },
  { &hall_adc1, ADC_CHANNEL_2,  GPIOA, GPIO_PIN_10, HALL_PRESS_THRESHOLD, HALL_RELEASE_THRESHOLD, 0, GPIO_PIN_RESET },
  { &hall_adc1, ADC_CHANNEL_3,  GPIOA, GPIO_PIN_11, HALL_PRESS_THRESHOLD, HALL_RELEASE_THRESHOLD, 0, GPIO_PIN_RESET },
  { &hall_adc1, ADC_CHANNEL_4,  GPIOB, GPIO_PIN_3,  HALL_PRESS_THRESHOLD, HALL_RELEASE_THRESHOLD, 0, GPIO_PIN_RESET },
  { &hall_adc2, ADC_CHANNEL_13, GPIOB, GPIO_PIN_4,  HALL_PRESS_THRESHOLD, HALL_RELEASE_THRESHOLD, 0, GPIO_PIN_RESET },
  { &hall_adc2, ADC_CHANNEL_3,  GPIOB, GPIO_PIN_5,  HALL_PRESS_THRESHOLD, HALL_RELEASE_THRESHOLD, 0, GPIO_PIN_RESET },
  { &hall_adc2, ADC_CHANNEL_4,  GPIOB, GPIO_PIN_6,  HALL_PRESS_THRESHOLD, HALL_RELEASE_THRESHOLD, 0, GPIO_PIN_RESET },
  { &hall_adc1, ADC_CHANNEL_6,  GPIOB, GPIO_PIN_7,  HALL_PRESS_THRESHOLD, HALL_RELEASE_THRESHOLD, 0, GPIO_PIN_RESET },
  { &hall_adc1, ADC_CHANNEL_7,  GPIOB, GPIO_PIN_9,  HALL_PRESS_THRESHOLD, HALL_RELEASE_THRESHOLD, 0, GPIO_PIN_RESET },
  { &hall_adc1, ADC_CHANNEL_8,  GPIOB, GPIO_PIN_10, HALL_PRESS_THRESHOLD, HALL_RELEASE_THRESHOLD, 0, GPIO_PIN_RESET },
  { &hall_adc1, ADC_CHANNEL_9,  GPIOB, GPIO_PIN_11, HALL_PRESS_THRESHOLD, HALL_RELEASE_THRESHOLD, 0, GPIO_PIN_RESET },
  { &hall_adc2, ADC_CHANNEL_5,  GPIOB, GPIO_PIN_12, HALL_PRESS_THRESHOLD, HALL_RELEASE_THRESHOLD, 0, GPIO_PIN_RESET },
  { &hall_adc2, ADC_CHANNEL_11, GPIOC, GPIO_PIN_6,  HALL_PRESS_THRESHOLD, HALL_RELEASE_THRESHOLD, 0, GPIO_PIN_RESET }
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

static void Hall_Button_Update(HallButton *button)
{
  ADC_HandleTypeDef *adc = *button->adc;

  if (adc == NULL)
  {
    Error_Handler();
  }

  button->value = Hall_ReadADC(adc, button->channel);

  if (button->output_state == GPIO_PIN_RESET && button->value >= button->press_threshold)
  {
    button->output_state = GPIO_PIN_SET;
  }
  else if (button->output_state == GPIO_PIN_SET && button->value <= button->release_threshold)
  {
    button->output_state = GPIO_PIN_RESET;
  }

  HAL_GPIO_WritePin(button->output_port, button->output_pin, button->output_state);
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
    Hall_Button_Update(&hall_buttons[index]);
  }
}
