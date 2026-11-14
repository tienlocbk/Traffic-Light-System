/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "scheduler.h"
#include "software_timer.h"
#include "button.h"
#include "traffic_light.h"
#include "dht20.h"
#include "lcd.h"
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* Hardware test steps (from Section 3, 4, 5 of project guide):
 * TEST_STEP_3_1: 3.1 Checking Program Flashing: Blink LED PA5
 * TEST_STEP_3_2: 3.2 Determining Signal Combinations for Light Modules
 * TEST_STEP_3_3: 3.3 Testing Buttons (BTN_1 to BTN_4, B1)
 * TEST_STEP_3_4: 3.4 Scanning I2C Devices (DHT20 & LCD)
 * TEST_STEP_4:   Step 4 O(1) Scheduler verification
 * TEST_STEP_5:   Step 5 Supporting Modules integrated demo on O(1) Scheduler
 */
#define TEST_STEP_3_1  1
#define TEST_STEP_3_2  2
#define TEST_STEP_3_3  3
#define TEST_STEP_3_4  4
#define TEST_STEP_4    5
#define TEST_STEP_5    6

/* >>> CHANGE THIS VALUE TO SWITCH TESTING STEP (1 to 6) <<< */
#define TEST_STEP      TEST_STEP_5
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
/* Global variables for Step 3.4 I2C device scan (accessible in Live Expressions) */
uint8_t found[8];
int n = 0;

/* Supporting module driver handles for DHT20 and LCD */
DHT20_t dht20;
LCD_t   lcd;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C1_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */
/* Step 5 demo tasks prototypes */
void task_step5_button_handler(void);
void task_step5_traffic_cycle(void);
void task_step5_dht20_lcd(void);
void task_step5_heartbeat(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
#if (TEST_STEP == TEST_STEP_5)
/* 
 * Step 5 Demo Modes:
 * 0: AUTO MODE   - Cycles traffic lights automatically using software_timer (3s Green, 1s Amber)
 * 1: MANUAL MODE - User controls lights directly with push buttons to test debouncing and lights
 */
static uint8_t demo_mode = 0;           /* 0 = Auto, 1 = Manual */
static uint8_t demo_traffic_phase = 0;  /* Auto cycle phase: 0..3 */

void task_step5_button_handler(void)
{
    /* Button 1 (D7 / PA8): Toggle between AUTO and MANUAL demo mode */
    if (isButtonPressed(BTN_1_INDEX))
    {
        HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
        demo_mode = !demo_mode;
        if (demo_mode == 0)
        {
            /* Switched back to AUTO: re-arm software timer */
            demo_traffic_phase = 0;
            traffic_set(ROAD_1, RED);
            traffic_set(ROAD_2, GREEN);
            setTimer(TIMER_TRAFFIC, 3000);
        }
        else
        {
            /* Switched to MANUAL: stop auto timer */
            clearTimer(TIMER_TRAFFIC);
        }
    }

    /* Long-press on Button 1 (> 1s): Toggle Amber warning blink */
    if (isButtonLongPressed(BTN_1_INDEX))
    {
        traffic_toggle(ROAD_1, AMBER);
        traffic_toggle(ROAD_2, AMBER);
    }

    /* In MANUAL mode, test individual buttons and lights */
    if (demo_mode == 1)
    {
        /* Button 2 (D8 / PA9): Step Road 1 (GREEN -> AMBER -> RED -> OFF) */
        if (isButtonPressed(BTN_2_INDEX))
        {
            TrafficColor next = (TrafficColor)((traffic_get(ROAD_1) + 1) % 4);
            traffic_set(ROAD_1, next);
        }

        /* Button 3 (D9 / PC7): Step Road 2 (GREEN -> AMBER -> RED -> OFF) */
        if (isButtonPressed(BTN_3_INDEX))
        {
            TrafficColor next = (TrafficColor)((traffic_get(ROAD_2) + 1) % 4);
            traffic_set(ROAD_2, next);
        }

        /* Button 4 (D10 / PB6): Toggle both roads ALL RED / ALL OFF */
        if (isButtonPressed(BTN_4_INDEX))
        {
            if (traffic_get(ROAD_1) == RED && traffic_get(ROAD_2) == RED)
            {
                traffic_all_off();
            }
            else
            {
                traffic_set(ROAD_1, RED);
                traffic_set(ROAD_2, RED);
            }
        }
    }

    /* Onboard USER Button (PC13): Toggle onboard LED LD2 */
    if (isButtonPressed(BTN_USER_INDEX))
    {
        HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
    }
}

void task_step5_traffic_cycle(void)
{
    /* Only auto-cycle in AUTO mode */
    if (demo_mode != 0)
    {
        return;
    }

    /* Check software timer flag (set by software_timer_run every 10ms) */
    if (timer_flag[TIMER_TRAFFIC] == 1)
    {
        switch (demo_traffic_phase)
        {
            case 0: /* Phase 0: Road 1 RED, Road 2 GREEN for 3 seconds */
                traffic_set(ROAD_1, RED);
                traffic_set(ROAD_2, GREEN);
                setTimer(TIMER_TRAFFIC, 3000);
                demo_traffic_phase = 1;
                break;

            case 1: /* Phase 1: Road 1 RED, Road 2 AMBER for 1 second */
                traffic_set(ROAD_1, RED);
                traffic_set(ROAD_2, AMBER);
                setTimer(TIMER_TRAFFIC, 1000);
                demo_traffic_phase = 2;
                break;

            case 2: /* Phase 2: Road 1 GREEN, Road 2 RED for 3 seconds */
                traffic_set(ROAD_1, GREEN);
                traffic_set(ROAD_2, RED);
                setTimer(TIMER_TRAFFIC, 3000);
                demo_traffic_phase = 3;
                break;

            case 3: /* Phase 3: Road 1 AMBER, Road 2 RED for 1 second */
                traffic_set(ROAD_1, AMBER);
                traffic_set(ROAD_2, RED);
                setTimer(TIMER_TRAFFIC, 1000);
                demo_traffic_phase = 0;
                break;

            default:
                demo_traffic_phase = 0;
                setTimer(TIMER_TRAFFIC, 100);
                break;
        }
    }
}

void task_step5_dht20_lcd(void)
{
    /* Helper names for LCD debug display */
    static const char *color_names[] = { "OFF", "GRN", "AMB", "RED" };

    if (DHT20_Read(&dht20) == DHT20_OK && lcd.ready)
    {
        char line1[17];
        char line2[17];
        snprintf(line1, sizeof(line1), "T:%.1fC  H:%.1f%%",
                 dht20.temperature, dht20.humidity);
        snprintf(line2, sizeof(line2), "%s R1:%s R2:%s",
                 (demo_mode == 0) ? "AUTO" : "MANU",
                 color_names[traffic_get(ROAD_1)],
                 color_names[traffic_get(ROAD_2)]);
        LCD_SetCursor(&lcd, 0, 0);
        LCD_PrintStr(&lcd, line1);
        LCD_SetCursor(&lcd, 1, 0);
        LCD_PrintStr(&lcd, line2);
    }
}

void task_step5_heartbeat(void)
{
    /* Heartbeat blink on PA5 indicates scheduler is active */
    HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
}
#endif
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_I2C1_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
#if (TEST_STEP == TEST_STEP_5)
  /* ------------------------------------------------------------------
   * Step 5: Supporting Modules Demo running on O(1) Scheduler
   * ------------------------------------------------------------------ */
  SCH_Init();
  software_timer_init();
  button_init();
  traffic_init();

  /* Start TIM2 interrupt for scheduler 10 ms ticks */
  HAL_TIM_Base_Start_IT(&htim2);

  /* Set initial traffic software timer (3 seconds) */
  setTimer(TIMER_TRAFFIC, 3000);

  /* Register periodic tasks in the O(1) scheduler:
   * 1. software_timer_run: decrements software timers every 10 ms (1 tick)
   * 2. button_reading: 3-stage debounces buttons every 10 ms (1 tick)
   * 3. task_step5_button_handler: checks button presses every 20 ms (2 ticks)
   * 4. task_step5_traffic_cycle: updates traffic light phases every 50 ms (5 ticks)
   * 5. task_step5_heartbeat: blinks LD2 every 500 ms (50 ticks)
   */
  SCH_Add_Task(software_timer_run, 0, 1);
  SCH_Add_Task(button_reading, 0, 1);
  SCH_Add_Task(task_step5_button_handler, 1, 2);
  SCH_Add_Task(task_step5_traffic_cycle, 2, 5);
  SCH_Add_Task(task_step5_heartbeat, 3, 50);

  /* Initialize DHT20 and LCD if available */
  if (LCD_Init(&lcd, &hi2c1, LCD_I2C_ADDRESS, 16, 2) == HAL_OK) {
      LCD_Clear(&lcd);
      LCD_SetCursor(&lcd, 0, 0);
      LCD_PrintStr(&lcd, "Step 5 Running");
  }
  DHT20_Init(&dht20, &hi2c1);
  SCH_Add_Task(task_step5_dht20_lcd, 50, 200); /* Update every 2 seconds */

#elif (TEST_STEP == TEST_STEP_4)
  /* Step 4: O(1) Scheduler verification */
  HAL_TIM_Base_Start_IT(&htim2);
  SCH_Init();
  SCH_Add_Task(task_step5_heartbeat, 0, 50);
#endif
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
#if (TEST_STEP == TEST_STEP_3_1)
    /* ------------------------------------------------------------------
     * Step 3.1: Checking Program Flashing: Blink LED PA5 (LD2)
     * ------------------------------------------------------------------ */
    HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
    HAL_Delay(500);

#elif (TEST_STEP == TEST_STEP_3_2)
    /* ------------------------------------------------------------------
     * Step 3.2: Determining the Signal Combinations for the Light Modules
     * Cycles through (0,0), (0,1), (1,0), (1,1) holding each for 2 seconds
     * for both Traffic Light module 1 (TL1) and module 2 (TL2).
     * ------------------------------------------------------------------ */
    for (int i = 0; i < 4; i++) {
      HAL_GPIO_WritePin(TL1_A_GPIO_Port, TL1_A_Pin, (i >> 1) & 1);
      HAL_GPIO_WritePin(TL1_B_GPIO_Port, TL1_B_Pin, i & 1);

      HAL_GPIO_WritePin(TL2_A_GPIO_Port, TL2_A_Pin, (i >> 1) & 1);
      HAL_GPIO_WritePin(TL2_B_GPIO_Port, TL2_B_Pin, i & 1);

      HAL_Delay(2000);
    }

#elif (TEST_STEP == TEST_STEP_3_3)
    /* ------------------------------------------------------------------
     * Step 3.3: Testing the Buttons
     * Read each button and show the result on LED PA5 (LD2).
     * Turns LD2 ON when any button (BTN_1..4, B1) is pressed (RESET / 0).
     * ------------------------------------------------------------------ */
    if (HAL_GPIO_ReadPin(BTN_1_GPIO_Port, BTN_1_Pin) == GPIO_PIN_RESET ||
        HAL_GPIO_ReadPin(BTN_2_GPIO_Port, BTN_2_Pin) == GPIO_PIN_RESET ||
        HAL_GPIO_ReadPin(BTN_3_GPIO_Port, BTN_3_Pin) == GPIO_PIN_RESET ||
        HAL_GPIO_ReadPin(BTN_4_GPIO_Port, BTN_4_Pin) == GPIO_PIN_RESET ||
        HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin) == GPIO_PIN_RESET)
    {
      HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
    }
    else
    {
      HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
    }

#elif (TEST_STEP == TEST_STEP_3_4)
    /* ------------------------------------------------------------------
     * Step 3.4: Scanning I2C Devices
     * Scans 7-bit addresses 1..127 on I2C1 for DHT20 and LCD.
     * Found addresses are stored in the global 'found' array.
     * Also blinks LD2 'n' times (e.g. 2 times) as immediate visual feedback.
     * ------------------------------------------------------------------ */
    n = 0;
    for (uint8_t addr = 1; addr < 128; addr++) {
      if (HAL_I2C_IsDeviceReady(&hi2c1, addr << 1, 1, 10) == HAL_OK) {
        if (n < 8) found[n++] = addr; // Set breakpoint here or inspect 'found'
      }
    }

    /* Visual feedback: blink LD2 'n' times (once for each device found) */
    for (int i = 0; i < n; i++) {
      HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
      HAL_Delay(150);
      HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
      HAL_Delay(150);
    }
    HAL_Delay(1000);

#elif (TEST_STEP == TEST_STEP_5 || TEST_STEP == TEST_STEP_4)
    /* ------------------------------------------------------------------
     * Steps 4 & 5: Dispatch ready tasks from the O(1) scheduler queue
     * ------------------------------------------------------------------ */
    SCH_Dispatch_Tasks();

#endif
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL16;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 7199;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 99;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, TL2_B_Pin|TL1_A_Pin|TL2_A_Pin|TL1_B_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : TL2_B_Pin TL1_A_Pin TL2_A_Pin TL1_B_Pin */
  GPIO_InitStruct.Pin = TL2_B_Pin|TL1_A_Pin|TL2_A_Pin|TL1_B_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : BTN_3_Pin */
  GPIO_InitStruct.Pin = BTN_3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(BTN_3_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : BTN_1_Pin BTN_2_Pin */
  GPIO_InitStruct.Pin = BTN_1_Pin|BTN_2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : BTN_4_Pin */
  GPIO_InitStruct.Pin = BTN_4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(BTN_4_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

}

/* USER CODE BEGIN 4 */

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM2)
  {
    SCH_Update();
  }
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
