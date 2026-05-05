# SmallboxHE

SmallboxHE is experimental STM32 firmware for a Hall-effect hitbox-style controller adapter.

The hardware concept is:

- 14 Hall-effect button sensors feed analog voltages into STM32 ADC inputs.
- The STM32 decides whether each button is pressed based on configurable trigger/release thresholds.
- 14 STM32 outputs drive MOSFETs.
- Those MOSFETs switch the corresponding input signals on a connected Brook controller board.

The goal is to add Hall-effect behavior, rapid trigger, and per-button tuning while still using the Brook board as the controller interface.

## Current Status

This repository is currently an early STM32CubeMX/IAR project scaffold.

Implemented firmware behavior:

- Initializes GPIO, ADC1, and ADC2.
- Reads one Hall sensor on `PA4` / `ADC2_IN17`.
- Drives one test output on `PA8`.
- Turns the output on above a press threshold and off below a release threshold.
- Uses hysteresis so the output does not flicker around the threshold.

Current test thresholds are in `Core/Src/main.c`:

```c
static const uint32_t HALL_PRESS_THRESHOLD = 2200;
static const uint32_t HALL_RELEASE_THRESHOLD = 1800;
```

## Target Hardware

- MCU: `STM32G474RET6`
- STM32 family: `STM32G4`
- Package: `LQFP64`
- Project generated with STM32CubeMX `6.17.0`
- Firmware package: `STM32Cube FW_G4 V1.6.2`
- Toolchain: IAR Embedded Workbench for ARM, EWARM `V8.50`

## Project Layout

```text
Core/       Application source and headers
Drivers/    STM32 HAL and CMSIS vendor files
EWARM/      IAR project, workspace, startup, and linker files
*.ioc       STM32CubeMX configuration
```

Main files:

- `Core/Src/main.c` - application entry point and current Hall sensor test
- `Core/Src/stm32g4xx_hal_msp.c` - generated peripheral pin/clock setup
- `STM Projects.ioc` - CubeMX pin and peripheral configuration
- `EWARM/Project.eww` - IAR workspace

## Building

Open the IAR workspace:

```text
EWARM/Project.eww
```

Then build/debug from IAR Embedded Workbench.

## Development Notes

When editing CubeMX-generated files, put custom code only inside `USER CODE BEGIN` / `USER CODE END` blocks. CubeMX can overwrite generated sections when the project is regenerated.

Do not edit files under `Drivers/` unless there is a very specific reason. They are vendor-provided STM32 HAL/CMSIS files.

## Roadmap

- Read all 14 Hall sensors.
- Map each sensor to its corresponding MOSFET output.
- Add per-button press and release thresholds.
- Add rapid-trigger behavior.
- Add calibration/settings storage.
- Add a board-side configuration interface.
- Add safer project naming and cleanup generated/vendor files where practical.
