#include "board.h"

static ADC_HandleTypeDef hall_adc1;
static ADC_HandleTypeDef hall_adc2;

static void Board_SystemClock_Config(void);
static void Board_GPIO_Init(void);
static void Board_ADC1_Init(void);
static void Board_ADC2_Init(void);
static void Board_CalibrateADCs(void);

void Board_Init(void)
{
  HAL_Init();
  Board_SystemClock_Config();
  Board_GPIO_Init();
  Board_ADC1_Init();
  Board_ADC2_Init();
  Board_CalibrateADCs();
}

ADC_HandleTypeDef *Board_GetHallAdc1(void)
{
  return &hall_adc1;
}

ADC_HandleTypeDef *Board_GetHallAdc2(void)
{
  return &hall_adc2;
}

static void Board_SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

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

static void Board_ADC1_Init(void)
{
  ADC_MultiModeTypeDef multimode = {0};
  ADC_ChannelConfTypeDef sConfig = {0};

  hall_adc1.Instance = ADC1;
  hall_adc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
  hall_adc1.Init.Resolution = ADC_RESOLUTION_12B;
  hall_adc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hall_adc1.Init.GainCompensation = 0;
  hall_adc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hall_adc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hall_adc1.Init.LowPowerAutoWait = DISABLE;
  hall_adc1.Init.ContinuousConvMode = DISABLE;
  hall_adc1.Init.NbrOfConversion = 1;
  hall_adc1.Init.DiscontinuousConvMode = DISABLE;
  hall_adc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hall_adc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hall_adc1.Init.DMAContinuousRequests = DISABLE;
  hall_adc1.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  hall_adc1.Init.OversamplingMode = DISABLE;
  if (HAL_ADC_Init(&hall_adc1) != HAL_OK)
  {
    Error_Handler();
  }

  multimode.Mode = ADC_MODE_INDEPENDENT;
  if (HAL_ADCEx_MultiModeConfigChannel(&hall_adc1, &multimode) != HAL_OK)
  {
    Error_Handler();
  }

  sConfig.Channel = ADC_CHANNEL_1;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_2CYCLES_5;
  sConfig.SingleDiff = ADC_SINGLE_ENDED;
  sConfig.OffsetNumber = ADC_OFFSET_NONE;
  sConfig.Offset = 0;
  if (HAL_ADC_ConfigChannel(&hall_adc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
}

static void Board_ADC2_Init(void)
{
  ADC_ChannelConfTypeDef sConfig = {0};

  hall_adc2.Instance = ADC2;
  hall_adc2.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
  hall_adc2.Init.Resolution = ADC_RESOLUTION_12B;
  hall_adc2.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hall_adc2.Init.GainCompensation = 0;
  hall_adc2.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hall_adc2.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hall_adc2.Init.LowPowerAutoWait = DISABLE;
  hall_adc2.Init.ContinuousConvMode = DISABLE;
  hall_adc2.Init.NbrOfConversion = 1;
  hall_adc2.Init.DiscontinuousConvMode = DISABLE;
  hall_adc2.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hall_adc2.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hall_adc2.Init.DMAContinuousRequests = DISABLE;
  hall_adc2.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  hall_adc2.Init.OversamplingMode = DISABLE;
  if (HAL_ADC_Init(&hall_adc2) != HAL_OK)
  {
    Error_Handler();
  }

  sConfig.Channel = ADC_CHANNEL_17;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_2CYCLES_5;
  sConfig.SingleDiff = ADC_SINGLE_ENDED;
  sConfig.OffsetNumber = ADC_OFFSET_NONE;
  sConfig.Offset = 0;
  if (HAL_ADC_ConfigChannel(&hall_adc2, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
}

static void Board_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_3
                          |GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7
                          |GPIO_PIN_9, GPIO_PIN_RESET);

  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9, GPIO_PIN_RESET);

  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11, GPIO_PIN_RESET);

  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_3
                          |GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7
                          |GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_13|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

static void Board_CalibrateADCs(void)
{
  if (HAL_ADCEx_Calibration_Start(&hall_adc1, ADC_SINGLE_ENDED) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_ADCEx_Calibration_Start(&hall_adc2, ADC_SINGLE_ENDED) != HAL_OK)
  {
    Error_Handler();
  }
}
