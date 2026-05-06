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

Button 0 preserves the known prototype path: `PA4 / ADC2_IN17 -> PA8`.

| Button | Hall input | ADC channel | Brook/MOSFET output |
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

Reserved or spare pins:

- `PB0`, `PB1`, `PB2`: GPIO inputs, possible user/settings buttons
- `PC7`, `PC8`, `PC9`: spare GPIO outputs
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

- `Core/Src/main.c` - startup, CubeMX init calls, and simple main loop
- `Core/Src/hall_buttons.c` - Hall polling, thresholds, state, and output mapping
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
