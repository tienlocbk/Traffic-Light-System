/*
 * button.c - Button debouncing and reading
 * Samples every 10ms with 3-stage buffer to filter bouncing.
 */

#include "button.h"
#include "main.h"

/* GPIO port and pin lookup tables */
static GPIO_TypeDef * const button_ports[NUM_BUTTONS] = {
    BTN_1_GPIO_Port,
    BTN_2_GPIO_Port,
    BTN_3_GPIO_Port,
    BTN_4_GPIO_Port
};

static const uint16_t button_pins[NUM_BUTTONS] = {
    BTN_1_Pin,
    BTN_2_Pin,
    BTN_3_Pin,
    BTN_4_Pin
};

/* 3-stage debouncing shift buffers */
static GPIO_PinState filter_buf0[NUM_BUTTONS];
static GPIO_PinState filter_buf1[NUM_BUTTONS];
static GPIO_PinState filter_buf2[NUM_BUTTONS];

/* Debounced steady state of each button (default released = GPIO_PIN_SET) */
static GPIO_PinState debounced_state[NUM_BUTTONS];

/* Event flags */
static uint8_t button_pressed_flag[NUM_BUTTONS];
static uint8_t button_long_pressed_flag[NUM_BUTTONS];

/* Duration counters for long-press detection */
static uint16_t counter_press_duration[NUM_BUTTONS];
static uint8_t  long_press_reported[NUM_BUTTONS];

void button_init(void)
{
    for (uint8_t i = 0; i < NUM_BUTTONS; i++)
    {
        filter_buf0[i] = GPIO_PIN_SET;
        filter_buf1[i] = GPIO_PIN_SET;
        filter_buf2[i] = GPIO_PIN_SET;
        debounced_state[i] = GPIO_PIN_SET;

        button_pressed_flag[i] = 0;
        button_long_pressed_flag[i] = 0;
        counter_press_duration[i] = 0;
        long_press_reported[i] = 0;
    }
}

void button_reading(void)
{
    for (uint8_t i = 0; i < NUM_BUTTONS; i++)
    {
        /* Shift previous samples and read current pin level */
        filter_buf2[i] = filter_buf1[i];
        filter_buf1[i] = filter_buf0[i];
        filter_buf0[i] = HAL_GPIO_ReadPin(button_ports[i], button_pins[i]);

        /* 3-stage validation: all 3 consecutive readings (spanning 20-30 ms) must agree */
        if ((filter_buf0[i] == filter_buf1[i]) && (filter_buf1[i] == filter_buf2[i]))
        {
            /* If the confirmed reading is different from previous debounced state: edge detected! */
            if (filter_buf0[i] != debounced_state[i])
            {
                debounced_state[i] = filter_buf0[i];

                if (debounced_state[i] == GPIO_PIN_RESET)
                {
                    /* Transition: RELEASED -> PRESSED (falling edge for active-low) */
                    button_pressed_flag[i] = 1;
                    counter_press_duration[i] = 0;
                    long_press_reported[i] = 0;
                    /* Start measuring hold time on the next 10 ms sample. */
                    continue;
                }
                else
                {
                    /* Transition: PRESSED -> RELEASED */
                    counter_press_duration[i] = 0;
                    long_press_reported[i] = 0;
                }
            }
        }

        /* Measure elapsed debounced hold time, even if a noisy raw sample
         * has not yet confirmed a release. A confirmed release resets above.
         */
        if (debounced_state[i] == GPIO_PIN_RESET)
        {
            if (counter_press_duration[i] < TICKS_FOR_LONG_PRESS)
            {
                counter_press_duration[i]++;
            }
            if (counter_press_duration[i] >= TICKS_FOR_LONG_PRESS &&
                !long_press_reported[i])
            {
                button_long_pressed_flag[i] = 1;
                long_press_reported[i] = 1;
            }
        }
    }
}

uint8_t button_press_events(void)
{
    uint8_t events = 0;
    for (uint8_t i = BTN_1_INDEX; i <= BTN_4_INDEX; i++)
    {
        if (isButtonPressed(i)) events |= (uint8_t)(1U << i);
    }
    return events;
}

uint8_t isButtonPressed(uint8_t index)
{
    if (index >= NUM_BUTTONS)
    {
        return 0;
    }

    if (button_pressed_flag[index] == 1)
    {
        /* Auto-clear flag so the press event is processed exactly once */
        button_pressed_flag[index] = 0;
        return 1;
    }
    return 0;
}

uint8_t isButtonLongPressed(uint8_t index)
{
    if (index >= NUM_BUTTONS)
    {
        return 0;
    }

    if (button_long_pressed_flag[index] == 1)
    {
        button_long_pressed_flag[index] = 0;
        return 1;
    }
    return 0;
}

uint8_t isButtonHeld(uint8_t index)
{
    if (index >= NUM_BUTTONS)
    {
        return 0;
    }
    return (debounced_state[index] == GPIO_PIN_RESET) ? 1 : 0;
}

