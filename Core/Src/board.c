#include "board.h"

static ADC_HandleTypeDef hall_adc1;
static ADC_HandleTypeDef hall_adc2;
static DMA_HandleTypeDef hall_adc1_dma;
static DMA_HandleTypeDef hall_adc2_dma;

static uint16_t hall_adc1_samples[BOARD_HALL_ADC1_SAMPLE_COUNT];
static uint16_t hall_adc2_samples[BOARD_HALL_ADC2_SAMPLE_COUNT];

static void Board_SystemClock_Config(void);
static void Board_GPIO_Init(void);
static void Board_DMA_Init(void);
static void Board_ADC1_Init(void);
static void Board_ADC2_Init(void);
static void Board_ConfigADCChannel(ADC_HandleTypeDef *hadc, uint32_t channel, uint32_t rank);
static void Board_CalibrateADCs(void);
static void Board_StartHallAdcDma(void);

void Board_Init(void)
{
  HAL_Init();
  Board_SystemClock_Config();
  Board_GPIO_Init();
  Board_ADC1_Init();
  Board_ADC2_Init();
  Board_DMA_Init();
  Board_CalibrateADCs();
  Board_StartHallAdcDma();
}

const volatile uint16_t *Board_GetHallAdc1Samples(void)
{
  return hall_adc1_samples;
}

const volatile uint16_t *Board_GetHallAdc2Samples(void)
{
  return hall_adc2_samples;
}

void Board_HallAdc1DmaIrqHandler(void)
{
  HAL_DMA_IRQHandler(&hall_adc1_dma);
}

void Board_HallAdc2DmaIrqHandler(void)
{
  HAL_DMA_IRQHandler(&hall_adc2_dma);
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

  hall_adc1.Instance = ADC1;
  hall_adc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
  hall_adc1.Init.Resolution = ADC_RESOLUTION_12B;
  hall_adc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hall_adc1.Init.GainCompensation = 0;
  hall_adc1.Init.ScanConvMode = ADC_SCAN_ENABLE;
  hall_adc1.Init.EOCSelection = ADC_EOC_SEQ_CONV;
  hall_adc1.Init.LowPowerAutoWait = DISABLE;
  hall_adc1.Init.ContinuousConvMode = ENABLE;
  hall_adc1.Init.NbrOfConversion = BOARD_HALL_ADC1_SAMPLE_COUNT;
  hall_adc1.Init.DiscontinuousConvMode = DISABLE;
  hall_adc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hall_adc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hall_adc1.Init.DMAContinuousRequests = ENABLE;
  hall_adc1.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
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

  Board_ConfigADCChannel(&hall_adc1, ADC_CHANNEL_1, ADC_REGULAR_RANK_1);
  Board_ConfigADCChannel(&hall_adc1, ADC_CHANNEL_2, ADC_REGULAR_RANK_2);
  Board_ConfigADCChannel(&hall_adc1, ADC_CHANNEL_3, ADC_REGULAR_RANK_3);
  Board_ConfigADCChannel(&hall_adc1, ADC_CHANNEL_4, ADC_REGULAR_RANK_4);
  Board_ConfigADCChannel(&hall_adc1, ADC_CHANNEL_6, ADC_REGULAR_RANK_5);
  Board_ConfigADCChannel(&hall_adc1, ADC_CHANNEL_7, ADC_REGULAR_RANK_6);
  Board_ConfigADCChannel(&hall_adc1, ADC_CHANNEL_8, ADC_REGULAR_RANK_7);
  Board_ConfigADCChannel(&hall_adc1, ADC_CHANNEL_9, ADC_REGULAR_RANK_8);
}

static void Board_ADC2_Init(void)
{
  hall_adc2.Instance = ADC2;
  hall_adc2.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
  hall_adc2.Init.Resolution = ADC_RESOLUTION_12B;
  hall_adc2.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hall_adc2.Init.GainCompensation = 0;
  hall_adc2.Init.ScanConvMode = ADC_SCAN_ENABLE;
  hall_adc2.Init.EOCSelection = ADC_EOC_SEQ_CONV;
  hall_adc2.Init.LowPowerAutoWait = DISABLE;
  hall_adc2.Init.ContinuousConvMode = ENABLE;
  hall_adc2.Init.NbrOfConversion = BOARD_HALL_ADC2_SAMPLE_COUNT;
  hall_adc2.Init.DiscontinuousConvMode = DISABLE;
  hall_adc2.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hall_adc2.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hall_adc2.Init.DMAContinuousRequests = ENABLE;
  hall_adc2.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
  hall_adc2.Init.OversamplingMode = DISABLE;
  if (HAL_ADC_Init(&hall_adc2) != HAL_OK)
  {
    Error_Handler();
  }

  Board_ConfigADCChannel(&hall_adc2, ADC_CHANNEL_17, ADC_REGULAR_RANK_1);
  Board_ConfigADCChannel(&hall_adc2, ADC_CHANNEL_13, ADC_REGULAR_RANK_2);
  Board_ConfigADCChannel(&hall_adc2, ADC_CHANNEL_3, ADC_REGULAR_RANK_3);
  Board_ConfigADCChannel(&hall_adc2, ADC_CHANNEL_4, ADC_REGULAR_RANK_4);
  Board_ConfigADCChannel(&hall_adc2, ADC_CHANNEL_5, ADC_REGULAR_RANK_5);
  Board_ConfigADCChannel(&hall_adc2, ADC_CHANNEL_11, ADC_REGULAR_RANK_6);
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

static void Board_DMA_Init(void)
{
  __HAL_RCC_DMAMUX1_CLK_ENABLE();
  __HAL_RCC_DMA1_CLK_ENABLE();

  hall_adc1_dma.Instance = DMA1_Channel1;
  hall_adc1_dma.Init.Request = DMA_REQUEST_ADC1;
  hall_adc1_dma.Init.Direction = DMA_PERIPH_TO_MEMORY;
  hall_adc1_dma.Init.PeriphInc = DMA_PINC_DISABLE;
  hall_adc1_dma.Init.MemInc = DMA_MINC_ENABLE;
  hall_adc1_dma.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
  hall_adc1_dma.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
  hall_adc1_dma.Init.Mode = DMA_CIRCULAR;
  hall_adc1_dma.Init.Priority = DMA_PRIORITY_HIGH;
  if (HAL_DMA_Init(&hall_adc1_dma) != HAL_OK)
  {
    Error_Handler();
  }
  __HAL_LINKDMA(&hall_adc1, DMA_Handle, hall_adc1_dma);

  hall_adc2_dma.Instance = DMA1_Channel2;
  hall_adc2_dma.Init.Request = DMA_REQUEST_ADC2;
  hall_adc2_dma.Init.Direction = DMA_PERIPH_TO_MEMORY;
  hall_adc2_dma.Init.PeriphInc = DMA_PINC_DISABLE;
  hall_adc2_dma.Init.MemInc = DMA_MINC_ENABLE;
  hall_adc2_dma.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
  hall_adc2_dma.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
  hall_adc2_dma.Init.Mode = DMA_CIRCULAR;
  hall_adc2_dma.Init.Priority = DMA_PRIORITY_HIGH;
  if (HAL_DMA_Init(&hall_adc2_dma) != HAL_OK)
  {
    Error_Handler();
  }
  __HAL_LINKDMA(&hall_adc2, DMA_Handle, hall_adc2_dma);

  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
  HAL_NVIC_SetPriority(DMA1_Channel2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel2_IRQn);
}

static void Board_ConfigADCChannel(ADC_HandleTypeDef *hadc, uint32_t channel, uint32_t rank)
{
  ADC_ChannelConfTypeDef sConfig = {0};

  sConfig.Channel = channel;
  sConfig.Rank = rank;
  sConfig.SamplingTime = ADC_SAMPLETIME_2CYCLES_5;
  sConfig.SingleDiff = ADC_SINGLE_ENDED;
  sConfig.OffsetNumber = ADC_OFFSET_NONE;
  sConfig.Offset = 0;

  if (HAL_ADC_ConfigChannel(hadc, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
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

static void Board_StartHallAdcDma(void)
{
  if (HAL_ADC_Start_DMA(&hall_adc1, (uint32_t *)hall_adc1_samples, BOARD_HALL_ADC1_SAMPLE_COUNT) != HAL_OK)
  {
    Error_Handler();
  }
  __HAL_DMA_DISABLE_IT(&hall_adc1_dma, DMA_IT_HT | DMA_IT_TC);

  if (HAL_ADC_Start_DMA(&hall_adc2, (uint32_t *)hall_adc2_samples, BOARD_HALL_ADC2_SAMPLE_COUNT) != HAL_OK)
  {
    Error_Handler();
  }
  __HAL_DMA_DISABLE_IT(&hall_adc2_dma, DMA_IT_HT | DMA_IT_TC);
}
