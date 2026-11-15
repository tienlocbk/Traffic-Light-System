/**
 * @file    dht20.h
 * @brief   DHT20 (AHT20-compatible) temperature / humidity sensor driver
 *          for STM32F103 using the HAL I2C API.
 *
 * Ported from the Arduino/ESP32 DHT20 library.
 * Sensor protocol follows the DHT20 datasheet:
 *   - 7-bit I2C address 0x38
 *   - Init: wait >=100 ms after power-up, read status; if (status & 0x18) != 0x18
 *           the calibration registers 0x1B, 0x1C, 0x1E must be re-initialised
 *   - Measure: send 0xAC 0x33 0x00, wait ~80 ms, read 7 bytes
 *              [status, H19:12, H11:4, H3:0|T19:16, T15:8, T7:0, CRC8]
 *   - Humidity    [%RH] = raw_h / 2^20 * 100
 *   - Temperature [degC] = raw_t / 2^20 * 200 - 50
 *
 * NOTE: DHT20_Read() is cooperative. One call starts a measurement and a
 *       later call collects its result, so it does not block the scheduler.
 */
#ifndef DHT20_H
#define DHT20_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx_hal.h"
#include <stdint.h>

/* 7-bit address (the driver shifts it left by 1 for HAL) */
#define DHT20_I2C_ADDRESS             0x38U

/* Minimum time between two real measurements (datasheet: >= 1 s is advised) */
#define DHT20_MIN_READ_INTERVAL_MS    1000U

/* Return codes */
#define DHT20_OK                       0
#define DHT20_ERROR_CHECKSUM         (-10)
#define DHT20_ERROR_CONNECT          (-11)
#define DHT20_ERROR_BYTES_ALL_ZERO   (-13)
#define DHT20_ERROR_BUSY             (-14)   /* sensor still measuring after retries */
#define DHT20_ERROR_LASTREAD         (-15)   /* called again before MIN_READ_INTERVAL */

typedef struct
{
    I2C_HandleTypeDef *hi2c;     /* HAL handle, e.g. &hi2c1                */

    float    temperature;        /* last good value [degC], offset applied */
    float    humidity;           /* last good value [%RH],  offset applied */
    float    temp_offset;        /* user calibration offset [degC]         */
    float    hum_offset;         /* user calibration offset [%RH]          */

    uint32_t last_read_tick;     /* HAL_GetTick() of last measurement      */
    uint8_t  has_read;           /* 0 until the first measurement started  */
    uint8_t  measuring;          /* conversion is in progress              */
    uint8_t  status;             /* status byte of last measurement        */
    uint8_t  raw[7];             /* raw bytes of last measurement          */
} DHT20_t;

/**
 * @brief  Initialise the driver and the sensor (blocks ~100 ms for power-up).
 * @param  dev   driver instance
 * @param  hi2c  initialised HAL I2C handle (e.g. &hi2c1)
 * @retval DHT20_OK or DHT20_ERROR_CONNECT
 */
int  DHT20_Init(DHT20_t *dev, I2C_HandleTypeDef *hi2c);

/** @brief  Returns 1 if the sensor ACKs its address, 0 otherwise. */
uint8_t DHT20_IsConnected(DHT20_t *dev);

/**
 * @brief  Start or poll a measurement, read + convert + CRC check.
 *         Safe to call from a periodic scheduler task: the trigger and
 *         result collection happen on separate calls. If called again before
 *         DHT20_MIN_READ_INTERVAL_MS has elapsed it returns
 *         DHT20_ERROR_LASTREAD immediately and keeps the previous values.
 * @retval DHT20_OK or one of the DHT20_ERROR_* codes
 */
int  DHT20_Read(DHT20_t *dev);

float DHT20_GetTemperature(const DHT20_t *dev);   /* [degC] */
float DHT20_GetHumidity(const DHT20_t *dev);      /* [%RH]  */

void  DHT20_SetTempOffset(DHT20_t *dev, float offset);
void  DHT20_SetHumOffset(DHT20_t *dev, float offset);

#ifdef __cplusplus
}
#endif

#endif /* DHT20_H */