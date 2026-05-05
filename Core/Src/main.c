/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct
{
  const char *name;
  volatile uint16_t *adc_value_source;
  GPIO_TypeDef *output_port;
  uint16_t output_pin;
  uint32_t press_threshold;
  uint32_t release_threshold;
  uint32_t adc_value;
  GPIO_PinState output_state;
} ButtonChannel;

typedef struct
{
  GPIO_TypeDef *a_port;
  uint16_t a_pin;
  GPIO_TypeDef *b_port;
  uint16_t b_pin;
  GPIO_TypeDef *sw_port;
  uint16_t sw_pin;
  uint8_t stable_ab;
  uint8_t last_raw_ab;
  uint32_t last_ab_change_ms;
  GPIO_PinState stable_sw;
  GPIO_PinState last_raw_sw;
  uint32_t last_sw_change_ms;
  uint8_t switch_pressed;
} RotaryEncoder;

typedef struct
{
  uint8_t rot_l;
  uint8_t rot_r;
  uint8_t rot_click;
  uint8_t oled_menu_button;
} InputEvents;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
ADC_HandleTypeDef hadc2;

DMA_HandleTypeDef hdma_adc1;
DMA_HandleTypeDef hdma_adc2;

/* USER CODE BEGIN PV */
#define BUTTON_COUNT 14U
#define ADC1_BUTTON_COUNT 8U
#define ADC2_BUTTON_COUNT 6U
#define DEFAULT_PRESS_THRESHOLD 2200U
#define DEFAULT_RELEASE_THRESHOLD 1800U
#define ENCODER_DEBOUNCE_MS 2U
#define ENCODER_SWITCH_DEBOUNCE_MS 10U
#define OLED_MENU_BUTTON_DEBOUNCE_MS 10U

#define OLED_MENU_BUTTON_PORT GPIOA
#define OLED_MENU_BUTTON_PIN  GPIO_PIN_12

#define LEFT_HE_ADC        adc1_values[0]
#define DOWN_HE_ADC        adc1_values[1]
#define RIGHT_HE_ADC       adc1_values[2]
#define UP_HE_ADC          adc1_values[3]
#define CROSS_HE_ADC       adc1_values[4]
#define CIRCLE_HE_ADC      adc1_values[5]
#define L2_HE_ADC          adc1_values[6]
#define R2_HE_ADC          adc1_values[7]

#define SQUARE_HE_ADC      adc2_values[0]
#define TRIANGLE_HE_ADC    adc2_values[1]
#define L1_HE_ADC          adc2_values[2]
#define R1_HE_ADC          adc2_values[3]
#define L3_HE_ADC          adc2_values[4]
#define R3_HE_ADC          adc2_values[5]

#define LEFT_MOSFET_PORT       GPIOA
#define LEFT_MOSFET_PIN        GPIO_PIN_10
#define DOWN_MOSFET_PORT       GPIOA
#define DOWN_MOSFET_PIN        GPIO_PIN_9
#define RIGHT_MOSFET_PORT      GPIOA
#define RIGHT_MOSFET_PIN       GPIO_PIN_8
#define UP_MOSFET_PORT         GPIOC
#define UP_MOSFET_PIN          GPIO_PIN_6
#define SQUARE_MOSFET_PORT     GPIOC
#define SQUARE_MOSFET_PIN      GPIO_PIN_7
#define TRIANGLE_MOSFET_PORT   GPIOB
#define TRIANGLE_MOSFET_PIN    GPIO_PIN_3
#define L1_MOSFET_PORT         GPIOB
#define L1_MOSFET_PIN          GPIO_PIN_4
#define R1_MOSFET_PORT         GPIOB
#define R1_MOSFET_PIN          GPIO_PIN_5
#define CROSS_MOSFET_PORT      GPIOB
#define CROSS_MOSFET_PIN       GPIO_PIN_6
#define CIRCLE_MOSFET_PORT     GPIOB
#define CIRCLE_MOSFET_PIN      GPIO_PIN_7
#define L2_MOSFET_PORT         GPIOC
#define L2_MOSFET_PIN          GPIO_PIN_9
#define R2_MOSFET_PORT         GPIOC
#define R2_MOSFET_PIN          GPIO_PIN_8
#define L3_MOSFET_PORT         GPIOA
#define L3_MOSFET_PIN          GPIO_PIN_11
#define R3_MOSFET_PORT         GPIOB
#define R3_MOSFET_PIN          GPIO_PIN_9

static volatile uint16_t adc1_values[ADC1_BUTTON_COUNT];
static volatile uint16_t adc2_values[ADC2_BUTTON_COUNT];

static ButtonChannel buttons[BUTTON_COUNT] =
{
  /* name       Hall input             MOSFET output */
  { "LEFT",     &LEFT_HE_ADC,     LEFT_MOSFET_PORT,     LEFT_MOSFET_PIN,     DEFAULT_PRESS_THRESHOLD, DEFAULT_RELEASE_THRESHOLD, 0U, GPIO_PIN_RESET },
  { "DOWN",     &DOWN_HE_ADC,     DOWN_MOSFET_PORT,     DOWN_MOSFET_PIN,     DEFAULT_PRESS_THRESHOLD, DEFAULT_RELEASE_THRESHOLD, 0U, GPIO_PIN_RESET },
  { "RIGHT",    &RIGHT_HE_ADC,    RIGHT_MOSFET_PORT,    RIGHT_MOSFET_PIN,    DEFAULT_PRESS_THRESHOLD, DEFAULT_RELEASE_THRESHOLD, 0U, GPIO_PIN_RESET },
  { "UP",       &UP_HE_ADC,       UP_MOSFET_PORT,       UP_MOSFET_PIN,       DEFAULT_PRESS_THRESHOLD, DEFAULT_RELEASE_THRESHOLD, 0U, GPIO_PIN_RESET },
  { "SQUARE",   &SQUARE_HE_ADC,   SQUARE_MOSFET_PORT,   SQUARE_MOSFET_PIN,   DEFAULT_PRESS_THRESHOLD, DEFAULT_RELEASE_THRESHOLD, 0U, GPIO_PIN_RESET },
  { "TRIANGLE", &TRIANGLE_HE_ADC, TRIANGLE_MOSFET_PORT, TRIANGLE_MOSFET_PIN, DEFAULT_PRESS_THRESHOLD, DEFAULT_RELEASE_THRESHOLD, 0U, GPIO_PIN_RESET },
  { "L1",       &L1_HE_ADC,       L1_MOSFET_PORT,       L1_MOSFET_PIN,       DEFAULT_PRESS_THRESHOLD, DEFAULT_RELEASE_THRESHOLD, 0U, GPIO_PIN_RESET },
  { "R1",       &R1_HE_ADC,       R1_MOSFET_PORT,       R1_MOSFET_PIN,       DEFAULT_PRESS_THRESHOLD, DEFAULT_RELEASE_THRESHOLD, 0U, GPIO_PIN_RESET },
  { "CROSS",    &CROSS_HE_ADC,    CROSS_MOSFET_PORT,    CROSS_MOSFET_PIN,    DEFAULT_PRESS_THRESHOLD, DEFAULT_RELEASE_THRESHOLD, 0U, GPIO_PIN_RESET },
  { "CIRCLE",   &CIRCLE_HE_ADC,   CIRCLE_MOSFET_PORT,   CIRCLE_MOSFET_PIN,   DEFAULT_PRESS_THRESHOLD, DEFAULT_RELEASE_THRESHOLD, 0U, GPIO_PIN_RESET },
  { "L2",       &L2_HE_ADC,       L2_MOSFET_PORT,       L2_MOSFET_PIN,       DEFAULT_PRESS_THRESHOLD, DEFAULT_RELEASE_THRESHOLD, 0U, GPIO_PIN_RESET },
  { "R2",       &R2_HE_ADC,       R2_MOSFET_PORT,       R2_MOSFET_PIN,       DEFAULT_PRESS_THRESHOLD, DEFAULT_RELEASE_THRESHOLD, 0U, GPIO_PIN_RESET },
  { "L3",       &L3_HE_ADC,       L3_MOSFET_PORT,       L3_MOSFET_PIN,       DEFAULT_PRESS_THRESHOLD, DEFAULT_RELEASE_THRESHOLD, 0U, GPIO_PIN_RESET },
  { "R3",       &R3_HE_ADC,       R3_MOSFET_PORT,       R3_MOSFET_PIN,       DEFAULT_PRESS_THRESHOLD, DEFAULT_RELEASE_THRESHOLD, 0U, GPIO_PIN_RESET }
};

static RotaryEncoder rotary_encoder =
{
  GPIOB, GPIO_PIN_0,
  GPIOB, GPIO_PIN_1,
  GPIOB, GPIO_PIN_2,
  0U,
  0U,
  0U,
  GPIO_PIN_SET,
  GPIO_PIN_SET,
  0U,
  0U
};

static GPIO_PinState oled_menu_button_stable_state = GPIO_PIN_SET;
static GPIO_PinState oled_menu_button_last_raw_state = GPIO_PIN_SET;
static uint32_t oled_menu_button_last_change_ms = 0U;

static InputEvents input_events = {0U, 0U, 0U, 0U};

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_ADC1_Init(void);
static void MX_ADC2_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static void Update_Button(ButtonChannel *button)
{
  button->adc_value = *button->adc_value_source;

  if (button->output_state == GPIO_PIN_RESET && button->adc_value >= button->press_threshold)
  {
    button->output_state = GPIO_PIN_SET;
  }
  else if (button->output_state == GPIO_PIN_SET && button->adc_value <= button->release_threshold)
  {
    button->output_state = GPIO_PIN_RESET;
  }

  HAL_GPIO_WritePin(button->output_port, button->output_pin, button->output_state);
}

static void Update_All_Buttons(void)
{
  for (uint32_t i = 0U; i < BUTTON_COUNT; i++)
  {
    Update_Button(&buttons[i]);
  }
}

static uint8_t Read_Rotary_AB(const RotaryEncoder *encoder)
{
  uint8_t a = (HAL_GPIO_ReadPin(encoder->a_port, encoder->a_pin) == GPIO_PIN_SET) ? 1U : 0U;
  uint8_t b = (HAL_GPIO_ReadPin(encoder->b_port, encoder->b_pin) == GPIO_PIN_SET) ? 1U : 0U;

  return (uint8_t)((a << 1U) | b);
}

static void Update_Rotary_Encoder(RotaryEncoder *encoder)
{
  static const int8_t transition_table[16] =
  {
     0, -1,  1,  0,
     1,  0,  0, -1,
    -1,  0,  0,  1,
     0,  1, -1,  0
  };

  uint32_t now = HAL_GetTick();
  uint8_t raw_ab = Read_Rotary_AB(encoder);
  GPIO_PinState raw_sw = HAL_GPIO_ReadPin(encoder->sw_port, encoder->sw_pin);

  if (raw_ab != encoder->last_raw_ab)
  {
    encoder->last_raw_ab = raw_ab;
    encoder->last_ab_change_ms = now;
  }
  else if ((now - encoder->last_ab_change_ms) >= ENCODER_DEBOUNCE_MS && raw_ab != encoder->stable_ab)
  {
    uint8_t transition = (uint8_t)((encoder->stable_ab << 2U) | raw_ab);
    int8_t direction = transition_table[transition];

    if (direction > 0)
    {
      input_events.rot_r = 1U;
    }
    else if (direction < 0)
    {
      input_events.rot_l = 1U;
    }

    encoder->stable_ab = raw_ab;
  }

  if (raw_sw != encoder->last_raw_sw)
  {
    encoder->last_raw_sw = raw_sw;
    encoder->last_sw_change_ms = now;
  }
  else if ((now - encoder->last_sw_change_ms) >= ENCODER_SWITCH_DEBOUNCE_MS && raw_sw != encoder->stable_sw)
  {
    encoder->stable_sw = raw_sw;
    encoder->switch_pressed = (raw_sw == GPIO_PIN_RESET) ? 1U : 0U;

    if (encoder->switch_pressed != 0U)
    {
      input_events.rot_click = 1U;
    }
  }
}

static void Update_OLED_Menu_Button(void)
{
  uint32_t now = HAL_GetTick();
  GPIO_PinState raw_state = HAL_GPIO_ReadPin(OLED_MENU_BUTTON_PORT, OLED_MENU_BUTTON_PIN);

  if (raw_state != oled_menu_button_last_raw_state)
  {
    oled_menu_button_last_raw_state = raw_state;
    oled_menu_button_last_change_ms = now;
  }
  else if ((now - oled_menu_button_last_change_ms) >= OLED_MENU_BUTTON_DEBOUNCE_MS &&
           raw_state != oled_menu_button_stable_state)
  {
    oled_menu_button_stable_state = raw_state;

    if (raw_state == GPIO_PIN_RESET)
    {
      input_events.oled_menu_button = 1U;
    }
  }
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_ADC2_Init();
  /* USER CODE BEGIN 2 */
  if (HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_ADCEx_Calibration_Start(&hadc2, ADC_SINGLE_ENDED) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc1_values, ADC1_BUTTON_COUNT) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_ADC_Start_DMA(&hadc2, (uint32_t *)adc2_values, ADC2_BUTTON_COUNT) != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    Update_All_Buttons();
    Update_Rotary_Encoder(&rotary_encoder);
    Update_OLED_Menu_Button();
    HAL_Delay(1);
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_MultiModeTypeDef multimode = {0};
  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.GainCompensation = 0;
  hadc1.Init.ScanConvMode = ADC_SCAN_ENABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SEQ_CONV;
  hadc1.Init.LowPowerAutoWait = DISABLE;
  hadc1.Init.ContinuousConvMode = ENABLE;
  hadc1.Init.NbrOfConversion = ADC1_BUTTON_COUNT;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.DMAContinuousRequests = ENABLE;
  hadc1.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
  hadc1.Init.OversamplingMode = DISABLE;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure the ADC multi-mode
  */
  multimode.Mode = ADC_MODE_INDEPENDENT;
  if (HAL_ADCEx_MultiModeConfigChannel(&hadc1, &multimode) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_1;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_2CYCLES_5;
  sConfig.SingleDiff = ADC_SINGLE_ENDED;
  sConfig.OffsetNumber = ADC_OFFSET_NONE;
  sConfig.Offset = 0;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  sConfig.Channel = ADC_CHANNEL_2;
  sConfig.Rank = ADC_REGULAR_RANK_2;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  sConfig.Channel = ADC_CHANNEL_3;
  sConfig.Rank = ADC_REGULAR_RANK_3;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  sConfig.Channel = ADC_CHANNEL_4;
  sConfig.Rank = ADC_REGULAR_RANK_4;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  sConfig.Channel = ADC_CHANNEL_6;
  sConfig.Rank = ADC_REGULAR_RANK_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  sConfig.Channel = ADC_CHANNEL_7;
  sConfig.Rank = ADC_REGULAR_RANK_6;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  sConfig.Channel = ADC_CHANNEL_8;
  sConfig.Rank = ADC_REGULAR_RANK_7;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  sConfig.Channel = ADC_CHANNEL_9;
  sConfig.Rank = ADC_REGULAR_RANK_8;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief ADC2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC2_Init(void)
{

  /* USER CODE BEGIN ADC2_Init 0 */

  /* USER CODE END ADC2_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC2_Init 1 */

  /* USER CODE END ADC2_Init 1 */

  /** Common config
  */
  hadc2.Instance = ADC2;
  hadc2.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
  hadc2.Init.Resolution = ADC_RESOLUTION_12B;
  hadc2.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc2.Init.GainCompensation = 0;
  hadc2.Init.ScanConvMode = ADC_SCAN_ENABLE;
  hadc2.Init.EOCSelection = ADC_EOC_SEQ_CONV;
  hadc2.Init.LowPowerAutoWait = DISABLE;
  hadc2.Init.ContinuousConvMode = ENABLE;
  hadc2.Init.NbrOfConversion = ADC2_BUTTON_COUNT;
  hadc2.Init.DiscontinuousConvMode = DISABLE;
  hadc2.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc2.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc2.Init.DMAContinuousRequests = ENABLE;
  hadc2.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
  hadc2.Init.OversamplingMode = DISABLE;
  if (HAL_ADC_Init(&hadc2) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_17;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_2CYCLES_5;
  sConfig.SingleDiff = ADC_SINGLE_ENDED;
  sConfig.OffsetNumber = ADC_OFFSET_NONE;
  sConfig.Offset = 0;
  if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  sConfig.Channel = ADC_CHANNEL_13;
  sConfig.Rank = ADC_REGULAR_RANK_2;
  if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  sConfig.Channel = ADC_CHANNEL_3;
  sConfig.Rank = ADC_REGULAR_RANK_3;
  if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  sConfig.Channel = ADC_CHANNEL_4;
  sConfig.Rank = ADC_REGULAR_RANK_4;
  if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  sConfig.Channel = ADC_CHANNEL_5;
  sConfig.Rank = ADC_REGULAR_RANK_5;
  if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  sConfig.Channel = ADC_CHANNEL_11;
  sConfig.Rank = ADC_REGULAR_RANK_6;
  if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC2_Init 2 */

  /* USER CODE END ADC2_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMAMUX1_CLK_ENABLE();
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
  /* DMA1_Channel2_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel2_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_3
                          |GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7
                          |GPIO_PIN_9, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11, GPIO_PIN_RESET);

  /*Configure GPIO pins : PB0 PB1 PB2 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PB10 PB11 PB12 PB3
                           PB4 PB5 PB6 PB7
                           PB9 */
  GPIO_InitStruct.Pin = GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_3
                          |GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7
                          |GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PB13 PB15 */
  GPIO_InitStruct.Pin = GPIO_PIN_13|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PC6 PC7 PC8 PC9 */
  GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PA8 PA9 PA10 PA11 */
  GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PA12 */
  GPIO_InitStruct.Pin = GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
