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
        -> gate-driven external switch
        -> Brook board button input
```

The goal is a low-latency, configurable Hall-effect layer with per-button tuning, rapid trigger, calibration, settings storage, and eventually OLED/menu interaction.

## Current Implementation

The current firmware is a bring-up scaffold, not final low-latency firmware.

Implemented:

- `main.c` calls `Board_Init()`, `App_Init()`, then repeatedly calls `App_RunOnce()`.
- `board.c` initializes HAL, clock, GPIO, ADC1, and ADC2.
- `board.c` calibrates ADC1 and ADC2 in single-ended mode.
- `app.c` connects the board ADC handles to `Hall_Buttons_Init(...)`.
- `app.c` initializes runtime settings before Hall buttons.
- `app.c` runs `Hall_Buttons_UpdateAll()` then `HAL_Delay(1)` once per tick.
- `settings.c` stores RAM-only settings for rapid trigger enable, actuation/release distance, and calibration mode.
- `hall_buttons.c` polls 14 ADC channels one at a time.
- `hall_buttons.c` drives 14 mapped GPIO outputs.
- Button state uses global hysteresis thresholds derived from the current actuation distance setting.

Current `main.c` shape:

```c
Board_Init();
App_Init();

while (1)
{
  App_RunOnce();
}
```

Keep this style. `main.c` should coordinate modules, not contain application logic.

## Current Button Logic

Threshold defaults are driven by `Core/Src/settings.c` and applied by `Core/Src/hall_buttons.c`:

```c
#define SETTINGS_ACTUATION_DISTANCE_DEFAULT 400U
```

Behavior:

- Read one ADC channel.
- If currently released and ADC value is `>= press_threshold`, output becomes set/high.
- If currently pressed and ADC value is `<= release_threshold`, output becomes reset/low.
- Otherwise preserve previous state.

This is configurable fixed-threshold hysteresis, not rapid trigger.

Known limitations:

- Thresholds are global.
- No per-button thresholds yet.
- Rapid trigger flag exists, but the rapid-trigger algorithm is not implemented yet.
- Calibration mode flag exists, but the calibration workflow is not implemented yet.
- Settings are RAM-only; no flash storage yet.
- ADC channels are reconfigured and polled one at a time.
- `HAL_Delay(1)` limits update rate.
- No timer-triggered ADC or DMA.
- No debounce/filtering beyond hysteresis.

## Current Pin Map

Source of truth in code: `Core/Src/hall_buttons.c`.

| Button index | Hall input | ADC channel | Gate output |
|---:|---|---|---|
| 0 / Left | `PA4` / `LEFT_HE` | `ADC2_IN17` | `PA10` / `LEFT_GATE` |
| 1 / Down | `PA5` / `DOWN_HE` | `ADC2_IN13` | `PA9` / `DOWN_GATE` |
| 2 / Right | `PA6` / `RIGHT_HE` | `ADC2_IN3` | `PA8` / `RIGHT_GATE` |
| 3 / Up | `PA7` / `UP_HE` | `ADC2_IN4` | `PC6` / `UP_GATE` |
| 4 / Square | `PA0` / `SQUARE_HE` | `ADC1_IN1` | `PC7` / `SQUARE_GATE` |
| 5 / Triangle | `PA1` / `TRIANGLE_HE` | `ADC1_IN2` | `PB3` / `TRIANGLE_GATE` |
| 6 / L1 | `PA2` / `L1_HE` | `ADC1_IN3` | `PB4` / `L1_GATE` |
| 7 / R1 | `PA3` / `R1_HE` | `ADC1_IN4` | `PB5` / `R1_GATE` |
| 8 / Cross | `PC0` / `CROSS_HE` | `ADC1_IN6` | `PB6` / `CROSS_GATE` |
| 9 / Circle | `PC1` / `CIRCLE_HE` | `ADC1_IN7` | `PB7` / `CIRCLE_GATE` |
| 10 / L2 | `PC2` / `L2_HE` | `ADC1_IN8` | `PC9` / `L2_GATE` |
| 11 / R2 | `PC3` / `R2_HE` | `ADC1_IN9` | `PC8` / `R2_GATE` |
| 12 / L3 | `PC4` / `LMOD_HE` | `ADC2_IN5` | `PA11` / `L3_GATE` |
| 13 / R3 | `PC5` / `RMOD_HE` | `ADC2_IN11` | `PB9` / `R3_GATE` |

Additional schematic/firmware pins:

| Pin | Current config | Intended use / note |
|---|---|---|
| `PB0` | GPIO input | Spare/user input candidate |
| `PB1` | GPIO input | Spare/user input candidate |
| `PB2` | GPIO input | Spare/user input candidate |
| `PA12` | Not configured in firmware | `MENU` on the schematic, not currently driven by Hall button firmware |
| `PB10` | GPIO output | `DC` display/control candidate |
| `PB11` | GPIO output | `RST` display/control candidate |
| `PB12` | GPIO output | `CS` display/control candidate |
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
- Gate/Brook electrical polarity still needs hardware validation.
- Brook board handles USB/controller behavior.

Unknowns that must be clarified before final firmware decisions:

- Brook board model.
- Hall sensor part number.
- Hall sensor output voltage range.
- Whether ADC value increases or decreases when pressed.
- Magnet orientation and expected rest/pressed ADC values per button.
- External switch part number and active gate polarity.
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

- `Core/Src/main.c` - thin startup and loop coordinator.
- `Core/Src/app.c` - application init and one-pass loop work.
- `Core/Inc/app.h` - app module public interface.
- `Core/Src/board.c` - HAL startup, system clock, GPIO, ADC init, and ADC calibration.
- `Core/Inc/board.h` - board init and ADC handle accessors.
- `Core/Src/app_error.c` - shared `Error_Handler` and assert handler.
- `Core/Src/settings.c` - RAM-only runtime settings backend.
- `Core/Inc/settings.h` - settings backend public interface.
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

- CubeMX-generated init logic that has been moved into `board.c`.
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
- Keep Hall input names and gate output names aligned with the schematic labels.
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
Core/Src/main.c         startup and loop coordination
Core/Src/app.c          application init and per-loop tick
Core/Src/board.c        HAL/peripheral initialization
Core/Src/app_error.c    fail-stop error/assert handlers
Core/Src/hall_buttons.c Hall polling and gate output mapping
```

Desired future split:

```text
Core/Src/main.c              startup and loop coordination
Core/Src/hall_buttons.c      physical Hall ADC channel mapping/acquisition
Core/Src/button_logic.c      actuation, release, rapid trigger, SOCD rules
Core/Src/outputs.c           gate GPIO output driving
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
- Confirm all 14 gate outputs on hardware.
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
