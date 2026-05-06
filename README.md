# SmallboxHE

SmallboxHE is experimental STM32 firmware for a Hall-effect hitbox-style controller adapter.

It reads analog Hall-effect button sensors with an STM32, turns those readings into digital button states, and drives MOSFET outputs connected to a Brook controller board. The Brook board remains the USB/controller interface; the STM32 adds the Hall-effect behavior layer.

## What It Does

Target signal path:

```text
Hall sensor -> STM32 ADC -> firmware button logic -> GPIO -> MOSFET -> Brook board input
```

The intended controller has:

- 14 Hall-effect analog button sensors
- 14 STM32-controlled MOSFET outputs
- A Brook board handling the final controller/USB interface
- Future support for rapid trigger, per-button tuning, calibration, settings storage, and an OLED/menu interface

## Current Status

This is an early STM32CubeMX/IAR firmware scaffold. It currently has a working 14-button polling structure, but it is not final rapid-trigger firmware yet.

Implemented:

- GPIO, ADC1, and ADC2 initialization
- ADC1/ADC2 single-ended calibration
- 14 Hall ADC channels polled from `Core/Src/hall_buttons.c`
- 14 mapped GPIO outputs for Brook/MOSFET switching
- Simple global press/release thresholds with hysteresis
- A clean `main.c` loop that delegates Hall processing to a separate module

Not implemented yet:

- Rapid trigger
- Per-button thresholds
- Calibration
- Flash-backed settings
- OLED/display/menu logic
- Profiles
- SOCD cleaning
- ADC DMA/timer-triggered scanning

## Current Button Logic

Thresholds are currently hardcoded in `Core/Src/hall_buttons.c`:

```c
static const uint32_t HALL_PRESS_THRESHOLD = 2200;
static const uint32_t HALL_RELEASE_THRESHOLD = 1800;
```

Each button is pressed when its ADC value reaches `2200` and released when it falls to `1800`. The gap is intentional hysteresis to prevent flicker near the threshold.

## Pin Map

| Button | Hall input | ADC channel | Brook/MOSFET output |
|---:|---|---|---|
| Left | `PA4` / `LEFT_HE` | `ADC2_IN17` | `PA10` / `LEFT_NPN` |
| Down | `PA5` / `DOWN_HE` | `ADC2_IN13` | `PA9` / `DOWN_NPN` |
| Right | `PA6` / `RIGHT_HE` | `ADC2_IN3` | `PA8` / `RIGHT_NPN` |
| Up | `PA7` / `UP_HE` | `ADC2_IN4` | `PC6` / `UP_NPN` |
| Square | `PA0` / `SQUARE_HE` | `ADC1_IN1` | `PC7` / `SQUARE_NPN` |
| Triangle | `PA1` / `TRIANGLE_HE` | `ADC1_IN2` | `PB3` / `TRIANGLE_NPN` |
| L1 | `PA2` / `L1_HE` | `ADC1_IN3` | `PB4` / `L1_NPN` |
| R1 | `PA3` / `R1_HE` | `ADC1_IN4` | `PB5` / `R1_NPN` |
| Cross | `PC0` / `CROSS_HE` | `ADC1_IN6` | `PB6` / `CROSS_NPN` |
| Circle | `PC1` / `CIRCLE_HE` | `ADC1_IN7` | `PB7` / `CIRCLE_NPN` |
| L2 | `PC2` / `L2_HE` | `ADC1_IN8` | `PC9` / `L2_NPN` |
| R2 | `PC3` / `R2_HE` | `ADC1_IN9` | `PC8` / `R2_NPN` |
| L3 | `PC4` / `LMOD_HE` | `ADC2_IN5` | `PA11` / `L3_NPN` |
| R3 | `PC5` / `RMOD_HE` | `ADC2_IN11` | `PB9` / `R3_NPN` |

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
- `Core/Src/board.c` - HAL startup, clock, GPIO, ADC init, and ADC calibration
- `Core/Src/app_error.c` - fail-stop error/assert handlers
- `Core/Src/hall_buttons.c` - Hall polling, thresholds, state, and output mapping
- `Core/Inc/app.h`, `Core/Inc/board.h` - application and board interfaces
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
- Confirm output polarity through the MOSFET/Brook board path
- Replace global thresholds with per-button thresholds
- Add raw ADC debug/readout support

Medium term:

- Add calibration
- Add settings defaults and flash storage
- Add rapid trigger
- Split button decision logic and output driving into their own modules

Long term:

- Add OLED/display and menu support
- Add profiles
- Add SOCD cleaning if needed
- Move ADC acquisition to timer-triggered DMA scanning
- Rename project files to remove spaces when safe
