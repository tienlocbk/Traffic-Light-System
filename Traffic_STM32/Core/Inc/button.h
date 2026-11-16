#ifndef INC_BUTTON_H_
#define INC_BUTTON_H_

#include "main.h"
#include <stdint.h>

#define NUM_BUTTONS            4

/* Button indices */
#define BTN_1_INDEX            0   /* D7  - PA8  (Mode) */
#define BTN_2_INDEX            1   /* D8  - PA9  (Manual / Adjust) */
#define BTN_3_INDEX            2   /* D9  - PC7  (Set) */
#define BTN_4_INDEX            3   /* D10 - PB6  (Cancel / Confirm) */

#define BTN_MODE               BTN_1_INDEX
#define BTN_SELECT             BTN_2_INDEX
#define BTN_SET                BTN_3_INDEX
#define BTN_CANCEL             BTN_4_INDEX

/* 100 ticks * 10ms = 1 second for long press */
#define TICKS_FOR_LONG_PRESS   100

/* Initialize button buffers */
void button_init(void);

/* Read pins and debounce, called every 10ms by scheduler */
void button_reading(void);

/* Check if button was pressed (returns 1 and clears flag) */
uint8_t isButtonPressed(uint8_t index);

/* Check if button is held for more than 1 second */
uint8_t isButtonLongPressed(uint8_t index);

/* Get current debounced state (1 if pressed, 0 if released) */
uint8_t isButtonHeld(uint8_t index);

/* Aliases for lab compatibility */
#define button_read(index)        isButtonPressed(index)
#define is_button_pressed(index)  isButtonPressed(index)
#define is_button_pressed_1s(idx) isButtonLongPressed(idx)

#endif /* INC_BUTTON_H_ */
