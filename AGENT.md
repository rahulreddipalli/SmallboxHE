# SmallboxHE Agent Context

This file is for Codex/agentic development context. Keep `README.md` pleasant for humans; keep the detailed engineering rules, project assumptions, and future architecture here.

## Project Intent

SmallboxHE is STM32 firmware for a Hall-effect hitbox-style controller adapter.

The Brook board remains the actual controller/USB interface. The STM32 sits in front of it:

```text
Hall sensor magnet position
        -> analog Hall voltage
        -> STM32 ADC input
        -> firmware threshold / rapid-trigger logic
        -> STM32 GPIO output
        -> MOSFET switch
        -> Brook board button input
```

The goal is a low-latency, configurable Hall-effect layer with per-button tuning, rapid trigger, calibration, settings storage, and eventually OLED/menu interaction.

## Current Implementation

The current firmware is a bring-up scaffold, not final low-latency firmware.

Implemented:

- `main.c` initializes GPIO, ADC1, and ADC2.
- `main.c` calibrates ADC1 and ADC2 in single-ended mode.
- `main.c` calls `Hall_Buttons_Init(&hadc1, &hadc2)`.
- Main loop calls `Hall_Buttons_UpdateAll()` then `HAL_Delay(1)`.
- `hall_buttons.c` polls 14 ADC channels one at a time.
- `hall_buttons.c` drives 14 mapped GPIO outputs.
- Button state uses global fixed hysteresis thresholds.

Current `main.c` shape:

```c
Hall_Buttons_Init(&hadc1, &hadc2);

while (1)
{
  Hall_Buttons_UpdateAll();
  HAL_Delay(1);
}
```

Keep this style. `main.c` should coordinate modules, not contain application logic.

## Current Button Logic

Thresholds live in `Core/Src/hall_buttons.c`:

```c
static const uint32_t HALL_PRESS_THRESHOLD = 2200;
static const uint32_t HALL_RELEASE_THRESHOLD = 1800;
```

Behavior:

- Read one ADC channel.
- If currently released and ADC value is `>= 2200`, output becomes set/high.
- If currently pressed and ADC value is `<= 1800`, output becomes reset/low.
- Otherwise preserve previous state.

This is fixed threshold hysteresis, not rapid trigger.

Known limitations:

- Thresholds are global.
- Thresholds are hardcoded.
- No calibration.
- ADC channels are reconfigured and polled one at a time.
- `HAL_Delay(1)` limits update rate.
- No timer-triggered ADC or DMA.
- No debounce/filtering beyond hysteresis.

## Current Pin Map

Source of truth in code: `Core/Src/hall_buttons.c`.

Button 0 is the known prototype path and should not be changed unless the user explicitly asks:

```text
PA4 / ADC2_IN17 -> PA8
```

| Button index | Hall input | ADC channel | Brook/MOSFET output |
|---:|---|---|---|
| 0 | `PA4` | `ADC2_IN17` | `PA8` |
| 1 | `PA0` | `ADC1_IN1` | `PA9` |
| 2 | `PA1` | `ADC1_IN2` | `PA10` |
| 3 | `PA2` | `ADC1_IN3` | `PA11` |
| 4 | `PA3` | `ADC1_IN4` | `PB3` |
| 5 | `PA5` | `ADC2_IN13` | `PB4` |
| 6 | `PA6` | `ADC2_IN3` | `PB5` |
| 7 | `PA7` | `ADC2_IN4` | `PB6` |
| 8 | `PC0` | `ADC1_IN6` | `PB7` |
| 9 | `PC1` | `ADC1_IN7` | `PB9` |
| 10 | `PC2` | `ADC1_IN8` | `PB10` |
| 11 | `PC3` | `ADC1_IN9` | `PB11` |
| 12 | `PC4` | `ADC2_IN5` | `PB12` |
| 13 | `PC5` | `ADC2_IN11` | `PC6` |

Additional configured pins:

| Pin | Current config | Intended use / note |
|---|---|---|
| `PB0` | GPIO input | Spare/user input candidate |
| `PB1` | GPIO input | Spare/user input candidate |
| `PB2` | GPIO input | Spare/user input candidate |
| `PC7` | GPIO output | Spare output |
| `PC8` | GPIO output | Spare output |
| `PC9` | GPIO output | Spare output |
| `PB13` | `SPI2_SCK` alternate-function pin | Reserved for possible SPI OLED/display |
| `PB15` | `SPI2_MOSI` alternate-function pin | Reserved for possible SPI OLED/display |
| `PA13` | SWDIO | Debug/programming |
| `PA14` | SWCLK | Debug/programming |

OLED status: `PB13` and `PB15` are assigned at the pin level, but SPI2 is not fully initialized and no display driver/drawing logic exists.

## Hardware Assumptions

Current assumptions:

- Hall sensors are single-ended analog outputs referenced to board ground.
- ADC channels should be configured single-ended unless hardware notes explicitly say otherwise.
- GPIO output high currently represents pressed in firmware.
- MOSFET/Brook electrical polarity still needs hardware validation.
- Brook board handles USB/controller behavior.

Unknowns that must be clarified before final firmware decisions:

- Exact button labels/order for the 14 buttons.
- Brook board model.
- Which Brook input each MOSFET output connects to.
- Hall sensor part number.
- Hall sensor output voltage range.
- Whether ADC value increases or decreases when pressed.
- Magnet orientation and expected rest/pressed ADC values per button.
- MOSFET part number and active polarity.
- OLED controller type, resolution, bus, reset/DC/CS pins.
- Settings input method: OLED buttons, spare GPIOs, serial, USB, or something else.
- Whether SOCD cleaning belongs in STM32 firmware or should be left to the Brook board/game.

Document assumptions instead of silently baking them into code.

## Toolchain And Project Files

Target:

- MCU: `STM32G474RET6`
- Family: `STM32G4`
- Package: `LQFP64`
- CubeMX: `6.17.0`
- Firmware package: `STM32Cube FW_G4 V1.6.2`
- Toolchain: IAR Embedded Workbench for ARM, EWARM `V8.50`

Important files:

- `Core/Src/main.c` - generated/application entry point. Keep simple.
- `Core/Src/hall_buttons.c` - current Hall polling and output mapping.
- `Core/Inc/hall_buttons.h` - Hall module public interface.
- `Core/Src/stm32g4xx_hal_msp.c` - generated pin/clock MSP setup.
- `Core/Inc/stm32g4xx_hal_conf.h` - HAL module config.
- `STM Projects.ioc` - CubeMX source of pin/peripheral configuration.
- `EWARM/Project.eww` - IAR workspace.
- `EWARM/STM Projects.ewp` - IAR project file. Add new `.c` files here.

No local command-line build is currently known. In previous sessions, neither `iccarm` nor `arm-none-eabi-gcc` was available on PATH.

## CubeMX Safety

CubeMX can overwrite generated sections.

Safe places:

- Code inside `/* USER CODE BEGIN ... */` / `/* USER CODE END ... */`.
- Separate user-created modules such as `hall_buttons.c`.

Risky places:

- Generated init functions in `main.c`.
- Generated MSP functions in `stm32g4xx_hal_msp.c`.
- Project metadata if CubeMX regenerates.

When changing pins/peripherals:

- Update `STM Projects.ioc`.
- Regenerate carefully if using CubeMX.
- Re-check `main.c`, `stm32g4xx_hal_msp.c`, `stm32g4xx_hal_conf.h`, and `EWARM/STM Projects.ewp`.
- Confirm custom `.c` files are still included in IAR.

## Agent Development Rules

- Read this file and `README.md` before making architectural changes.
- Prefer small modules over expanding `main.c`.
- Preserve `PA4 / ADC2_IN17 -> PA8` unless explicitly asked to change it.
- Keep Hall ADC inputs single-ended unless explicit hardware evidence says otherwise.
- Do not edit `Drivers/` unless there is a specific HAL/vendor reason.
- Avoid unrelated generated/vendor churn.
- Update `EWARM/STM Projects.ewp` whenever adding a `.c` file.
- Update `README.md` for human-facing status/pin/usage changes.
- Update `AGENT.md` for architecture, rules, assumptions, and future-agent context.
- If a hardware detail is unclear, ask or record it as an assumption.
- Keep code ASCII unless the file already uses non-ASCII for a reason.

## Intended Module Architecture

Current:

```text
Core/Src/main.c
Core/Src/hall_buttons.c
Core/Inc/hall_buttons.h
```

Desired future split:

```text
Core/Src/main.c              startup and loop coordination
Core/Src/hall_buttons.c      physical Hall ADC channel mapping/acquisition
Core/Src/button_logic.c      actuation, release, rapid trigger, SOCD rules
Core/Src/outputs.c           Brook/MOSFET GPIO output driving
Core/Src/settings.c          runtime settings and defaults
Core/Src/settings_storage.c  flash persistence
Core/Src/calibration.c       rest/press range measurement and threshold setup
Core/Src/display.c           OLED/display driver integration
Core/Src/menu.c              settings page/menu state machine
```

Only create these when implementing actual behavior. Do not add empty architecture files just to match this list.

## Feature Roadmap

Near term:

- Validate all 14 ADC inputs on hardware.
- Confirm all 14 MOSFET/Brook outputs on hardware.
- Confirm output active polarity.
- Replace global thresholds with per-button thresholds.
- Add a raw ADC debug/readout path.
- Remove or replace `HAL_Delay(1)` once timing needs are clearer.

Medium term:

- Add calibration for rest/pressed values.
- Add settings structs with defaults.
- Add flash-backed settings storage.
- Add rapid trigger.
- Add per-button actuation/release distances or ADC deltas.
- Split physical ADC acquisition from button decision logic.
- Split output driving into its own module.

Longer term:

- Add OLED/display driver.
- Add settings page/menu logic.
- Add profiles.
- Add SOCD cleaning if needed.
- Move ADC acquisition to timer-triggered scanning and DMA.
- Add a board-side configuration interface.
- Rename project files to remove spaces, for example `SmallboxHE.ioc`, when safe.

## Definition Of Done For Firmware Changes

- Code compiles in IAR.
- New `.c` files are included in `EWARM/STM Projects.ewp`.
- `main.c` remains simple.
- Pin map changes are reflected in code, `STM Projects.ioc`, `README.md`, and this file.
- Behavior changes are documented.
- Hardware assumptions are documented.
- The user is told if build verification could not be run locally.
