# SmallboxHE

SmallboxHE is experimental STM32 firmware for a Hall-effect hitbox-style controller adapter.

It reads analog Hall-effect button sensors with an STM32, turns those readings into digital button states, and drives gate outputs for the external switching stage connected to a Brook controller board. The Brook board remains the USB/controller interface; the STM32 adds the Hall-effect behavior layer.

## What It Does

Target signal path:

```text
Hall sensor -> STM32 ADC -> firmware button logic -> GPIO gate output -> external switch -> Brook board input
```

The intended controller has:

- 14 Hall-effect analog button sensors
- 14 STM32-controlled gate outputs
- A Brook board handling the final controller/USB interface
- Future support for rapid trigger, per-button tuning, calibration, and an OLED/menu interface

## Current Status

This is an early STM32CubeMX/IAR firmware scaffold. It currently has a working 14-button DMA-backed scan structure, but it is not final rapid-trigger firmware yet.

Implemented:

- GPIO, ADC1, and ADC2 initialization
- ADC1/ADC2 single-ended calibration
- 14 Hall ADC channels scanned continuously into DMA buffers
- 14 mapped GPIO gate outputs for Brook switching
- Simple global press/release thresholds with hysteresis
- Runtime settings backend for profiles, routing, rapid trigger enable, actuation/release distance, and calibration mode
- 4 programmable controller profiles, with Profile 0 defaulting to one-to-one Hall-to-gate routing
- Flash-backed settings/profile storage in the last reserved 2 KB flash page
- A clean `main.c` loop that delegates Hall processing to a separate module

Not implemented yet:

- Rapid trigger algorithm
- Per-button thresholds
- Calibration workflow
- OLED/display/menu logic
- SOCD cleaning
- Timer-triggered ADC scanning

## Current Button Logic

Thresholds are derived from the actuation distance in `Core/Src/settings.c`:

```c
#define SETTINGS_ACTUATION_DISTANCE_DEFAULT 400U
```

The current default keeps the midpoint near the original `2200` press / `1800` release behavior, but the menu backend can now adjust that distance. The same setting controls the press and release gap symmetrically for now.

ADC1 and ADC2 run continuous scan conversions with circular DMA. `hall_buttons.c` reads the latest samples from the DMA buffers and updates the gate outputs without blocking on `HAL_ADC_PollForConversion(...)`.

DMA setup is currently hand-written in `Core/Src/board.c`. The `.ioc` file tracks the ADC scan channel order, but future CubeMX regeneration may not preserve the manual DMA initialization unless it is re-applied or modeled in CubeMX.

Settings currently include:

- rapid trigger enabled/disabled
- actuation/release distance
- calibration mode enabled/disabled
- active profile index
- 4 profile routing tables

Each physical Hall source can route to any physical gate output or be disabled. Profile 0 defaults to one-to-one routing, so `LMOD_HE` drives `L3_GATE` and `RMOD_HE` drives `R3_GATE`. A later OLED/rotary menu can reprogram that profile so, for example, `LMOD_HE` contributes to `LEFT_GATE` and `RMOD_HE` contributes to `RIGHT_GATE`. Shared outputs are OR-combined, so multiple Hall sources can target one gate without overwriting each other.

Settings load from flash during `Settings_Init()` if the saved record has the expected magic, version, length, and CRC. `Settings_Save()` writes the current settings/profile store. The IAR flash linker file reserves the last flash page by ending application ROM at `0x0807F7FF`; settings storage starts at `0x0807F800`.

## Pin Map

| Button | Hall input | ADC channel | Gate output |
|---:|---|---|---|
| Left | `PA4` / `LEFT_HE` | `ADC2_IN17` | `PA10` / `LEFT_GATE` |
| Down | `PA5` / `DOWN_HE` | `ADC2_IN13` | `PA9` / `DOWN_GATE` |
| Right | `PA6` / `RIGHT_HE` | `ADC2_IN3` | `PA8` / `RIGHT_GATE` |
| Up | `PA7` / `UP_HE` | `ADC2_IN4` | `PC6` / `UP_GATE` |
| Square | `PA0` / `SQUARE_HE` | `ADC1_IN1` | `PC7` / `SQUARE_GATE` |
| Triangle | `PA1` / `TRIANGLE_HE` | `ADC1_IN2` | `PB3` / `TRIANGLE_GATE` |
| L1 | `PA2` / `L1_HE` | `ADC1_IN3` | `PB4` / `L1_GATE` |
| R1 | `PA3` / `R1_HE` | `ADC1_IN4` | `PB5` / `R1_GATE` |
| Cross | `PC0` / `CROSS_HE` | `ADC1_IN6` | `PB6` / `CROSS_GATE` |
| Circle | `PC1` / `CIRCLE_HE` | `ADC1_IN7` | `PB7` / `CIRCLE_GATE` |
| L2 | `PC2` / `L2_HE` | `ADC1_IN8` | `PC9` / `L2_GATE` |
| R2 | `PC3` / `R2_HE` | `ADC1_IN9` | `PC8` / `R2_GATE` |
| L3 | `PC4` / `LMOD_HE` | `ADC2_IN5` | `PA11` / `L3_GATE` |
| R3 | `PC5` / `RMOD_HE` | `ADC2_IN11` | `PB9` / `R3_GATE` |

Reserved or spare pins:

- `PB0`, `PB1`, `PB2`: GPIO inputs, possible user/settings buttons
- `PA12`: `MENU` on the schematic, not currently configured or driven by Hall button firmware
- `PB10`, `PB11`, `PB12`: display/control outputs marked `DC`, `RST`, and `CS`
- `PB13`: configured as `SPI2_SCK`, reserved for possible OLED
- `PB15`: configured as `SPI2_MOSI`, reserved for possible OLED
- `PA13`, `PA14`: SWD debug pins

SPI/OLED note: display pins are reserved, but SPI/OLED firmware is not configured yet.

## Hardware / Toolchain

- MCU: `STM32G474RET6`
- Family: `STM32G4`
- Package: `LQFP64`
- CubeMX: `6.17.0`
- STM32Cube firmware package: `STM32Cube FW_G4 V1.6.2`
- Toolchain: IAR Embedded Workbench for ARM, EWARM `V8.50`
- Workspace: `EWARM/Project.eww`

## Project Layout

```text
Core/
  Inc/      Application headers and generated STM32 headers
  Src/      Application source and generated STM32 source
Drivers/   STM32 HAL and CMSIS vendor files
EWARM/     IAR project, workspace, startup, and linker files
*.ioc      STM32CubeMX configuration
```

Important files:

- `Core/Src/main.c` - small startup and application loop coordinator
- `Core/Src/app.c` - application init and one-pass loop work
- `Core/Src/board.c` - HAL startup, clock, GPIO, ADC/DMA init, ADC calibration, and ADC DMA start
- `Core/Src/app_error.c` - fail-stop error/assert handlers
- `Core/Src/settings.c` - runtime settings backend for future OLED/menu control
- `Core/Src/settings_storage.c` - flash-backed settings/profile persistence
- `Core/Src/hall_buttons.c` - Hall DMA sample consumption, thresholds, state, and output mapping
- `Core/Inc/app.h`, `Core/Inc/board.h`, `Core/Inc/settings.h` - application, board, and settings interfaces
- `Core/Inc/settings_storage.h` - settings storage interface
- `Core/Inc/hall_buttons.h` - Hall button module interface
- `STM Projects.ioc` - CubeMX pin/peripheral configuration
- `EWARM/STM Projects.ewp` - IAR project file

For deeper agent/developer context, see `AGENT.md`.

## Building

Open:

```text
EWARM/Project.eww
```

Then build/debug from IAR Embedded Workbench.

When adding new `.c` files, also add them to `EWARM/STM Projects.ewp` so IAR includes them in the build.

## Roadmap

Near term:

- Validate all 14 Hall inputs on hardware
- Confirm gate polarity through the Brook board switching path
- Replace global thresholds with per-button thresholds
- Add OLED/rotary UI for editing active profile routes
- Add raw ADC debug/readout support

Medium term:

- Add calibration
- Add rapid trigger
- Split button decision logic and output driving into their own modules

Long term:

- Add OLED/display and menu support
- Add SOCD cleaning if needed
- Move ADC acquisition from software-start continuous scan to timer-triggered DMA scanning if needed
- Rename project files to remove spaces when safe
