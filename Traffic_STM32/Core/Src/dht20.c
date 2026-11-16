/**
 * @file    dht20.c
 * @brief   DHT20 driver implementation (STM32 HAL, I2C, polling).
 */
#include "dht20.h"
#include <string.h>

#define DHT20_ADDR_8BIT        ((uint16_t)(DHT20_I2C_ADDRESS << 1))
#define DHT20_I2C_TIMEOUT_MS   100U

#define DHT20_STATUS_BUSY      0x80U
#define DHT20_STATUS_CAL_MASK  0x18U
#define DHT20_MEASURE_WAIT_MS 80U

/* ------------------------------------------------------------------ */
/* Private helpers                                                     */
/* ------------------------------------------------------------------ */

/* CRC-8, poly 0x31, init 0xFF (DHT20 datasheet) */
static uint8_t dht20_crc8(const uint8_t *data, uint8_t len)
{
    uint8_t crc = 0xFF;

    while (len--)
    {
        crc ^= *data++;
        for (uint8_t i = 0; i < 8; i++)
        {
            if (crc & 0x80U)
                crc = (uint8_t)((crc << 1) ^ 0x31U);
            else
                crc = (uint8_t)(crc << 1);
        }
    }
    return crc;
}

static int dht20_read_status(DHT20_t *dev, uint8_t *status)
{
    if (HAL_I2C_Master_Receive(dev->hi2c, DHT20_ADDR_8BIT, status, 1,
                               DHT20_I2C_TIMEOUT_MS) != HAL_OK)
        return DHT20_ERROR_CONNECT;
    return DHT20_OK;
}

/* Re-initialise one calibration register (datasheet / Arduino lib procedure) */
static int dht20_reset_register(DHT20_t *dev, uint8_t reg)
{
    uint8_t tx[3] = { reg, 0x00, 0x00 };
    uint8_t rx[3] = { 0, 0, 0 };

    if (HAL_I2C_Master_Transmit(dev->hi2c, DHT20_ADDR_8BIT, tx, 3,
                                DHT20_I2C_TIMEOUT_MS) != HAL_OK)
        return DHT20_ERROR_CONNECT;
    HAL_Delay(5);

    if (HAL_I2C_Master_Receive(dev->hi2c, DHT20_ADDR_8BIT, rx, 3,
                               DHT20_I2C_TIMEOUT_MS) != HAL_OK)
        return DHT20_ERROR_CONNECT;
    HAL_Delay(10);

    tx[0] = (uint8_t)(0xB0U | reg);
    tx[1] = rx[1];
    tx[2] = rx[2];
    if (HAL_I2C_Master_Transmit(dev->hi2c, DHT20_ADDR_8BIT, tx, 3,
                                DHT20_I2C_TIMEOUT_MS) != HAL_OK)
        return DHT20_ERROR_CONNECT;
    HAL_Delay(5);

    return DHT20_OK;
}

/* Convert dev->raw[] into temperature / humidity, verify CRC */
static int dht20_convert(DHT20_t *dev)
{
    const uint8_t *b = dev->raw;

    if (dht20_crc8(b, 6) != b[6])
        return DHT20_ERROR_CHECKSUM;

    dev->status = b[0];

    /* 20-bit humidity: b1[7:0] b2[7:0] b3[7:4] */
    uint32_t raw_h = ((uint32_t)b[1] << 12) | ((uint32_t)b[2] << 4) | (b[3] >> 4);

    /* 20-bit temperature: b3[3:0] b4[7:0] b5[7:0] */
    uint32_t raw_t = ((uint32_t)(b[3] & 0x0FU) << 16) | ((uint32_t)b[4] << 8) | b[5];

    dev->humidity    = ((float)raw_h * 100.0f / 1048576.0f) + dev->hum_offset;
    dev->temperature = ((float)raw_t * 200.0f / 1048576.0f) - 50.0f + dev->temp_offset;

    return DHT20_OK;
}

/* ------------------------------------------------------------------ */
/* Public API                                                          */
/* ------------------------------------------------------------------ */

int DHT20_Init(DHT20_t *dev, I2C_HandleTypeDef *hi2c)
{
    memset(dev, 0, sizeof(*dev));
    dev->hi2c = hi2c;

    HAL_Delay(100);   /* datasheet: wait >= 100 ms after power-on */

    if (!DHT20_IsConnected(dev))
        return DHT20_ERROR_CONNECT;

    uint8_t status = 0;
    int rc = dht20_read_status(dev, &status);
    if (rc != DHT20_OK)
        return rc;

    /* Calibration bits (0x18) not set -> re-initialise registers */
    if ((status & DHT20_STATUS_CAL_MASK) != DHT20_STATUS_CAL_MASK)
    {
        (void)dht20_reset_register(dev, 0x1B);
        (void)dht20_reset_register(dev, 0x1C);
        (void)dht20_reset_register(dev, 0x1E);
        HAL_Delay(10);
    }

    return DHT20_OK;
}

uint8_t DHT20_IsConnected(DHT20_t *dev)
{
    return (HAL_I2C_IsDeviceReady(dev->hi2c, DHT20_ADDR_8BIT, 3,
                                  DHT20_I2C_TIMEOUT_MS) == HAL_OK) ? 1U : 0U;
}

int DHT20_Read(DHT20_t *dev)
{
    uint32_t now = HAL_GetTick();

    if (!dev->measuring)
    {
        /* Rate limit real measurements without delaying the scheduler. */
        if (dev->has_read &&
            (uint32_t)(now - dev->last_read_tick) < DHT20_MIN_READ_INTERVAL_MS)
            return DHT20_ERROR_LASTREAD;

        const uint8_t cmd[3] = { 0xAC, 0x33, 0x00 };
        if (HAL_I2C_Master_Transmit(dev->hi2c, DHT20_ADDR_8BIT,
                                    (uint8_t *)cmd, 3,
                                    DHT20_I2C_TIMEOUT_MS) != HAL_OK)
            return DHT20_ERROR_CONNECT;

        dev->has_read = 1;
        dev->last_read_tick = now;
        dev->measuring = 1;
        return DHT20_ERROR_BUSY;
    }

    /* The conversion takes at least 80 ms. Poll on a later task invocation. */
    if ((uint32_t)(now - dev->last_read_tick) < DHT20_MEASURE_WAIT_MS)
        return DHT20_ERROR_BUSY;

    if (HAL_I2C_Master_Receive(dev->hi2c, DHT20_ADDR_8BIT, dev->raw, 7,
                               DHT20_I2C_TIMEOUT_MS) != HAL_OK)
    {
        dev->measuring = 0;
        return DHT20_ERROR_CONNECT;
    }

    if ((dev->raw[0] & DHT20_STATUS_BUSY) != 0)
        return DHT20_ERROR_BUSY;

    dev->measuring = 0;

    /* Sanity check + convert. */
    uint8_t all_zero = 1;
    for (uint8_t i = 0; i < 7; i++)
    {
        if (dev->raw[i] != 0) { all_zero = 0; break; }
    }
    if (all_zero)
        return DHT20_ERROR_BYTES_ALL_ZERO;

    return dht20_convert(dev);
}

float DHT20_GetTemperature(const DHT20_t *dev) { return dev->temperature; }
float DHT20_GetHumidity(const DHT20_t *dev)    { return dev->humidity; }

void DHT20_SetTempOffset(DHT20_t *dev, float offset) { dev->temp_offset = offset; }
void DHT20_SetHumOffset(DHT20_t *dev, float offset)  { dev->hum_offset  = offset; }