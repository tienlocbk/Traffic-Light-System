/**
 * @file    lcd.h
 * @brief   HD44780 character LCD (16x2 / 20x4) driver through a PCF8574
 *          I2C backpack, for STM32F103 using the HAL I2C API.
 *
 * Ported from the Arduino LiquidCrystal_I2C library.
 *
 * Assumed (standard) PCF8574 backpack wiring:
 *     P0 = RS, P1 = RW, P2 = EN, P3 = Backlight, P4..P7 = D4..D7
 */
#ifndef LCD_H
#define LCD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx_hal.h"
#include <stdint.h>

/* ------------------------------------------------------------------ */
/* LCD I2C ADDRESS  (7-bit, NOT shifted)                                          */
/* ------------------------------------------------------------------ */
#define LCD_I2C_ADDRESS     0x21U

/* Default geometry */
#define LCD_COLS            16U
#define LCD_ROWS            2U

/* HD44780 ROM A00 degree symbol: LCD_PrintChar(&lcd, LCD_CHAR_DEGREE) */
#define LCD_CHAR_DEGREE     0xDFU

typedef struct
{
    I2C_HandleTypeDef *hi2c;
    uint16_t addr;              /* 8-bit (already shifted) address        */
    uint8_t  cols;
    uint8_t  rows;
    uint8_t  backlight;         /* current PCF8574 backlight bit          */
    uint8_t  displayfunction;
    uint8_t  displaycontrol;
    uint8_t  displaymode;
    uint8_t  ready;             /* 1 after a successful LCD_Init()        */
} LCD_t;

/**
 * @brief  Initialise the LCD (4-bit mode). Blocks ~100 ms.
 *         If the LCD does not ACK, lcd->ready stays 0 and every other
 *         LCD_* call returns immediately (so a missing LCD doesn't stall
 *         the program).
 * @param  addr7  7-bit I2C address, e.g. LCD_I2C_ADDRESS
 * @retval HAL_OK if the LCD answered, HAL_ERROR otherwise
 */
HAL_StatusTypeDef LCD_Init(LCD_t *lcd, I2C_HandleTypeDef *hi2c,
                           uint8_t addr7, uint8_t cols, uint8_t rows);

void LCD_Clear(LCD_t *lcd);
void LCD_Home(LCD_t *lcd);
void LCD_SetCursor(LCD_t *lcd, uint8_t col, uint8_t row);   /* 0-based */

void LCD_PrintChar(LCD_t *lcd, char c);
void LCD_PrintString(LCD_t *lcd, const char *str);
void LCD_PrintInt(LCD_t *lcd, int32_t value);
/** Prints value with `decimals` digits (0..4). No printf/%f needed. */
void LCD_PrintFloat(LCD_t *lcd, float value, uint8_t decimals);

void LCD_Backlight(LCD_t *lcd, uint8_t on);
void LCD_Command(LCD_t *lcd, uint8_t cmd);

#ifdef __cplusplus
}
#endif

#endif /* LCD_H */