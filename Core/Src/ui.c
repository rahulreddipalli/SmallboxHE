#include "ui.h"

#include "calibration.h"
#include "hall_buttons.h"
#include "settings.h"
#include "ui_render.h"

#define UI_RENDER_PERIOD_MS 50U
#define UI_MENU_ENTRY_COUNT 7U
#define UI_BUTTON_SETUP_FIELD_COUNT 6U
#define UI_PROFILE_FIELD_COUNT 4U
#define UI_VALUE_STEP_ADC 10
#define UI_VALUE_STEP_TRAVEL 10

typedef enum
{
  UI_CONFIRM_NONE = 0,
  UI_CONFIRM_RESET_PROFILE,
  UI_CONFIRM_SAVE_SETTINGS
} UiConfirmAction;

typedef struct
{
  UiScreen screen;
  UiScreen previous_screen;
  UiConfirmAction confirm_action;
  HallButtonId selected_button;
  uint8_t menu_index;
  uint8_t edit_field;
  uint8_t selected_profile;
  uint32_t last_render_tick;
  uint8_t render_due;
} UiState;

static UiState ui;

static void Ui_AppendChar(char *buffer, uint8_t buffer_size, uint8_t *offset, char value)
{
  if (buffer == NULL || offset == NULL || buffer_size == 0U)
  {
    return;
  }

  if (*offset < (uint8_t)(buffer_size - 1U))
  {
    buffer[*offset] = value;
    (*offset)++;
    buffer[*offset] = '\0';
  }
}

static void Ui_AppendString(char *buffer, uint8_t buffer_size, uint8_t *offset, const char *text)
{
  while (text != NULL && *text != '\0')
  {
    Ui_AppendChar(buffer, buffer_size, offset, *text);
    text++;
  }
}

static void Ui_AppendU32(char *buffer, uint8_t buffer_size, uint8_t *offset, uint32_t value)
{
  char digits[10];
  uint8_t digit_count = 0U;

  if (value == 0U)
  {
    Ui_AppendChar(buffer, buffer_size, offset, '0');
    return;
  }

  while (value > 0U && digit_count < sizeof(digits))
  {
    digits[digit_count] = (char)('0' + (value % 10U));
    value /= 10U;
    digit_count++;
  }

  while (digit_count > 0U)
  {
    digit_count--;
    Ui_AppendChar(buffer, buffer_size, offset, digits[digit_count]);
  }
}

static void Ui_RequestRender(void)
{
  ui.render_due = 1U;
}

static void Ui_SetScreen(UiScreen screen)
{
  ui.previous_screen = ui.screen;
  ui.screen = screen;
  ui.edit_field = 0U;
  Ui_RequestRender();
}

static void Ui_AdjustSelectedButton(int8_t direction)
{
  int16_t next_button = (int16_t)ui.selected_button + direction;

  if (next_button < 0)
  {
    next_button = (int16_t)HALL_BUTTON_COUNT - 1;
  }
  else if (next_button >= (int16_t)HALL_BUTTON_COUNT)
  {
    next_button = 0;
  }

  ui.selected_button = (HallButtonId)next_button;
}

static void Ui_AdjustU16(uint16_t current_value, int16_t delta, uint16_t minimum, uint16_t maximum, void (*setter)(HallButtonId, uint16_t))
{
  int32_t next_value = (int32_t)current_value + delta;

  if (setter == NULL)
  {
    return;
  }

  if (next_value < (int32_t)minimum)
  {
    next_value = (int32_t)minimum;
  }
  else if (next_value > (int32_t)maximum)
  {
    next_value = (int32_t)maximum;
  }

  setter(ui.selected_button, (uint16_t)next_value);
}

static void Ui_HandleDashboard(UiEvent event)
{
  if (event == UI_EVT_ROTATE_CW)
  {
    Ui_AdjustSelectedButton(1);
  }
  else if (event == UI_EVT_ROTATE_CCW)
  {
    Ui_AdjustSelectedButton(-1);
  }
  else if (event == UI_EVT_CLICK)
  {
    Ui_SetScreen(UI_SCREEN_BUTTON_SETUP);
  }
  else if (event == UI_EVT_LONG_PRESS)
  {
    Ui_SetScreen(UI_SCREEN_MAIN_MENU);
  }
  else if (event == UI_EVT_DOUBLE_CLICK)
  {
    Settings_ToggleRapidTrigger();
    Hall_Buttons_ApplySettings();
  }
}

static void Ui_HandleMainMenu(UiEvent event)
{
  if (event == UI_EVT_ROTATE_CW)
  {
    ui.menu_index = (uint8_t)((ui.menu_index + 1U) % UI_MENU_ENTRY_COUNT);
  }
  else if (event == UI_EVT_ROTATE_CCW)
  {
    ui.menu_index = (ui.menu_index == 0U) ? (UI_MENU_ENTRY_COUNT - 1U) : (uint8_t)(ui.menu_index - 1U);
  }
  else if (event == UI_EVT_CLICK)
  {
    switch (ui.menu_index)
    {
      case 0:
        Ui_SetScreen(UI_SCREEN_BUTTON_SELECT);
        break;

      case 1:
        Ui_SetScreen(UI_SCREEN_RAPID_TRIGGER);
        break;

      case 2:
        Ui_SetScreen(UI_SCREEN_PROFILES);
        break;

      case 3:
        Ui_SetScreen(UI_SCREEN_CALIBRATION);
        break;

      case 4:
        Ui_SetScreen(UI_SCREEN_DISPLAY);
        break;

      case 5:
        Ui_SetScreen(UI_SCREEN_RAW_ADC);
        break;

      case 6:
      default:
        Ui_SetScreen(UI_SCREEN_SYSTEM);
        break;
    }
  }
  else if (event == UI_EVT_LONG_PRESS || event == UI_EVT_BACK)
  {
    Ui_SetScreen(UI_SCREEN_DASHBOARD);
  }
}

static void Ui_HandleButtonSetup(UiEvent event)
{
  const ButtonSettings *button_settings = Settings_GetButtonSettings(ui.selected_button);
  uint8_t route_output = Settings_GetRouteOutput(ui.selected_button);
  int8_t direction = (event == UI_EVT_ROTATE_CW) ? 1 : -1;
  int16_t next_route;

  if (event == UI_EVT_CLICK)
  {
    ui.edit_field = (uint8_t)((ui.edit_field + 1U) % UI_BUTTON_SETUP_FIELD_COUNT);
  }
  else if (event == UI_EVT_LONG_PRESS || event == UI_EVT_BACK)
  {
    Hall_Buttons_ApplySettings();
    Ui_SetScreen(UI_SCREEN_DASHBOARD);
  }
  else if ((event == UI_EVT_ROTATE_CW || event == UI_EVT_ROTATE_CCW) && button_settings != NULL)
  {
    switch (ui.edit_field)
    {
      case 0:
        Ui_AdjustU16(button_settings->press_threshold, (int16_t)(direction * UI_VALUE_STEP_ADC), SETTINGS_ADC_MIN, SETTINGS_ADC_MAX, Settings_SetButtonPressThreshold);
        break;

      case 1:
        Ui_AdjustU16(button_settings->release_threshold, (int16_t)(direction * UI_VALUE_STEP_ADC), SETTINGS_ADC_MIN, SETTINGS_ADC_MAX, Settings_SetButtonReleaseThreshold);
        break;

      case 2:
        Ui_AdjustU16(button_settings->deadzone_low, (int16_t)(direction * UI_VALUE_STEP_TRAVEL), SETTINGS_TRAVEL_MIN, SETTINGS_TRAVEL_MAX, Settings_SetButtonDeadzoneLow);
        break;

      case 3:
        Ui_AdjustU16(button_settings->deadzone_high, (int16_t)(direction * UI_VALUE_STEP_TRAVEL), SETTINGS_TRAVEL_MIN, SETTINGS_TRAVEL_MAX, Settings_SetButtonDeadzoneHigh);
        break;

      case 4:
        Settings_SetButtonInvertAxis(ui.selected_button, button_settings->invert_axis == 0U);
        break;

      case 5:
        next_route = (route_output == SETTINGS_ROUTE_DISABLED) ? -1 : (int16_t)route_output;
        next_route += direction;
        if (next_route < -1)
        {
          next_route = (int16_t)HALL_BUTTON_COUNT - 1;
        }
        else if (next_route >= (int16_t)HALL_BUTTON_COUNT)
        {
          next_route = -1;
        }
        Settings_SetRouteOutput(ui.selected_button, (next_route < 0) ? SETTINGS_ROUTE_DISABLED : (uint8_t)next_route);
        break;

      default:
        break;
    }

    Hall_Buttons_ApplySettings();
  }
}

static void Ui_HandleRapidTrigger(UiEvent event)
{
  if (event == UI_EVT_CLICK || event == UI_EVT_ROTATE_CW || event == UI_EVT_ROTATE_CCW)
  {
    Settings_ToggleRapidTrigger();
    Hall_Buttons_ApplySettings();
  }
  else if (event == UI_EVT_LONG_PRESS || event == UI_EVT_BACK)
  {
    Ui_SetScreen(UI_SCREEN_DASHBOARD);
  }
}

static void Ui_HandleProfiles(UiEvent event)
{
  if (event == UI_EVT_ROTATE_CW)
  {
    ui.selected_profile = (uint8_t)((ui.selected_profile + 1U) % SETTINGS_PROFILE_COUNT);
  }
  else if (event == UI_EVT_ROTATE_CCW)
  {
    ui.selected_profile = (ui.selected_profile == 0U) ? (SETTINGS_PROFILE_COUNT - 1U) : (uint8_t)(ui.selected_profile - 1U);
  }
  else if (event == UI_EVT_CLICK)
  {
    if (ui.edit_field == 0U)
    {
      Settings_SetActiveProfileIndex(ui.selected_profile);
      Hall_Buttons_ApplySettings();
    }
    else if (ui.edit_field == 1U)
    {
      Settings_CopyProfile(Settings_GetActiveProfileIndex(), ui.selected_profile);
    }
    else if (ui.edit_field == 2U)
    {
      ui.confirm_action = UI_CONFIRM_RESET_PROFILE;
      Ui_SetScreen(UI_SCREEN_CONFIRM_DIALOG);
    }
    else
    {
      ui.confirm_action = UI_CONFIRM_SAVE_SETTINGS;
      Ui_SetScreen(UI_SCREEN_CONFIRM_DIALOG);
    }
  }
  else if (event == UI_EVT_DOUBLE_CLICK)
  {
    ui.edit_field = (uint8_t)((ui.edit_field + 1U) % UI_PROFILE_FIELD_COUNT);
  }
  else if (event == UI_EVT_LONG_PRESS || event == UI_EVT_BACK)
  {
    Ui_SetScreen(UI_SCREEN_DASHBOARD);
  }
}

static void Ui_HandleCalibration(UiEvent event)
{
  if (event == UI_EVT_CLICK && Calibration_IsActive() == 0U)
  {
    Calibration_Start();
  }
  else if (event == UI_EVT_LONG_PRESS || event == UI_EVT_BACK)
  {
    Calibration_Cancel();
    Ui_SetScreen(UI_SCREEN_DASHBOARD);
  }
}

static void Ui_HandleConfirm(UiEvent event)
{
  if (event == UI_EVT_CLICK)
  {
    if (ui.confirm_action == UI_CONFIRM_RESET_PROFILE)
    {
      Settings_ResetProfileToDefault(ui.selected_profile);
      Hall_Buttons_ApplySettings();
    }
    else if (ui.confirm_action == UI_CONFIRM_SAVE_SETTINGS)
    {
      (void)Settings_Save();
    }

    ui.confirm_action = UI_CONFIRM_NONE;
    Ui_SetScreen(UI_SCREEN_PROFILES);
  }
  else if (event == UI_EVT_BACK || event == UI_EVT_LONG_PRESS)
  {
    ui.confirm_action = UI_CONFIRM_NONE;
    Ui_SetScreen(ui.previous_screen);
  }
}

static void Ui_RenderDashboard(void)
{
  char line[32];
  uint8_t offset;
  uint8_t index;

  line[0] = '\0';
  offset = 0U;
  Ui_AppendString(line, sizeof(line), &offset, "P");
  Ui_AppendU32(line, sizeof(line), &offset, Settings_GetActiveProfileIndex());
  Ui_AppendString(line, sizeof(line), &offset, " B");
  Ui_AppendU32(line, sizeof(line), &offset, (uint32_t)ui.selected_button);
  Ui_AppendString(line, sizeof(line), &offset, " ADC");
  Ui_AppendU32(line, sizeof(line), &offset, Hall_Buttons_GetAdcValue(ui.selected_button));
  UiRender_Text(0U, 0U, line);

  line[0] = '\0';
  offset = 0U;
  Ui_AppendString(line, sizeof(line), &offset, "T");
  Ui_AppendU32(line, sizeof(line), &offset, Hall_Buttons_GetNormalizedTravel(ui.selected_button) / 10U);
  Ui_AppendString(line, sizeof(line), &offset, "% ");
  Ui_AppendString(line, sizeof(line), &offset, Hall_Buttons_GetPressedState(ui.selected_button) ? "ON" : "OFF");
  Ui_AppendString(line, sizeof(line), &offset, " RT");
  Ui_AppendU32(line, sizeof(line), &offset, Settings_IsRapidTriggerEnabled());
  UiRender_Text(0U, 8U, line);

  for (index = 0U; index < HALL_BUTTON_COUNT; index++)
  {
    UiRender_Bar(0U, (uint8_t)(16U + (index * 3U)), 64U, Hall_Buttons_GetNormalizedTravel((HallButtonId)index), 1000U);
  }
}

static void Ui_RenderSimpleScreen(const char *title)
{
  char line[32];
  uint8_t offset = 0U;

  UiRender_Text(0U, 0U, title);
  line[0] = '\0';
  Ui_AppendString(line, sizeof(line), &offset, "B");
  Ui_AppendU32(line, sizeof(line), &offset, (uint32_t)ui.selected_button);
  Ui_AppendString(line, sizeof(line), &offset, " F");
  Ui_AppendU32(line, sizeof(line), &offset, ui.edit_field);
  UiRender_Text(0U, 8U, line);
}

static void Ui_RenderRapidTrigger(void)
{
  UiRender_Text(0U, 0U, "Rapid Trigger");
  UiRender_Text(0U, 8U, Settings_IsRapidTriggerEnabled() ? "Global ON" : "Global OFF");
}

void Ui_Init(void)
{
  ui.screen = UI_SCREEN_DASHBOARD;
  ui.previous_screen = UI_SCREEN_DASHBOARD;
  ui.confirm_action = UI_CONFIRM_NONE;
  ui.selected_button = HALL_BUTTON_LEFT;
  ui.menu_index = 0U;
  ui.edit_field = 0U;
  ui.selected_profile = Settings_GetActiveProfileIndex();
  ui.last_render_tick = 0U;
  ui.render_due = 1U;
  Calibration_Init();
}

void Ui_HandleEvent(UiEvent event)
{
  if (event == UI_EVT_NONE)
  {
    return;
  }

  switch (ui.screen)
  {
    case UI_SCREEN_DASHBOARD:
      Ui_HandleDashboard(event);
      break;

    case UI_SCREEN_MAIN_MENU:
      Ui_HandleMainMenu(event);
      break;

    case UI_SCREEN_BUTTON_SELECT:
      Ui_HandleDashboard(event);
      break;

    case UI_SCREEN_BUTTON_SETUP:
      Ui_HandleButtonSetup(event);
      break;

    case UI_SCREEN_RAPID_TRIGGER:
      Ui_HandleRapidTrigger(event);
      break;

    case UI_SCREEN_PROFILES:
      Ui_HandleProfiles(event);
      break;

    case UI_SCREEN_CALIBRATION:
      Ui_HandleCalibration(event);
      break;

    case UI_SCREEN_CONFIRM_DIALOG:
      Ui_HandleConfirm(event);
      break;

    case UI_SCREEN_DISPLAY:
    case UI_SCREEN_RAW_ADC:
    case UI_SCREEN_SYSTEM:
      if (event == UI_EVT_LONG_PRESS || event == UI_EVT_BACK)
      {
        Ui_SetScreen(UI_SCREEN_DASHBOARD);
      }
      break;

    default:
      Ui_SetScreen(UI_SCREEN_DASHBOARD);
      break;
  }

  Ui_RequestRender();
}

void Ui_Tick(void)
{
  Calibration_Tick();

  if ((HAL_GetTick() - ui.last_render_tick) >= UI_RENDER_PERIOD_MS)
  {
    ui.render_due = 1U;
  }

  if (Calibration_GetState() == CAL_DONE && ui.screen == UI_SCREEN_CALIBRATION)
  {
    Ui_SetScreen(UI_SCREEN_DASHBOARD);
  }
}

void Ui_Render(void)
{
  if (ui.render_due == 0U)
  {
    return;
  }

  ui.render_due = 0U;
  ui.last_render_tick = HAL_GetTick();

  UiRender_Clear();

  switch (ui.screen)
  {
    case UI_SCREEN_DASHBOARD:
      Ui_RenderDashboard();
      break;

    case UI_SCREEN_MAIN_MENU:
      Ui_RenderSimpleScreen("Main Menu");
      break;

    case UI_SCREEN_BUTTON_SELECT:
      Ui_RenderSimpleScreen("Button Select");
      break;

    case UI_SCREEN_BUTTON_SETUP:
      Ui_RenderSimpleScreen("Button Setup");
      break;

    case UI_SCREEN_RAPID_TRIGGER:
      Ui_RenderRapidTrigger();
      break;

    case UI_SCREEN_PROFILES:
      Ui_RenderSimpleScreen("Profiles");
      break;

    case UI_SCREEN_CALIBRATION:
      Ui_RenderSimpleScreen("Calibration");
      break;

    case UI_SCREEN_DISPLAY:
      Ui_RenderSimpleScreen("Display");
      break;

    case UI_SCREEN_RAW_ADC:
      Ui_RenderSimpleScreen("Raw ADC");
      break;

    case UI_SCREEN_SYSTEM:
      Ui_RenderSimpleScreen("System");
      break;

    case UI_SCREEN_CONFIRM_DIALOG:
      Ui_RenderSimpleScreen("Confirm");
      break;

    default:
      break;
  }

  UiRender_Flush();
}

UiScreen Ui_GetScreen(void)
{
  return ui.screen;
}
