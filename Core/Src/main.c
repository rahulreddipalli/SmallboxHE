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
  char symbol;
  uint8_t columns[5];
} FontGlyph;

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

typedef enum
{
  UI_SCREEN_HOME = 0,
  UI_SCREEN_SETTINGS,
  UI_SCREEN_CALIBRATION
} UIScreen;

typedef enum
{
  SETTINGS_ITEM_RAPID_TRIGGER = 0,
  SETTINGS_ITEM_ACTUATION,
  SETTINGS_ITEM_ADVANCED,
  SETTINGS_ITEM_CALIBRATION,
  SETTINGS_ITEM_SAVE,
  SETTINGS_ITEM_COUNT
} SettingsItem;

typedef enum
{
  ADVANCED_ITEM_HYSTERESIS = 0,
  ADVANCED_ITEM_RAW_ADC,
  ADVANCED_ITEM_COUNT
} AdvancedItem;

typedef enum
{
  CALIBRATION_STEP_SELECT_BUTTON = 0,
  CALIBRATION_STEP_CAPTURE_MIN,
  CALIBRATION_STEP_CAPTURE_MAX
} CalibrationStep;

typedef struct
{
  uint8_t rapid_trigger_enabled;
  uint8_t actuation_tenths_mm;
  uint8_t hysteresis_tenths_mm;
  uint8_t show_raw_adc;
} ControllerSettings;

typedef struct
{
  uint16_t min_adc;
  uint16_t max_adc;
} ButtonCalibration;

typedef struct
{
  UIScreen screen;
  uint8_t selected_item;
  uint8_t selected_advanced_item;
  uint8_t editing_value;
  uint8_t advanced_open;
  uint8_t selected_button;
  CalibrationStep calibration_step;
} UIState;

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
#define SWITCH_TRAVEL_TENTHS_MM 35U
#define ACTUATION_MIN_TENTHS_MM 1U
#define ACTUATION_MAX_TENTHS_MM SWITCH_TRAVEL_TENTHS_MM
#define DEFAULT_ACTUATION_TENTHS_MM 12U
#define DEFAULT_HYSTERESIS_TENTHS_MM 1U
#define HYSTERESIS_MIN_TENTHS_MM 1U
#define HYSTERESIS_MAX_TENTHS_MM 10U

#define OLED_MENU_BUTTON_PORT GPIOA
#define OLED_MENU_BUTTON_PIN  GPIO_PIN_12

#define OLED_DC_PORT    GPIOB
#define OLED_DC_PIN     GPIO_PIN_10
#define OLED_RST_PORT   GPIOB
#define OLED_RST_PIN    GPIO_PIN_11
#define OLED_CS_PORT    GPIOB
#define OLED_CS_PIN     GPIO_PIN_12
#define OLED_SCK_PORT   GPIOB
#define OLED_SCK_PIN    GPIO_PIN_13
#define OLED_MOSI_PORT  GPIOB
#define OLED_MOSI_PIN   GPIO_PIN_15
#define OLED_WIDTH_PIXELS 256U
#define OLED_HEIGHT_PIXELS 64U
#define OLED_BUFFER_SIZE ((OLED_WIDTH_PIXELS * OLED_HEIGHT_PIXELS) / 2U)
#define OLED_TEXT_COLOR 15U
#define OLED_DIM_COLOR 5U
#define OLED_REFRESH_MS 50U

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
static uint8_t oled_buffer[OLED_BUFFER_SIZE];

static const FontGlyph font_glyphs[] =
{
  { ' ', { 0x00, 0x00, 0x00, 0x00, 0x00 } },
  { '<', { 0x08, 0x14, 0x22, 0x41, 0x00 } },
  { '-', { 0x08, 0x08, 0x08, 0x08, 0x08 } },
  { '.', { 0x00, 0x60, 0x60, 0x00, 0x00 } },
  { ':', { 0x00, 0x36, 0x36, 0x00, 0x00 } },
  { '/', { 0x20, 0x10, 0x08, 0x04, 0x02 } },
  { '%', { 0x62, 0x64, 0x08, 0x13, 0x23 } },
  { '>', { 0x41, 0x22, 0x14, 0x08, 0x00 } },
  { '0', { 0x3E, 0x51, 0x49, 0x45, 0x3E } },
  { '1', { 0x00, 0x42, 0x7F, 0x40, 0x00 } },
  { '2', { 0x42, 0x61, 0x51, 0x49, 0x46 } },
  { '3', { 0x21, 0x41, 0x45, 0x4B, 0x31 } },
  { '4', { 0x18, 0x14, 0x12, 0x7F, 0x10 } },
  { '5', { 0x27, 0x45, 0x45, 0x45, 0x39 } },
  { '6', { 0x3C, 0x4A, 0x49, 0x49, 0x30 } },
  { '7', { 0x01, 0x71, 0x09, 0x05, 0x03 } },
  { '8', { 0x36, 0x49, 0x49, 0x49, 0x36 } },
  { '9', { 0x06, 0x49, 0x49, 0x29, 0x1E } },
  { 'A', { 0x7E, 0x11, 0x11, 0x11, 0x7E } },
  { 'B', { 0x7F, 0x49, 0x49, 0x49, 0x36 } },
  { 'C', { 0x3E, 0x41, 0x41, 0x41, 0x22 } },
  { 'D', { 0x7F, 0x41, 0x41, 0x22, 0x1C } },
  { 'E', { 0x7F, 0x49, 0x49, 0x49, 0x41 } },
  { 'F', { 0x7F, 0x09, 0x09, 0x09, 0x01 } },
  { 'G', { 0x3E, 0x41, 0x49, 0x49, 0x7A } },
  { 'H', { 0x7F, 0x08, 0x08, 0x08, 0x7F } },
  { 'I', { 0x00, 0x41, 0x7F, 0x41, 0x00 } },
  { 'J', { 0x20, 0x40, 0x41, 0x3F, 0x01 } },
  { 'K', { 0x7F, 0x08, 0x14, 0x22, 0x41 } },
  { 'L', { 0x7F, 0x40, 0x40, 0x40, 0x40 } },
  { 'M', { 0x7F, 0x02, 0x0C, 0x02, 0x7F } },
  { 'N', { 0x7F, 0x04, 0x08, 0x10, 0x7F } },
  { 'O', { 0x3E, 0x41, 0x41, 0x41, 0x3E } },
  { 'P', { 0x7F, 0x09, 0x09, 0x09, 0x06 } },
  { 'Q', { 0x3E, 0x41, 0x51, 0x21, 0x5E } },
  { 'R', { 0x7F, 0x09, 0x19, 0x29, 0x46 } },
  { 'S', { 0x46, 0x49, 0x49, 0x49, 0x31 } },
  { 'T', { 0x01, 0x01, 0x7F, 0x01, 0x01 } },
  { 'U', { 0x3F, 0x40, 0x40, 0x40, 0x3F } },
  { 'V', { 0x1F, 0x20, 0x40, 0x20, 0x1F } },
  { 'W', { 0x3F, 0x40, 0x38, 0x40, 0x3F } },
  { 'X', { 0x63, 0x14, 0x08, 0x14, 0x63 } },
  { 'Y', { 0x07, 0x08, 0x70, 0x08, 0x07 } },
  { 'Z', { 0x61, 0x51, 0x49, 0x45, 0x43 } }
};

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
static ControllerSettings controller_settings =
{
  1U,
  DEFAULT_ACTUATION_TENTHS_MM,
  DEFAULT_HYSTERESIS_TENTHS_MM,
  0U
};
static ButtonCalibration button_calibration[BUTTON_COUNT];
static UIState ui_state =
{
  UI_SCREEN_HOME,
  SETTINGS_ITEM_RAPID_TRIGGER,
  ADVANCED_ITEM_HYSTERESIS,
  0U,
  0U,
  0U,
  CALIBRATION_STEP_SELECT_BUTTON
};

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
static void Initialize_Button_Calibration(void)
{
  for (uint32_t i = 0U; i < BUTTON_COUNT; i++)
  {
    button_calibration[i].min_adc = 0U;
    button_calibration[i].max_adc = 4095U;
  }
}

static void Apply_Settings_To_Button(ButtonChannel *button, const ButtonCalibration *calibration)
{
  uint32_t low_adc = calibration->min_adc;
  uint32_t high_adc = calibration->max_adc;
  uint32_t range_adc;
  uint32_t actuation_offset;
  uint32_t hysteresis_offset;

  if (high_adc < low_adc)
  {
    uint32_t temp = low_adc;

    low_adc = high_adc;
    high_adc = temp;
  }

  range_adc = high_adc - low_adc;
  actuation_offset = (range_adc * controller_settings.actuation_tenths_mm) / SWITCH_TRAVEL_TENTHS_MM;
  hysteresis_offset = (range_adc * controller_settings.hysteresis_tenths_mm) / SWITCH_TRAVEL_TENTHS_MM;

  button->press_threshold = low_adc + actuation_offset;

  if (button->press_threshold > (low_adc + hysteresis_offset))
  {
    button->release_threshold = button->press_threshold - hysteresis_offset;
  }
  else
  {
    button->release_threshold = low_adc;
  }
}

static void Apply_Controller_Settings(void)
{
  for (uint32_t i = 0U; i < BUTTON_COUNT; i++)
  {
    Apply_Settings_To_Button(&buttons[i], &button_calibration[i]);
  }
}

static void Save_Controller_Settings(void)
{
  /* TODO: Persist settings/calibration to flash when storage layout is decided. */
}

static void Clear_Input_Events(void)
{
  input_events.rot_l = 0U;
  input_events.rot_r = 0U;
  input_events.rot_click = 0U;
  input_events.oled_menu_button = 0U;
}

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

static void OLED_Write_Byte(uint8_t value)
{
  for (uint8_t bit = 0U; bit < 8U; bit++)
  {
    HAL_GPIO_WritePin(OLED_SCK_PORT, OLED_SCK_PIN, GPIO_PIN_RESET);

    if ((value & 0x80U) != 0U)
    {
      HAL_GPIO_WritePin(OLED_MOSI_PORT, OLED_MOSI_PIN, GPIO_PIN_SET);
    }
    else
    {
      HAL_GPIO_WritePin(OLED_MOSI_PORT, OLED_MOSI_PIN, GPIO_PIN_RESET);
    }

    HAL_GPIO_WritePin(OLED_SCK_PORT, OLED_SCK_PIN, GPIO_PIN_SET);
    value <<= 1U;
  }
}

static void OLED_Write_Command(uint8_t command)
{
  HAL_GPIO_WritePin(OLED_CS_PORT, OLED_CS_PIN, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(OLED_DC_PORT, OLED_DC_PIN, GPIO_PIN_RESET);
  OLED_Write_Byte(command);
  HAL_GPIO_WritePin(OLED_CS_PORT, OLED_CS_PIN, GPIO_PIN_SET);
}

static void OLED_Write_Data(const uint8_t *data, uint32_t length)
{
  HAL_GPIO_WritePin(OLED_CS_PORT, OLED_CS_PIN, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(OLED_DC_PORT, OLED_DC_PIN, GPIO_PIN_SET);

  for (uint32_t i = 0U; i < length; i++)
  {
    OLED_Write_Byte(data[i]);
  }

  HAL_GPIO_WritePin(OLED_CS_PORT, OLED_CS_PIN, GPIO_PIN_SET);
}

static void OLED_Reset(void)
{
  HAL_GPIO_WritePin(OLED_CS_PORT, OLED_CS_PIN, GPIO_PIN_SET);
  HAL_GPIO_WritePin(OLED_SCK_PORT, OLED_SCK_PIN, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(OLED_MOSI_PORT, OLED_MOSI_PIN, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(OLED_RST_PORT, OLED_RST_PIN, GPIO_PIN_RESET);
  HAL_Delay(20);
  HAL_GPIO_WritePin(OLED_RST_PORT, OLED_RST_PIN, GPIO_PIN_SET);
  HAL_Delay(20);
}

static void OLED_Init(void)
{
  OLED_Reset();

  OLED_Write_Command(0xAE); /* Display off */
  OLED_Write_Command(0xA0); /* Segment remap */
  OLED_Write_Command(0xC0); /* Common scan direction */
  OLED_Write_Command(0x40); /* Display start line */
  OLED_Write_Command(0xD3); /* Display offset */
  OLED_Write_Command(0x00);
  OLED_Write_Command(0xA4); /* Normal display from RAM */
  OLED_Write_Command(0xA6); /* Normal, not inverted */
  OLED_Write_Command(0xA8); /* Multiplex ratio */
  OLED_Write_Command(0x3F);
  OLED_Write_Command(0xAD); /* Internal DC-DC */
  OLED_Write_Command(0x81);
  OLED_Write_Command(0x81); /* Contrast */
  OLED_Write_Command(0x80);
  OLED_Write_Command(0xD5); /* Display clock */
  OLED_Write_Command(0x50);
  OLED_Write_Command(0xD9); /* Discharge/precharge period */
  OLED_Write_Command(0x22);
  OLED_Write_Command(0xBE); /* VCOMH */
  OLED_Write_Command(0x35);
  OLED_Write_Command(0xDC); /* VSEGM */
  OLED_Write_Command(0x35);
  OLED_Write_Command(0xAF); /* Display on */
}

static void OLED_Clear_Buffer(uint8_t color)
{
  uint8_t packed_color = (uint8_t)(((color & 0x0FU) << 4U) | (color & 0x0FU));

  for (uint32_t i = 0U; i < OLED_BUFFER_SIZE; i++)
  {
    oled_buffer[i] = packed_color;
  }
}

static void OLED_Set_Pixel(uint16_t x, uint16_t y, uint8_t color)
{
  uint32_t index;

  if (x >= OLED_WIDTH_PIXELS || y >= OLED_HEIGHT_PIXELS)
  {
    return;
  }

  index = ((uint32_t)y * (OLED_WIDTH_PIXELS / 2U)) + (x / 2U);

  if ((x & 1U) == 0U)
  {
    oled_buffer[index] = (uint8_t)((oled_buffer[index] & 0x0FU) | ((color & 0x0FU) << 4U));
  }
  else
  {
    oled_buffer[index] = (uint8_t)((oled_buffer[index] & 0xF0U) | (color & 0x0FU));
  }
}

static const uint8_t *OLED_Find_Glyph(char symbol)
{
  for (uint32_t i = 0U; i < (sizeof(font_glyphs) / sizeof(font_glyphs[0])); i++)
  {
    if (font_glyphs[i].symbol == symbol)
    {
      return font_glyphs[i].columns;
    }
  }

  return font_glyphs[0].columns;
}

static void OLED_Draw_Char(uint16_t x, uint16_t y, char symbol, uint8_t color, uint8_t scale)
{
  const uint8_t *glyph = OLED_Find_Glyph(symbol);

  for (uint8_t column = 0U; column < 5U; column++)
  {
    for (uint8_t row = 0U; row < 7U; row++)
    {
      if ((glyph[column] & (1U << row)) != 0U)
      {
        for (uint8_t sx = 0U; sx < scale; sx++)
        {
          for (uint8_t sy = 0U; sy < scale; sy++)
          {
            OLED_Set_Pixel((uint16_t)(x + (column * scale) + sx),
                           (uint16_t)(y + (row * scale) + sy),
                           color);
          }
        }
      }
    }
  }
}

static void OLED_Draw_Text(uint16_t x, uint16_t y, const char *text, uint8_t color, uint8_t scale)
{
  uint16_t cursor_x = x;

  while (*text != '\0')
  {
    OLED_Draw_Char(cursor_x, y, *text, color, scale);
    cursor_x = (uint16_t)(cursor_x + (6U * scale));
    text++;
  }
}

static void OLED_Update(void)
{
  for (uint8_t row = 0U; row < OLED_HEIGHT_PIXELS; row++)
  {
    OLED_Write_Command(0xB0);
    OLED_Write_Command(row);
    OLED_Write_Command(0x00);
    OLED_Write_Command(0x10);
    OLED_Write_Data(&oled_buffer[(uint32_t)row * (OLED_WIDTH_PIXELS / 2U)], OLED_WIDTH_PIXELS / 2U);
  }
}

static void Copy_Text(char *destination, const char *source)
{
  while (*source != '\0')
  {
    *destination = *source;
    destination++;
    source++;
  }

  *destination = '\0';
}

static void Append_Text(char *destination, const char *source)
{
  while (*destination != '\0')
  {
    destination++;
  }

  Copy_Text(destination, source);
}

static void Format_Tenths_Mm(uint8_t tenths_mm, char *text)
{
  text[0] = (char)('0' + (tenths_mm / 10U));
  text[1] = '.';
  text[2] = (char)('0' + (tenths_mm % 10U));
  text[3] = 'M';
  text[4] = 'M';
  text[5] = '\0';
}

static void Format_U16(uint16_t value, char *text)
{
  char reversed[5];
  uint8_t count = 0U;

  if (value == 0U)
  {
    text[0] = '0';
    text[1] = '\0';
    return;
  }

  while (value != 0U && count < sizeof(reversed))
  {
    reversed[count] = (char)('0' + (value % 10U));
    value = (uint16_t)(value / 10U);
    count++;
  }

  for (uint8_t i = 0U; i < count; i++)
  {
    text[i] = reversed[count - 1U - i];
  }

  text[count] = '\0';
}

static void Draw_Menu_Line(uint8_t row, uint8_t selected, const char *text, uint8_t editing)
{
  uint8_t color = (selected != 0U) ? OLED_TEXT_COLOR : OLED_DIM_COLOR;
  uint16_t y = (uint16_t)(14U + (row * 10U));

  OLED_Draw_Text(0U, y, (selected != 0U) ? ">" : " ", color, 1U);
  OLED_Draw_Text(12U, y, text, color, 1U);

  if (editing != 0U)
  {
    OLED_Draw_Text(238U, y, "<", color, 1U);
  }
}

static void Build_Settings_Label(SettingsItem item, char *text)
{
  char value_text[8];

  switch (item)
  {
    case SETTINGS_ITEM_RAPID_TRIGGER:
      Copy_Text(text, "RAPID TRIG:");
      Append_Text(text, (controller_settings.rapid_trigger_enabled != 0U) ? "ON" : "OFF");
      break;

    case SETTINGS_ITEM_ACTUATION:
      Copy_Text(text, "ACT:");
      Format_Tenths_Mm(controller_settings.actuation_tenths_mm, value_text);
      Append_Text(text, value_text);
      break;

    case SETTINGS_ITEM_ADVANCED:
      Copy_Text(text, "ADVANCED");
      break;

    case SETTINGS_ITEM_CALIBRATION:
      Copy_Text(text, "CALIBRATE");
      break;

    case SETTINGS_ITEM_SAVE:
      Copy_Text(text, "SAVE");
      break;

    default:
      Copy_Text(text, "");
      break;
  }
}

static void Build_Advanced_Label(AdvancedItem item, char *text)
{
  char value_text[8];

  switch (item)
  {
    case ADVANCED_ITEM_HYSTERESIS:
      Copy_Text(text, "HYST:");
      Format_Tenths_Mm(controller_settings.hysteresis_tenths_mm, value_text);
      Append_Text(text, value_text);
      break;

    case ADVANCED_ITEM_RAW_ADC:
      Copy_Text(text, "RAW ADC:");
      Append_Text(text, (controller_settings.show_raw_adc != 0U) ? "ON" : "OFF");
      break;

    default:
      Copy_Text(text, "");
      break;
  }
}

static void OLED_Render_Home(void)
{
  OLED_Clear_Buffer(0U);
  OLED_Draw_Text(104U, 26U, "HOME", OLED_TEXT_COLOR, 2U);
}

static void OLED_Render_Settings(void)
{
  char line_text[24];

  OLED_Clear_Buffer(0U);

  if (ui_state.advanced_open != 0U)
  {
    OLED_Draw_Text(0U, 0U, "ADVANCED", OLED_TEXT_COLOR, 1U);

    for (uint8_t i = 0U; i < ADVANCED_ITEM_COUNT; i++)
    {
      Build_Advanced_Label((AdvancedItem)i, line_text);
      Draw_Menu_Line(i, (i == ui_state.selected_advanced_item) ? 1U : 0U, line_text,
                     (ui_state.editing_value != 0U && i == ui_state.selected_advanced_item) ? 1U : 0U);
    }

    return;
  }

  OLED_Draw_Text(0U, 0U, "SETTINGS", OLED_TEXT_COLOR, 1U);

  for (uint8_t i = 0U; i < SETTINGS_ITEM_COUNT; i++)
  {
    Build_Settings_Label((SettingsItem)i, line_text);
    Draw_Menu_Line(i, (i == ui_state.selected_item) ? 1U : 0U, line_text,
                   (ui_state.editing_value != 0U && i == ui_state.selected_item) ? 1U : 0U);
  }
}

static void OLED_Render_Calibration(void)
{
  char value_text[8];
  char line_text[24];
  uint8_t selected_button = ui_state.selected_button;

  OLED_Clear_Buffer(0U);
  OLED_Draw_Text(0U, 0U, "CALIBRATE", OLED_TEXT_COLOR, 1U);

  Copy_Text(line_text, "BTN:");
  Append_Text(line_text, buttons[selected_button].name);
  OLED_Draw_Text(0U, 16U, line_text, OLED_TEXT_COLOR, 1U);

  Copy_Text(line_text, "ADC:");
  Format_U16((uint16_t)buttons[selected_button].adc_value, value_text);
  Append_Text(line_text, value_text);
  OLED_Draw_Text(0U, 28U, line_text, OLED_DIM_COLOR, 1U);

  if (ui_state.calibration_step == CALIBRATION_STEP_SELECT_BUTTON)
  {
    OLED_Draw_Text(0U, 44U, "CLICK TO START", OLED_TEXT_COLOR, 1U);
  }
  else if (ui_state.calibration_step == CALIBRATION_STEP_CAPTURE_MIN)
  {
    OLED_Draw_Text(0U, 44U, "RELEASE THEN CLICK", OLED_TEXT_COLOR, 1U);
  }
  else
  {
    OLED_Draw_Text(0U, 44U, "PRESS THEN CLICK", OLED_TEXT_COLOR, 1U);
  }
}

static void OLED_Render_UI(void)
{
  switch (ui_state.screen)
  {
    case UI_SCREEN_HOME:
      OLED_Render_Home();
      break;

    case UI_SCREEN_SETTINGS:
      OLED_Render_Settings();
      break;

    case UI_SCREEN_CALIBRATION:
      OLED_Render_Calibration();
      break;

    default:
      OLED_Render_Home();
      break;
  }

  OLED_Update();
}

static void OLED_Render_If_Due(void)
{
  static uint32_t last_oled_refresh_ms = 0U;
  uint32_t now = HAL_GetTick();

  if ((now - last_oled_refresh_ms) >= OLED_REFRESH_MS)
  {
    last_oled_refresh_ms = now;
    OLED_Render_UI();
  }
}

static void Move_Settings_Cursor(int8_t direction)
{
  if (direction > 0)
  {
    ui_state.selected_item = (uint8_t)((ui_state.selected_item + 1U) % SETTINGS_ITEM_COUNT);
  }
  else if (direction < 0)
  {
    if (ui_state.selected_item == 0U)
    {
      ui_state.selected_item = SETTINGS_ITEM_COUNT - 1U;
    }
    else
    {
      ui_state.selected_item--;
    }
  }
}

static void Move_Advanced_Cursor(int8_t direction)
{
  if (direction > 0)
  {
    ui_state.selected_advanced_item = (uint8_t)((ui_state.selected_advanced_item + 1U) % ADVANCED_ITEM_COUNT);
  }
  else if (direction < 0)
  {
    if (ui_state.selected_advanced_item == 0U)
    {
      ui_state.selected_advanced_item = ADVANCED_ITEM_COUNT - 1U;
    }
    else
    {
      ui_state.selected_advanced_item--;
    }
  }
}

static void Adjust_Actuation(int8_t direction)
{
  int16_t next_value = (int16_t)controller_settings.actuation_tenths_mm + direction;

  if (next_value < (int16_t)ACTUATION_MIN_TENTHS_MM)
  {
    next_value = ACTUATION_MIN_TENTHS_MM;
  }
  else if (next_value > (int16_t)ACTUATION_MAX_TENTHS_MM)
  {
    next_value = ACTUATION_MAX_TENTHS_MM;
  }

  controller_settings.actuation_tenths_mm = (uint8_t)next_value;
  Apply_Controller_Settings();
}

static void Adjust_Hysteresis(int8_t direction)
{
  int16_t next_value = (int16_t)controller_settings.hysteresis_tenths_mm + direction;

  if (next_value < (int16_t)HYSTERESIS_MIN_TENTHS_MM)
  {
    next_value = HYSTERESIS_MIN_TENTHS_MM;
  }
  else if (next_value > (int16_t)HYSTERESIS_MAX_TENTHS_MM)
  {
    next_value = HYSTERESIS_MAX_TENTHS_MM;
  }

  controller_settings.hysteresis_tenths_mm = (uint8_t)next_value;
  Apply_Controller_Settings();
}

static void Move_Calibration_Button(int8_t direction)
{
  if (direction > 0)
  {
    ui_state.selected_button = (uint8_t)((ui_state.selected_button + 1U) % BUTTON_COUNT);
  }
  else if (direction < 0)
  {
    if (ui_state.selected_button == 0U)
    {
      ui_state.selected_button = BUTTON_COUNT - 1U;
    }
    else
    {
      ui_state.selected_button--;
    }
  }
}

static void Process_Advanced_Settings_Input(const InputEvents *events, int8_t direction)
{
  if (events->oled_menu_button != 0U)
  {
    ui_state.advanced_open = 0U;
    ui_state.editing_value = 0U;
    return;
  }

  if (ui_state.editing_value != 0U)
  {
    if (direction != 0)
    {
      Adjust_Hysteresis(direction);
    }

    if (events->rot_click != 0U)
    {
      ui_state.editing_value = 0U;
    }

    return;
  }

  if (direction != 0)
  {
    Move_Advanced_Cursor(direction);
  }

  if (events->rot_click == 0U)
  {
    return;
  }

  switch ((AdvancedItem)ui_state.selected_advanced_item)
  {
    case ADVANCED_ITEM_HYSTERESIS:
      ui_state.editing_value = 1U;
      break;

    case ADVANCED_ITEM_RAW_ADC:
      controller_settings.show_raw_adc =
        (controller_settings.show_raw_adc == 0U) ? 1U : 0U;
      break;

    default:
      break;
  }
}

static void Process_Home_Input(const InputEvents *events)
{
  if (events->rot_click != 0U || events->oled_menu_button != 0U)
  {
    ui_state.screen = UI_SCREEN_SETTINGS;
    ui_state.selected_item = SETTINGS_ITEM_RAPID_TRIGGER;
    ui_state.editing_value = 0U;
    ui_state.advanced_open = 0U;
  }
}

static void Process_Settings_Input(const InputEvents *events)
{
  int8_t direction = 0;

  if (events->rot_r != 0U)
  {
    direction = 1;
  }
  else if (events->rot_l != 0U)
  {
    direction = -1;
  }

  if (ui_state.advanced_open != 0U)
  {
    Process_Advanced_Settings_Input(events, direction);
    return;
  }

  if (events->oled_menu_button != 0U)
  {
    ui_state.screen = UI_SCREEN_HOME;
    ui_state.editing_value = 0U;
    ui_state.advanced_open = 0U;
    return;
  }

  if (ui_state.editing_value != 0U)
  {
    if (direction != 0)
    {
      Adjust_Actuation(direction);
    }

    if (events->rot_click != 0U)
    {
      ui_state.editing_value = 0U;
    }

    return;
  }

  if (direction != 0)
  {
    Move_Settings_Cursor(direction);
  }

  if (events->rot_click == 0U)
  {
    return;
  }

  switch ((SettingsItem)ui_state.selected_item)
  {
    case SETTINGS_ITEM_RAPID_TRIGGER:
      controller_settings.rapid_trigger_enabled =
        (controller_settings.rapid_trigger_enabled == 0U) ? 1U : 0U;
      break;

    case SETTINGS_ITEM_ACTUATION:
      ui_state.editing_value = 1U;
      break;

    case SETTINGS_ITEM_ADVANCED:
      ui_state.advanced_open = 1U;
      ui_state.selected_advanced_item = ADVANCED_ITEM_HYSTERESIS;
      ui_state.editing_value = 0U;
      break;

    case SETTINGS_ITEM_CALIBRATION:
      ui_state.screen = UI_SCREEN_CALIBRATION;
      ui_state.selected_button = 0U;
      ui_state.calibration_step = CALIBRATION_STEP_SELECT_BUTTON;
      break;

    case SETTINGS_ITEM_SAVE:
      Save_Controller_Settings();
      break;

    default:
      break;
  }
}

static void Process_Calibration_Input(const InputEvents *events)
{
  uint8_t selected_button = ui_state.selected_button;

  if (events->oled_menu_button != 0U)
  {
    ui_state.screen = UI_SCREEN_SETTINGS;
    ui_state.calibration_step = CALIBRATION_STEP_SELECT_BUTTON;
    ui_state.advanced_open = 0U;
    return;
  }

  if (ui_state.calibration_step == CALIBRATION_STEP_SELECT_BUTTON)
  {
    if (events->rot_r != 0U)
    {
      Move_Calibration_Button(1);
    }
    else if (events->rot_l != 0U)
    {
      Move_Calibration_Button(-1);
    }

    if (events->rot_click != 0U)
    {
      ui_state.calibration_step = CALIBRATION_STEP_CAPTURE_MIN;
    }

    return;
  }

  if (events->rot_click == 0U)
  {
    return;
  }

  if (ui_state.calibration_step == CALIBRATION_STEP_CAPTURE_MIN)
  {
    button_calibration[selected_button].min_adc = (uint16_t)buttons[selected_button].adc_value;
    ui_state.calibration_step = CALIBRATION_STEP_CAPTURE_MAX;
  }
  else
  {
    button_calibration[selected_button].max_adc = (uint16_t)buttons[selected_button].adc_value;
    Apply_Controller_Settings();
    ui_state.calibration_step = CALIBRATION_STEP_SELECT_BUTTON;
  }
}

static void Process_UI_Input(void)
{
  InputEvents events = input_events;

  Clear_Input_Events();

  switch (ui_state.screen)
  {
    case UI_SCREEN_HOME:
      Process_Home_Input(&events);
      break;

    case UI_SCREEN_SETTINGS:
      Process_Settings_Input(&events);
      break;

    case UI_SCREEN_CALIBRATION:
      Process_Calibration_Input(&events);
      break;

    default:
      ui_state.screen = UI_SCREEN_HOME;
      break;
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
  Initialize_Button_Calibration();
  Apply_Controller_Settings();

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

  OLED_Init();
  OLED_Render_UI();

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
    Process_UI_Input();
    OLED_Render_If_Due();
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
                          |GPIO_PIN_9|GPIO_PIN_13|GPIO_PIN_15, GPIO_PIN_RESET);

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
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
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
