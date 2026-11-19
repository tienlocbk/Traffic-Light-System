# Two-Road Traffic Light System

STM32F103RB implementation of a two-road traffic-light intersection for the
CO3009 Microprocessors - Microcontrollers course project. The project targets
the XNUCLEO-F103RB board and provides automatic, manual, and timing-tuning
modes, with DHT20 temperature/humidity display on a 16x2 I2C LCD.

## Hardware

The project is configured for **NUCLEO-F103RB / STM32F103RBTx**. The relevant
Arduino-header connections are:

| Function | Arduino pin | STM32 pin | GPIO label |
| --- | --- | --- | --- |
| Road 1 signal A | D3 | PB3 | `TL1_A` |
| Road 1 signal B | D4 | PB5 | `TL1_B` |
| Road 2 signal A | D5 | PB4 | `TL2_A` |
| Road 2 signal B | D6 | PB10 | `TL2_B` |
| Mode button | D7 | PA8 | `BTN_1` |
| Select/manual button | D8 | PA9 | `BTN_2` |
| Increase button | D9 | PC7 | `BTN_3` |
| Decrease/cancel button | D10 | PB6 | `BTN_4` |
| On-board LED | - | PA5 | `LD2` |

The DHT20 and LCD use I2C1 on PB8/PB9. The current driver configuration uses
DHT20 address `0x38` and LCD address `0x21` as 7-bit addresses. The LCD
backpack address may need to be updated after an I2C scan if different hardware
is used.

## Project structure

```text
Traffic_STM32/
├── Traffic_STM32.ioc          # STM32CubeMX configuration
├── Core/
│   ├── Inc/                    # Application and driver headers
│   └── Src/                    # Application, FSMs, scheduler, and drivers
├── Drivers/                   # STM32 HAL and CMSIS files
└── Debug/                     # STM32CubeIDE build output
```

Important modules:

- `scheduler.c/.h`: sorted relative-delay O(1) scheduler.
- `software_timer.c/.h`: 10 ms software-timer API for supporting-module tests.
- `button.c/.h`: active-low input sampling, debounce, press and long-press events.
- `traffic_light.c/.h`: two-pin, four-state traffic-light driver.
- `dht20.c/.h`: cooperative DHT20 measurement and conversion.
- `lcd.c/.h`: 16x2 HD44780 LCD through a PCF8574 I2C backpack.
- `fsm_automatic.c/.h`: alternating traffic phases.
- `fsm_manual.c/.h`: button-controlled phase changes.
- `fsm_tuning.c/.h`: green/amber duration adjustment; red is derived as green + amber.
- `fsm_mode.c/.h`: mode switching and all-red safety clearance.

The integrated FSM uses the shared `traffic.remaining_ms` countdown for phase
and mode transitions; the standalone software-timer module remains available
for scheduler-based module demonstrations and tests.

## Timing and scheduler

The scheduler tick is **10 ms**. The current clock configuration is:

- HSI divided by 2, multiplied by 16: 64 MHz system clock.
- APB1 prescaler: 2, giving a 64 MHz TIM2 clock on STM32F1.
- TIM2 prescaler: `7999`.
- TIM2 auto-reload period: `79`.

Therefore:

```text
T = (PSC + 1) * (ARR + 1) / f_TIM
  = 8000 * 80 / 64,000,000
  = 10 ms
```

`SCH_Update()` is called from the TIM2 interrupt and only processes the head
of the sorted relative-delay task list. `SCH_Dispatch_Tasks()` runs from the
main loop. Period zero is supported for one-shot tasks.

The integrated application schedules:

| Task | Initial delay | Period | Purpose |
| --- | ---: | ---: | --- |
| `button_reading` | 0 ticks | 1 tick | Sample and debounce buttons |
| `task_buttons` | 1 tick | 2 ticks | Forward button press events |
| `task_fsm` | 0 ticks | 1 tick | Run the mode/state machines |
| `task_lcd_sensor` | 10 ticks | 10 ticks | Poll DHT20 and refresh LCD |
| `task_heartbeat` | 3 ticks | 50 ticks | Toggle the on-board LED |

## Operating the system

Buttons are active-low and are debounced before events reach the FSM:

1. **BTN_1 / D7 - Mode**: Automatic -> Manual -> Tuning. In Tuning, BTN_1
   saves the new timing and returns to Automatic.
2. **BTN_2 / D8 - Select/manual**: Advance the phase in Manual mode; switch
   between green and amber values in Tuning mode.
3. **BTN_3 / D9 - Increase**: Increase the selected tuning value.
4. **BTN_4 / D10 - Decrease**: Decrease the selected tuning value.

Mode changes first enter a one-second all-red safety-clearance state. This
prevents both roads from becoming green during a transition.

Default timings are:

```text
Green = 3 s
Amber = 2 s
Red   = 5 s (Green + Amber)
```

The LCD displays temperature and humidity on the first row. The second row
shows the active mode, traffic colors, automatic countdowns, or the selected
tuning value.

## Build

1. Install STM32CubeIDE with the STM32F1 firmware package and an ST-LINK
   driver.
2. Open the repository as an existing STM32CubeIDE project, or import
   `Traffic_STM32/.project`.
3. Confirm that the selected board/device is **NUCLEO-F103RB /
   STM32F103RBTx**.
4. Confirm that SYS -> Debug is set to **Serial Wire**.
5. Select **Project -> Build Project**.

The generated build files are placed in `Traffic_STM32/Debug/`, including the
ELF, HEX, and BIN images.

## Flash and run

1. Connect the XNUCLEO-F103RB board through USB.
2. Build the project.
3. Start **Run As -> STM32 MCU C/C++ Application** in STM32CubeIDE, or use
   the configured ST-LINK debug/flash launch configuration.
4. Reset the board and verify the heartbeat LED, traffic-light sequence,
   button controls, LCD output, and DHT20 readings.

Keep SYS -> Debug set to Serial Wire so the board can be programmed repeatedly.


