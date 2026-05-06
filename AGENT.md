# SmallboxHE Agent Entry Point

This is the context golden source for Codex and other agentic development sessions on SmallboxHE. Start here before planning, refactoring, or implementing firmware features.

`README.md` is for humans. This file is for building the project correctly.

## First 10 Minutes For A New Agent

Before changing code:

1. Read this file fully.
2. Read `README.md` for the public-facing project summary.
3. Inspect the working tree with `git status --short --branch`.
4. Inspect the current firmware shape:
   - `Core/Src/main.c`
   - `Core/Src/app.c`
   - `Core/Src/board.c`
   - `Core/Src/settings.c`
   - `Core/Src/hall_buttons.c`
   - related headers in `Core/Inc/`
5. If adding source files, inspect and update `EWARM/STM Projects.ewp`.
6. If changing pins or peripherals, inspect and update `STM Projects.ioc`.
7. Do not assume hardware details that are not documented here.
8. Keep `main.c` tiny.
9. For any hardware-specific work, inspect the relevant official datasheets, reference manuals, schematics, or vendor documentation online before designing or coding against that hardware.

Useful orientation commands:

```powershell
git status --short --branch
rg -n "Hall_|Settings_|Board_|App_|GPIO_PIN|ADC_CHANNEL|SPI2|OLED|MENU" Core "STM Projects.ioc" "EWARM/STM Projects.ewp"
Get-ChildItem Core\Inc,Core\Src | Select-Object Directory,Name
```

## Project Mission

SmallboxHE is STM32 firmware for a Hall-effect hitbox-style controller adapter.

The STM32 is not intended to be the USB controller. The Brook board remains the controller/USB interface. The STM32 is an analog Hall sensing and digital switching layer in front of the Brook board.

Target signal chain:

```text
Button/magnet movement
        -> Hall sensor analog voltage
        -> STM32 ADC input
        -> firmware button state logic
        -> STM32 GPIO gate output
        -> external switching stage
        -> Brook board button input
```

Long-term firmware goals:

- Read 14 Hall-effect analog sensors.
- Convert analog position to digital button state.
- Support per-button actuation/release tuning.
- Support rapid trigger.
- Support calibration.
- Persist settings to flash.
- Optionally provide OLED/menu configuration.
- Keep Brook board as final controller interface.

## Current Architecture

The project has already moved beyond a monolithic `main.c`.

Current control flow:

```text
main.c
  -> Board_Init()
       HAL_Init()
       clock init
       GPIO init
       DMA init
       ADC1/ADC2 scan init
       ADC calibration
       ADC1/ADC2 circular DMA start
  -> App_Init()
       Settings_Init()
       Hall_Buttons_Init(Board_GetHallAdc1Samples(), Board_GetHallAdc2Samples())
  -> while forever
       App_RunOnce()
         Hall_Buttons_UpdateAll()
```

Current `main.c` should stay essentially this simple:

```c
int main(void)
{
  Board_Init();
  App_Init();

  while (1)
  {
    App_RunOnce();
  }
}
```

Do not move product logic back into `main.c`.

## Current Files And Ownership

Application files:

- `Core/Src/main.c`
  - Thin startup and forever-loop coordinator.
  - Should only include high-level app/board calls.
- `Core/Src/app.c`
  - Application initialization and one loop tick.
  - Owns current call order between settings and Hall logic.
- `Core/Src/board.c`
  - HAL startup, system clock, GPIO init, DMA init, ADC scan init, ADC calibration, and ADC DMA start.
  - Owns `hall_adc1` and `hall_adc2` handles.
  - Owns the ADC DMA handles and ADC sample buffers.
  - Provides ADC sample buffer accessors.
- `Core/Src/app_error.c`
  - Shared `Error_Handler()` and assert fail-stop behavior.
- `Core/Src/settings.c`
  - Runtime settings/profile backend.
  - Rapid-trigger enable flag exists, but rapid trigger algorithm does not.
  - Calibration-mode flag exists, but calibration workflow does not.
- `Core/Src/hall_buttons.c`
  - Physical Hall ADC to gate output map.
  - Maps buttons to ADC DMA sample buffer indices.
  - Current hysteresis button state logic.
  - Raw ADC and gate state getters.

Headers:

- `Core/Inc/app.h`
- `Core/Inc/board.h`
- `Core/Inc/settings.h`
- `Core/Inc/hall_buttons.h`
- `Core/Inc/main.h`
- generated STM32 HAL headers

Project metadata:

- `STM Projects.ioc`
  - CubeMX pin/peripheral configuration.
- `EWARM/Project.eww`
  - IAR workspace.
- `EWARM/STM Projects.ewp`
  - IAR project file. New `.c` files must be listed here.

Vendor/generated:

- `Drivers/`
  - STM32 HAL/CMSIS vendor code. Avoid editing.
- `Core/Src/stm32g4xx_hal_msp.c`
  - Generated MSP setup. Be careful with CubeMX regeneration.
- `Core/Src/system_stm32g4xx.c`
  - Generated/system support.

## Current Implementation Facts

Settings:

- `Settings_Init()` calls `Settings_ResetDefaults()` and then attempts `Settings_Storage_Load(&settings)`.
- Settings are flash-backed through `Core/Src/settings_storage.c`.
- If flash load fails magic/version/length/CRC validation, defaults remain active.
- `Settings_Save()` writes the current settings/profile store to flash.
- The IAR flash linker reserves the final 2 KB page by ending ROM at `0x0807F7FF`.
- Settings storage begins at `0x0807F800`, which is the final 2 KB flash page on the 512 KB STM32G474RET6 layout.
- Defaults in `Core/Inc/settings.h`:

```c
#define SETTINGS_ACTUATION_DISTANCE_MIN 1U
#define SETTINGS_ACTUATION_DISTANCE_MAX 4095U
#define SETTINGS_ACTUATION_DISTANCE_DEFAULT 400U
#define SETTINGS_PROFILE_COUNT 4U
#define SETTINGS_ROUTE_DISABLED 0xFFU
```

- `SettingsState` currently has:
  - `active_profile_index`
  - `calibration_mode_enabled`
  - `profiles[SETTINGS_PROFILE_COUNT]`
- `ControllerProfile` currently has:
  - `output_for_button[HALL_BUTTON_COUNT]`
  - `rapid_trigger_enabled`
  - `actuation_distance`
- Profile 0 defaults to one-to-one routing.
- A route target can be `0..HALL_BUTTON_COUNT - 1` or `SETTINGS_ROUTE_DISABLED`.

Hall button logic:

- Current base thresholds in `Core/Src/hall_buttons.c`:

```c
static const uint32_t HALL_PRESS_THRESHOLD = 2200U;
static const uint32_t HALL_RELEASE_THRESHOLD = 1800U;
static const uint32_t HALL_THRESHOLD_CENTER = 2000U;
```

- `Hall_Buttons_ApplySettings()` derives current global thresholds from `Settings_GetActuationDistance()`.
- With default distance `400`, thresholds remain equivalent to press `2200`, release `1800`.
- The same actuation distance is applied to every button.
- Thresholds are symmetrical around center `2000`.
- This is still fixed-threshold hysteresis, not rapid trigger.
- `Settings_IsRapidTriggerEnabled()` exists but is not used by `hall_buttons.c`.
- `Settings_IsCalibrationModeEnabled()` exists but is not used by a calibration workflow.
- `hall_buttons.c` asks `Settings_GetRouteOutput(...)` where each physical Hall source should route.
- Output states are OR-combined so multiple Hall sources can map to the same gate safely.
- This replaces the old special-case mod-as-directions setting.

ADC behavior:

- ADC reads are single-ended.
- ADC1 scans 8 Hall channels into a circular DMA buffer.
- ADC2 scans 6 Hall channels into a circular DMA buffer.
- `Hall_Buttons_UpdateAll()` consumes the latest DMA samples; it does not call `HAL_ADC_PollForConversion(...)`.
- ADC conversions currently start from software and then run continuously.
- `ADC_SAMPLETIME_2CYCLES_5` is used.
- ADC resolution is 12-bit, so values are expected in `0..4095`.
- DMA uses `DMA1_Channel1` / `DMA_REQUEST_ADC1` for ADC1 and `DMA1_Channel2` / `DMA_REQUEST_ADC2` for ADC2.
- DMA mode is circular with halfword peripheral and memory alignment.
- DMA half-transfer and transfer-complete interrupts are disabled after start to avoid constant IRQ load; DMA error interrupts remain available.
- DMA initialization is currently hand-written in `Core/Src/board.c`; the `.ioc` reflects ADC scan ranks but may not preserve this DMA setup if CubeMX regenerates code.
- No timer trigger, oversampling, or filtering yet.

Loop timing:

- `App_RunOnce()` currently calls `Hall_Buttons_UpdateAll()` without a deliberate delay.
- The loop runs as fast as the CPU can service the current app work and reacts to the latest DMA samples.

Output behavior:

- Firmware sets a gate GPIO high when the logical button is pressed.
- Firmware resets it low when released.
- Actual external switch/Brook active polarity still needs hardware validation.

## Current Button And Pin Map

Source of truth in code: `Core/Src/hall_buttons.c`.

The current map follows named schematic-style labels. Do not revert to older prototype assumptions.

| ID | Label | Hall input | ADC channel | Gate output |
|---:|---|---|---|---|
| 0 | `LEFT` | `PA4` / `LEFT_HE` | `ADC2_IN17` | `PA10` / `LEFT_GATE` |
| 1 | `DOWN` | `PA5` / `DOWN_HE` | `ADC2_IN13` | `PA9` / `DOWN_GATE` |
| 2 | `RIGHT` | `PA6` / `RIGHT_HE` | `ADC2_IN3` | `PA8` / `RIGHT_GATE` |
| 3 | `UP` | `PA7` / `UP_HE` | `ADC2_IN4` | `PC6` / `UP_GATE` |
| 4 | `SQUARE` | `PA0` / `SQUARE_HE` | `ADC1_IN1` | `PC7` / `SQUARE_GATE` |
| 5 | `TRIANGLE` | `PA1` / `TRIANGLE_HE` | `ADC1_IN2` | `PB3` / `TRIANGLE_GATE` |
| 6 | `L1` | `PA2` / `L1_HE` | `ADC1_IN3` | `PB4` / `L1_GATE` |
| 7 | `R1` | `PA3` / `R1_HE` | `ADC1_IN4` | `PB5` / `R1_GATE` |
| 8 | `CROSS` | `PC0` / `CROSS_HE` | `ADC1_IN6` | `PB6` / `CROSS_GATE` |
| 9 | `CIRCLE` | `PC1` / `CIRCLE_HE` | `ADC1_IN7` | `PB7` / `CIRCLE_GATE` |
| 10 | `L2` | `PC2` / `L2_HE` | `ADC1_IN8` | `PC9` / `L2_GATE` |
| 11 | `R2` | `PC3` / `R2_HE` | `ADC1_IN9` | `PC8` / `R2_GATE` |
| 12 | `L3` | `PC4` / `LMOD_HE` | `ADC2_IN5` | `PA11` / `L3_GATE` |
| 13 | `R3` | `PC5` / `RMOD_HE` | `ADC2_IN11` | `PB9` / `R3_GATE` |

Additional schematic/firmware pins:

| Pin | Current config/status | Planning note |
|---|---|---|
| `PB0` | GPIO input | Spare/user input candidate |
| `PB1` | GPIO input | Spare/user input candidate |
| `PB2` | GPIO input | Spare/user input candidate |
| `PA12` | Not configured in firmware | `MENU` on schematic; not yet in firmware |
| `PB10` | GPIO output | Display/control candidate: `DC` |
| `PB11` | GPIO output | Display/control candidate: `RST` |
| `PB12` | GPIO output | Display/control candidate: `CS` |
| `PB13` | AF `SPI2_SCK` | Reserved for possible SPI OLED/display |
| `PB15` | AF `SPI2_MOSI` | Reserved for possible SPI OLED/display |
| `PA13` | SWDIO | Debug/programming |
| `PA14` | SWCLK | Debug/programming |

Historical context:

- Earlier bring-up discussed a one-channel prototype path around `PA4 / ADC2_IN17`.
- The current named map supersedes that earlier prototype.
- In the current firmware `PA4 / ADC2_IN17` is `LEFT_HE`, and its gate output is `PA10 / LEFT_GATE`.
- `PA8` is currently `RIGHT_GATE`.

## What Is Not Present Yet

Do not claim these exist until implemented:

- OLED/display driver
- SPI display peripheral initialization beyond pin assignment
- OLED drawing primitives
- Settings page/menu logic
- Physical menu input handling
- Calibration workflow
- Rapid-trigger algorithm
- SOCD cleaning
- USB HID controller stack
- Timer-triggered ADC acquisition
- Per-button thresholds
- Per-button calibration data

There is no OLED/menu workflow yet, but the settings/profile model is ready for it to edit.

## Hardware Assumptions And Unknowns

Current assumptions:

- Hall sensors are single-ended analog outputs referenced to board ground.
- ADC channels should stay single-ended unless hardware notes explicitly say otherwise.
- Firmware currently treats higher ADC value as more pressed.
- Gate output high currently means logical pressed.
- Brook board handles USB/controller identity.
- External switching stage is gate-driven, but exact part/polarity must be confirmed.

Known unknowns:

- Brook board model.
- Exact external switch/MOSFET part number.
- Whether gate high actually closes the Brook input.
- Hall sensor part number.
- Hall sensor output voltage range.
- Whether ADC value truly increases with press on final hardware.
- Magnet orientation and mechanical travel.
- Expected rest ADC value per button.
- Expected fully pressed ADC value per button.
- OLED controller, resolution, and required pins.
- Menu/settings input method.
- Whether SOCD should be handled by STM32, Brook board, game, or not at all.

When implementing behavior that depends on an unknown, either ask the user or encode the assumption clearly in `AGENT.md` and `README.md`.

Mandatory hardware documentation rule:

- Never rely on memory or generic part knowledge for hardware behavior.
- Before implementing code that depends on any referenced hardware, inspect the relevant documentation online.
- Prefer primary sources: ST reference manuals/datasheets/application notes, Brook documentation, Hall sensor datasheets, OLED controller datasheets, MOSFET/switch datasheets, and the project schematic/PCB files if available.
- Record the exact hardware assumption or cited document in `AGENT.md`, `README.md`, or a dedicated hardware note before baking it into firmware.
- If the exact part number or schematic source is unknown, stop and ask the user instead of guessing.
- This applies especially to ADC electrical limits, Hall sensor transfer direction/range, MOSFET/gate polarity, Brook input behavior, OLED controller commands, flash layout/endurance, and STM32 peripheral/DMA behavior.

## CubeMX And Generated-Code Rules

CubeMX can overwrite generated sections.

Safe places:

- Separate application files: `app.c`, `board.c`, `settings.c`, `hall_buttons.c`, future modules.
- User blocks in generated files.

Risky places:

- `STM Projects.ioc`
- `Core/Src/stm32g4xx_hal_msp.c`
- generated HAL init patterns copied from CubeMX
- `EWARM/STM Projects.ewp` if CubeMX regenerates project metadata

If changing pin/peripheral config:

1. Update `STM Projects.ioc`.
2. Update `board.c` or generated init code as needed.
3. Update `stm32g4xx_hal_conf.h` if enabling a HAL module.
4. Update `EWARM/STM Projects.ewp` if new source files are added.
5. Update `README.md` if human-facing status/pin map changes.
6. Update this file if architecture, assumptions, or planning context changes.

## Agent Development Rules

Hard rules:

- Keep `main.c` tiny.
- Prefer coherent modules over adding logic to `main.c`.
- Inspect official online documentation for all referenced hardware before implementing hardware-dependent behavior.
- Keep Hall ADC inputs single-ended unless hardware evidence says otherwise.
- Keep named Hall/gate labels aligned with schematic comments in `hall_buttons.c`.
- Do not edit `Drivers/` unless there is a specific HAL/vendor reason.
- Avoid unrelated generated/vendor churn.
- Add every new `.c` file to `EWARM/STM Projects.ewp`.
- Document hardware assumptions.
- Tell the user when build verification could not be run.

Coding style:

- Follow the existing C style: simple functions, explicit structs, HAL types.
- Keep code ASCII.
- Use `uint32_t` for ADC values and threshold math.
- Clamp user/settings-driven ADC thresholds into `0..4095`.
- Keep public APIs small and named by module prefix: `Settings_`, `Hall_Buttons_`, `Board_`, `App_`.
- Avoid empty placeholder modules. Create a module when it owns real behavior.

Git/worktree:

- Check `git status` before editing.
- Do not revert user changes.
- Commit only when the user asks.
- Keep commits focused.

## Intended Module Architecture

Current modules:

```text
Core/Src/main.c          thin entry point
Core/Src/app.c           app init and run-once scheduling
Core/Src/board.c         HAL, clock, GPIO, ADC/DMA init/calibration/start
Core/Src/app_error.c     fail-stop error/assert handling
Core/Src/settings.c      settings/profile model
Core/Src/settings_storage.c flash persistence
Core/Src/hall_buttons.c  Hall ADC DMA sample mapping and gate state
```

Likely next modules:

```text
Core/Src/button_logic.c      actuation/release decision, rapid trigger, SOCD
Core/Src/outputs.c           gate GPIO abstraction
Core/Src/calibration.c       rest/pressed measurement and threshold generation
Core/Src/display.c           OLED/display driver integration
Core/Src/menu.c              settings page/menu state machine
Core/Src/debug.c             temporary raw ADC/status output if needed
```

Suggested separation:

- `hall_buttons.c` should eventually stop owning all button decision logic.
- `hall_buttons.c` can remain the physical channel map/acquisition facade.
- `button_logic.c` should own pressed/released decisions and rapid trigger.
- `outputs.c` should own gate polarity and GPIO writes.
- `settings.c` should own runtime settings/profile state.
- `settings_storage.c` should own flash persistence.
- `calibration.c` should own measuring rest/pressed ranges and generating defaults.
- `menu.c` should translate user actions into settings changes.
- `display.c` should render state; it should not own button logic.

## Planning Roadmap

Phase 0: preserve bring-up reliability

- Keep current 14-button DMA-backed scan working.
- Validate that the named map matches the actual PCB/schematic.
- Confirm gate polarity through hardware.
- Add a way to inspect raw ADC values.

Phase 1: button correctness

- Add per-button state/config structs.
- Add per-button thresholds.
- Add rest/pressed ADC recording.
- Decide whether higher ADC means pressed per button or globally.
- Add configurable active-high/active-low gate behavior if hardware requires it.

Phase 2: settings model

- Extend `ControllerProfile` for per-button values.
- Add a way to apply settings without resetting runtime state unexpectedly.
- Keep OLED/menu edits in RAM until the user explicitly saves or a debounced save occurs.

Phase 3: calibration

- Implement calibration mode behavior.
- Capture rest ADC values.
- Capture pressed ADC values or travel range.
- Derive actuation/release thresholds.
- Store calibration data in settings.

Phase 4: rapid trigger

- Define rapid trigger semantics in ADC units first.
- Track per-button dynamic baseline/last value.
- Implement press/release based on movement deltas.
- Preserve fixed-threshold fallback.
- Use `rapid_trigger_enabled` setting.

Phase 5: persistence

- Improve flash storage wear behavior if frequent saves become likely.
- Consider two-page sequence-numbered storage if profiles are saved often.
- Add defaults and migration behavior.
- Avoid wearing flash during normal scanning.

Phase 6: display/menu

- Confirm OLED controller and wiring.
- Enable the proper SPI HAL module.
- Initialize SPI/display control pins.
- Add display primitives.
- Add menu state machine.
- Wire menu actions to settings.

Phase 7: acquisition performance

- Move from software-start continuous scan to timer-triggered scan if needed.
- Revisit sampling time.
- Define target scan rate and latency budget.

## Context Log From Prior Discussion

Relevant context already established:

- The user wants this repo to be understandable by Codex from docs alone.
- The user explicitly asked for a human README plus an agent-focused `AGENT.md`.
- The user wants `AGENT.md` to be the entry point and planning source for agentic development.
- Earlier the project was a simple `main.c` prototype; it has since been split into modules.
- Earlier OLED/drawing code was checked and found not implemented.
- Settings page/menu logic was checked and found not implemented.
- A RAM settings backend now exists, but no menu/page/storage workflow exists.
- A flash-backed profile store now exists; menu/page editing workflow still does not.
- ADC reads were converted from blocking per-channel polling to continuous scan with circular DMA.
- The old deliberate `HAL_Delay(1)` in `App_RunOnce()` was removed after DMA conversion.
- Hall inputs should be single-ended for normal analog Hall sensors.
- The code should grow conservatively in sympathy with STM32Cube/IAR patterns.
- `main.c` should remain readable and small.
- New `.c` files must be included in the IAR `.ewp`.
- Hardware details should not be guessed.

## Verification Notes

Known local limitation:

- A command-line IAR/GCC ARM build has not been available in this environment in prior checks.
- Use IAR Embedded Workbench via `EWARM/Project.eww` for real build/debug verification.

Minimum checks after edits:

```powershell
git status --short --branch
rg -n "TODO|FIXME|Error_Handler|Hall_|Settings_|Board_|App_" Core
rg -n "app.c|board.c|settings.c|hall_buttons.c" "EWARM/STM Projects.ewp"
```

For source changes, inspect diffs:

```powershell
git diff -- Core README.md AGENT.md "STM Projects.ioc" "EWARM/STM Projects.ewp"
```

## Definition Of Done For Agent Work

A firmware task is not done until:

- The requested behavior is implemented.
- `main.c` remains small.
- New source files are added to `EWARM/STM Projects.ewp`.
- Pin/peripheral changes are reflected in code and `STM Projects.ioc`.
- Human-facing changes are reflected in `README.md`.
- Agent/planning/architecture changes are reflected here.
- Build verification is run, or the user is clearly told why it could not be run.
- No unrelated user or generated changes were reverted.
