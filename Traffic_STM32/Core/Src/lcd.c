/**
 * @file    lcd.c
 * @brief   HD44780 + PCF8574 I2C LCD driver (STM32 HAL, polling).
 */
#include "lcd.h"

/* HD44780 commands */
#define LCD_CMD_CLEARDISPLAY     0x01U
#define LCD_CMD_RETURNHOME       0x02U
#define LCD_CMD_ENTRYMODESET     0x04U
#define LCD_CMD_DISPLAYCONTROL   0x08U
#define LCD_CMD_FUNCTIONSET      0x20U
#define LCD_CMD_SETDDRAMADDR     0x80U

/* Entry mode flags */
#define LCD_ENTRYLEFT            0x02U
#define LCD_ENTRYSHIFTDECREMENT  0x00U

/* Display control flags */
#define LCD_DISPLAYON            0x04U
#define LCD_CURSOROFF            0x00U
#define LCD_BLINKOFF             0x00U

/* Function set flags */
#define LCD_4BITMODE             0x00U
#define LCD_1LINE                0x00U
#define LCD_2LINE                0x08U
#define LCD_5x8DOTS              0x00U

/* PCF8574 pin bits */
#define LCD_BIT_RS               0x01U
#define LCD_BIT_RW               0x02U
#define LCD_BIT_EN               0x04U
#define LCD_BIT_BL               0x08U

#define LCD_I2C_TIMEOUT_MS       50U

/* ------------------------------------------------------------------ */
/* Low level                                                           */
/* ------------------------------------------------------------------ */

/* Write one raw byte to the expander */
static HAL_StatusTypeDef lcd_expander_write(LCD_t *lcd, uint8_t data)
{
    data |= lcd->backlight;
    return HAL_I2C_Master_Transmit(lcd->hi2c, lcd->addr, &data, 1, LCD_I2C_TIMEOUT_MS);
}

/* Send one nibble (upper 4 bits of `nibble`) with an EN pulse */
static HAL_StatusTypeDef lcd_write4(LCD_t *lcd, uint8_t nibble)
{
    uint8_t buf[2];
    buf[0] = (uint8_t)((nibble | LCD_BIT_EN) | lcd->backlight);    /* EN high */
    buf[1] = (uint8_t)((nibble & ~LCD_BIT_EN) | lcd->backlight);   /* EN low  */
    return HAL_I2C_Master_Transmit(lcd->hi2c, lcd->addr, buf, 2, LCD_I2C_TIMEOUT_MS);
}

/* Send a full byte as two nibbles in a single I2C transaction.
 * mode = 0 for command, LCD_BIT_RS for data. Each byte on the bus lasts
 * ~90 us at 100 kHz, which comfortably satisfies the HD44780 EN timing
 * and the ~37 us command execution time. */
static HAL_StatusTypeDef lcd_send(LCD_t *lcd, uint8_t value, uint8_t mode)
{
    uint8_t hi = (uint8_t)(value & 0xF0U);
    uint8_t lo = (uint8_t)((value << 4) & 0xF0U);
    uint8_t bl = lcd->backlight;
    uint8_t buf[4];

    buf[0] = (uint8_t)(hi | mode | LCD_BIT_EN | bl);
    buf[1] = (uint8_t)(hi | mode | bl);
    buf[2] = (uint8_t)(lo | mode | LCD_BIT_EN | bl);
    buf[3] = (uint8_t)(lo | mode | bl);

    return HAL_I2C_Master_Transmit(lcd->hi2c, lcd->addr, buf, 4, LCD_I2C_TIMEOUT_MS);
}

/* ------------------------------------------------------------------ */
/* Public API                                                          */
/* ------------------------------------------------------------------ */

HAL_StatusTypeDef LCD_Init(LCD_t *lcd, I2C_HandleTypeDef *hi2c,
                           uint8_t addr7, uint8_t cols, uint8_t rows)
{
    lcd->hi2c      = hi2c;
    lcd->addr      = (uint16_t)(addr7 << 1);
    lcd->cols      = cols;
    lcd->rows      = rows;
    lcd->backlight = LCD_BIT_BL;
    lcd->ready     = 0;

    lcd->displayfunction = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS;
    if (rows > 1)
        lcd->displayfunction |= LCD_2LINE;

    HAL_Delay(50);   /* LCD power-up time (>40 ms) */

    /* Is the LCD there? If not, leave ready = 0 so nothing else blocks. */
    if (HAL_I2C_IsDeviceReady(hi2c, lcd->addr, 3, LCD_I2C_TIMEOUT_MS) != HAL_OK)
        return HAL_ERROR;

    lcd->ready = 1;

    /* Backlight on, all other lines low */
    lcd_expander_write(lcd, 0);
    HAL_Delay(100);

    /* HD44780 "initialisation by instruction" -> 4-bit mode */
    lcd_write4(lcd, 0x30);  HAL_Delay(5);
    lcd_write4(lcd, 0x30);  HAL_Delay(5);
    lcd_write4(lcd, 0x30);  HAL_Delay(1);
    lcd_write4(lcd, 0x20);  HAL_Delay(1);   /* switch to 4-bit */

    lcd_send(lcd, (uint8_t)(LCD_CMD_FUNCTIONSET | lcd->displayfunction), 0);

    lcd->displaycontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
    lcd_send(lcd, (uint8_t)(LCD_CMD_DISPLAYCONTROL | lcd->displaycontrol), 0);

    LCD_Clear(lcd);

    lcd->displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
    lcd_send(lcd, (uint8_t)(LCD_CMD_ENTRYMODESET | lcd->displaymode), 0);

    LCD_Home(lcd);
    return HAL_OK;
}

void LCD_Command(LCD_t *lcd, uint8_t cmd)
{
    if (!lcd->ready) return;
    lcd_send(lcd, cmd, 0);
}

void LCD_Clear(LCD_t *lcd)
{
    if (!lcd->ready) return;
    lcd_send(lcd, LCD_CMD_CLEARDISPLAY, 0);
    HAL_Delay(2);    /* clear takes ~1.5 ms */
}

void LCD_Home(LCD_t *lcd)
{
    if (!lcd->ready) return;
    lcd_send(lcd, LCD_CMD_RETURNHOME, 0);
    HAL_Delay(2);
}

void LCD_SetCursor(LCD_t *lcd, uint8_t col, uint8_t row)
{
    static const uint8_t row_offsets[4] = { 0x00, 0x40, 0x14, 0x54 };

    if (!lcd->ready) return;

    if (row >= lcd->rows) row = (uint8_t)(lcd->rows - 1U);
    if (row > 3U)         row = 3U;
    if (col >= lcd->cols) col = (uint8_t)(lcd->cols - 1U);

    lcd_send(lcd, (uint8_t)(LCD_CMD_SETDDRAMADDR | (col + row_offsets[row])), 0);
}

void LCD_PrintChar(LCD_t *lcd, char c)
{
    if (!lcd->ready) return;
    lcd_send(lcd, (uint8_t)c, LCD_BIT_RS);
}

void LCD_PrintString(LCD_t *lcd, const char *str)
{
    if (!lcd->ready || str == NULL) return;
    while (*str)
        lcd_send(lcd, (uint8_t)*str++, LCD_BIT_RS);
}

void LCD_PrintInt(LCD_t *lcd, int32_t value)
{
    char buf[12];
    uint8_t i = 0;
    uint32_t u;

    if (value < 0)
    {
        LCD_PrintChar(lcd, '-');
        u = (uint32_t)(-(int64_t)value);
    }
    else
    {
        u = (uint32_t)value;
    }

    do {
        buf[i++] = (char)('0' + (u % 10U));
        u /= 10U;
    } while (u != 0U);

    while (i)
        LCD_PrintChar(lcd, buf[--i]);
}

void LCD_PrintFloat(LCD_t *lcd, float value, uint8_t decimals)
{
    static const uint32_t pow10[5] = { 1U, 10U, 100U, 1000U, 10000U };

    if (decimals > 4U) decimals = 4U;

    uint8_t neg = (value < 0.0f) ? 1U : 0U;
    float a = neg ? -value : value;
    float scaled = a * (float)pow10[decimals] + 0.5f;

    if (scaled >= 2147483000.0f)      /* would overflow int32_t */
    {
        LCD_PrintString(lcd, "OVF");
        return;
    }

    uint32_t rounded = (uint32_t)scaled;
    uint32_t ipart   = rounded / pow10[decimals];
    uint32_t fpart   = rounded % pow10[decimals];

    if (neg && rounded != 0U)         /* avoid printing "-0.0" */
        LCD_PrintChar(lcd, '-');

    LCD_PrintInt(lcd, (int32_t)ipart);

    if (decimals > 0U)
    {
        LCD_PrintChar(lcd, '.');
        /* exactly `decimals` digits, leading zeros included */
        for (uint32_t d = pow10[decimals - 1U]; d >= 1U; d /= 10U)
            LCD_PrintChar(lcd, (char)('0' + ((fpart / d) % 10U)));
    }
}

void LCD_Backlight(LCD_t *lcd, uint8_t on)
{
    if (!lcd->ready) return;
    lcd->backlight = on ? LCD_BIT_BL : 0U;
    lcd_expander_write(lcd, 0);
}