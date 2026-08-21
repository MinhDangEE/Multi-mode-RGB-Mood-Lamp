/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body - Multi-Mode RGB Mood Lamp
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ssd1306.h"
#include "ssd1306_fonts.h"
#include <string.h>
#include <stdio.h>
#include <math.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum {
    STATE_SOLID = 0,
    STATE_BREATHING,
    STATE_RAINBOW,
    STATE_CUSTOM,
    STATE_SLEEP
} SystemState;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

I2C_HandleTypeDef hi2c1;

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
volatile SystemState current_state = STATE_SOLID;
volatile uint8_t mode_changed_flag = 1;

volatile uint32_t timer_seconds = 0;
volatile uint8_t timer_active = 0;
volatile uint8_t timer_display_update = 0;
uint32_t last_countdown_time = 0;

uint8_t custom_r = 255;
uint8_t custom_g = 255;
uint8_t custom_b = 255;

char uart_rx_buf[32];
uint8_t uart_rx_idx = 0;
uint8_t rx_byte = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);
static void MX_I2C1_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM3_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
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
  MX_ADC1_Init();
  MX_I2C1_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_USART1_UART_Init();

  /* USER CODE BEGIN 2 */
  /* Configure PWM pins */
  GPIO_InitTypeDef GPIO_InitStruct_PWM = {0};
  GPIO_InitStruct_PWM.Pin = GPIO_PIN_6 | GPIO_PIN_7;
  GPIO_InitStruct_PWM.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct_PWM.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct_PWM);
  
  GPIO_InitStruct_PWM.Pin = GPIO_PIN_0;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct_PWM);

  /* Start PWM channels for RGB LED */
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);

  /* Start ADC for brightness control */
  HAL_ADC_Start(&hadc1);

  /* Start UART receive interrupt */
  HAL_UART_Receive_IT(&huart1, &rx_byte, 1);

  /* Initialize OLED display */
  ssd1306_Init();
  ssd1306_Fill(Black);
  ssd1306_SetCursor(10, 10);
  ssd1306_WriteString("OLED Ready", Font_7x10, White);
  ssd1306_UpdateScreen();

  char startup_msg[] = "\r\n[SYSTEM] RGB Mood Lamp Controller Ready\r\n";
  HAL_UART_Transmit(&huart1, (uint8_t*)startup_msg, strlen(startup_msg), 100);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    uint32_t now = HAL_GetTick();

    /* Read potentiometer on PA1 */
    HAL_ADC_Stop(&hadc1);
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, 10);
    uint32_t adc_val = HAL_ADC_GetValue(&hadc1);
    uint16_t max_bright = (uint16_t)((adc_val * 999) / 4095);

    /* Update mode changes */
    if (mode_changed_flag == 1) {
        mode_changed_flag = 0;

        ssd1306_Fill(Black);
        ssd1306_SetCursor(0, 5);
        ssd1306_WriteString("MODE:", Font_7x10, White);

        switch (current_state) {
            case STATE_SOLID: {
                char msg[] = "Mode 1: Solid White\r\n";
                HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100);
                ssd1306_SetCursor(0, 20);
                ssd1306_WriteString("1. SOLID WHITE", Font_7x10, White);
                break;
            }
            case STATE_BREATHING: {
                char msg[] = "Mode 2: Breathing White\r\n";
                HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100);
                ssd1306_SetCursor(0, 20);
                ssd1306_WriteString("2. BREATHING", Font_7x10, White);
                break;
            }
            case STATE_RAINBOW: {
                char msg[] = "Mode 3: Rainbow Spectrum\r\n";
                HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100);
                ssd1306_SetCursor(0, 20);
                ssd1306_WriteString("3. RAINBOW", Font_7x10, White);
                break;
            }
            case STATE_CUSTOM: {
                char msg[48];
                snprintf(msg, sizeof(msg), "Mode 4: Custom RGB (%d,%d,%d)\r\n", custom_r, custom_g, custom_b);
                HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100);
                ssd1306_SetCursor(0, 20);
                char oled_str[32];
                snprintf(oled_str, sizeof(oled_str), "RGB:%d,%d,%d", custom_r, custom_g, custom_b);
                ssd1306_WriteString(oled_str, Font_7x10, White);
                break;
            }
            case STATE_SLEEP: {
                char msg[] = "Mode 0: Sleep\r\n";
                HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100);
                ssd1306_SetCursor(0, 20);
                ssd1306_WriteString("0. SLEEP OFF", Font_7x10, White);
                break;
            }
        }

        /* Render timer status on OLED */
        ssd1306_SetCursor(0, 45);
        char init_t_buf[32];
        if (timer_active) {
            uint32_t min = timer_seconds / 60;
            uint32_t sec = timer_seconds % 60;
            snprintf(init_t_buf, sizeof(init_t_buf), "Timer: %02lu:%02lu  ", (unsigned long)min, (unsigned long)sec);
        } else {
            snprintf(init_t_buf, sizeof(init_t_buf), "Timer: OFF    ");
        }
        ssd1306_WriteString(init_t_buf, Font_7x10, White);
        ssd1306_UpdateScreen();
    }

    /* Auto-off countdown timer execution */
    if (timer_active && (now - last_countdown_time >= 1000)) {
        last_countdown_time = now;
        if (timer_seconds > 0) {
            timer_seconds--;
            timer_display_update = 1;

            if (timer_seconds == 0) {
                timer_active = 0;
                current_state = STATE_SLEEP;
                mode_changed_flag = 1;

                char msg_end[] = "\r\n[TIMER] Expired -> Standby\r\n";
                HAL_UART_Transmit(&huart1, (uint8_t*)msg_end, strlen(msg_end), 100);
            }
        }
    }

    /* Update timer display on OLED */
    if (timer_display_update) {
        timer_display_update = 0;
        ssd1306_SetCursor(0, 45);
        char t_buf[32];
        if (timer_active) {
            uint32_t min = timer_seconds / 60;
            uint32_t sec = timer_seconds % 60;
            snprintf(t_buf, sizeof(t_buf), "Timer: %02lu:%02lu  ", (unsigned long)min, (unsigned long)sec);
        } else {
            snprintf(t_buf, sizeof(t_buf), "Timer: OFF    ");
        }
        ssd1306_WriteString(t_buf, Font_7x10, White);
        ssd1306_UpdateScreen();
    }

    /* Output PWM based on active state */
    switch (current_state) {
        case STATE_SOLID:
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, max_bright);
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, max_bright);
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, max_bright);
            break;

        case STATE_BREATHING: {
            uint32_t t = now % 3000;
            float progress;
            if (t < 1500) {
                progress = (float)t / 1500.0f;
            } else {
                progress = (float)(3000 - t) / 1500.0f;
            }

            float brightness_ratio = powf(progress, 2.2f);
            uint16_t pwm_val = (uint16_t)((float)max_bright * brightness_ratio);

            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, pwm_val);
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, pwm_val);
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, pwm_val);
            break;
        }

        case STATE_RAINBOW: {
            uint32_t t = now % 6000;
            uint32_t phase = t / 1000;
            uint32_t sub_t = t % 1000;

            uint16_t inc = (uint16_t)((sub_t * max_bright) / 1000);
            uint16_t dec = max_bright - inc;

            uint16_t r = 0, g = 0, b = 0;
            switch (phase) {
                case 0: r = max_bright; g = inc;        b = 0;          break;
                case 1: r = dec;        g = max_bright; b = 0;          break;
                case 2: r = 0;          g = max_bright; b = inc;        break;
                case 3: r = 0;          g = dec;        b = max_bright; break;
                case 4: r = inc;        g = 0;          b = max_bright; break;
                case 5: r = max_bright; g = 0;          b = dec;        break;
            }

            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, r);
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, g);
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, b);
            break;
        }

        case STATE_CUSTOM: {
            uint16_t eff_bright = (max_bright == 0) ? 999 : max_bright;
            uint16_t pwm_r = (uint16_t)(((uint32_t)custom_r * eff_bright) / 255);
            uint16_t pwm_g = (uint16_t)(((uint32_t)custom_g * eff_bright) / 255);
            uint16_t pwm_b = (uint16_t)(((uint32_t)custom_b * eff_bright) / 255);
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, pwm_r);
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, pwm_g);
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, pwm_b);
            break;
        }

        case STATE_SLEEP:
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0);
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 0);
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 0);
            break;
    }
  /* USER CODE END 3 */
  }
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
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
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{
  ADC_ChannelConfTypeDef sConfig = {0};

  hadc1.Instance = ADC1;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_1;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
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
}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{
  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 71;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 65535;
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
}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{
  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 71;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 999;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_TIM_MspPostInit(&htim3);
}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
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
  __HAL_RCC_AFIO_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /* Configure GPIO pins : PB12 (Mode) PB13 (Timer Cancel) */
  GPIO_InitStruct.Pin = GPIO_PIN_12 | GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* Configure GPIO pins : PB14 (DT) PB15 (CLK) for Rotary Encoder */
  GPIO_InitStruct.Pin = GPIO_PIN_14 | GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init */
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

/* USER CODE BEGIN 4 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    uint32_t current_time = HAL_GetTick();
    static uint32_t last_pb12_time = 0;
    static uint32_t last_pb13_time = 0;

    /* Mode button (PB12) */
    if (GPIO_Pin == GPIO_PIN_12) {
        if ((current_time - last_pb12_time) > 150) {
            current_state++;
            if (current_state > STATE_SLEEP) {
                current_state = STATE_SOLID;
            }
            mode_changed_flag = 1;
            last_pb12_time = current_time;
        }
    }

    /* Timer cancel button (PB13) */
    if (GPIO_Pin == GPIO_PIN_13) {
        if ((current_time - last_pb13_time) > 150) {
            timer_seconds = 0;
            timer_active = 0;
            timer_display_update = 1;

            char msg_cancel[] = "[TIMER] Off\r\n";
            HAL_UART_Transmit(&huart1, (uint8_t*)msg_cancel, strlen(msg_cancel), 100);

            last_pb13_time = current_time;
        }
    }

    /* Rotary Encoder (PB14 DT / PB15 CLK) */
    if (GPIO_Pin == GPIO_PIN_14 || GPIO_Pin == GPIO_PIN_15) {
        uint8_t clk = (uint8_t)HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_15);
        uint8_t dt  = (uint8_t)HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_14);
        uint8_t curr_state = (clk << 1) | dt;

        static uint8_t prev_state = 0x03;
        static int8_t enc_accum = 0;

        static const int8_t rot_table[16] = {
             0, -1,  1,  0,
             1,  0,  0, -1,
            -1,  0,  0,  1,
             0,  1, -1,  0
        };

        uint8_t index = (prev_state << 2) | curr_state;
        int8_t step = rot_table[index];
        prev_state = curr_state;

        if (step != 0) {
            enc_accum += step;

            if (enc_accum >= 2) {
                enc_accum = 0;
                timer_seconds += 10;
                if (timer_seconds > 36000) timer_seconds = 36000;
                timer_active = 1;
                timer_display_update = 1;

                char msg_enc[48];
                snprintf(msg_enc, sizeof(msg_enc), "[TIMER] Set: %lus (%02lum%02lus)\r\n", 
                         (unsigned long)timer_seconds, 
                         (unsigned long)(timer_seconds / 60), 
                         (unsigned long)(timer_seconds % 60));
                HAL_UART_Transmit(&huart1, (uint8_t*)msg_enc, strlen(msg_enc), 100);
            } else if (enc_accum <= -2) {
                enc_accum = 0;
                if (timer_seconds >= 10) {
                    timer_seconds -= 10;
                } else {
                    timer_seconds = 0;
                }
                if (timer_seconds == 0) {
                    timer_active = 0;
                }
                timer_display_update = 1;

                char msg_enc[48];
                snprintf(msg_enc, sizeof(msg_enc), "[TIMER] Set: %lus (%02lum%02lus)\r\n", 
                         (unsigned long)timer_seconds, 
                         (unsigned long)(timer_seconds / 60), 
                         (unsigned long)(timer_seconds % 60));
                HAL_UART_Transmit(&huart1, (uint8_t*)msg_enc, strlen(msg_enc), 100);
            }
        }
    }
}

/* UART receive callback */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) {
        uint8_t ch = rx_byte;
        HAL_UART_Receive_IT(&huart1, &rx_byte, 1);

        if (ch == '\n' || ch == '\r') {
            if (uart_rx_idx > 0) {
                uart_rx_buf[uart_rx_idx] = '\0';

                /* Custom RGB command "C:R,G,B" */
                if ((uart_rx_buf[0] == 'C' || uart_rx_buf[0] == 'c') && uart_rx_buf[1] == ':') {
                    int r = 0, g = 0, b = 0;
                    if (sscanf(&uart_rx_buf[2], "%d,%d,%d", &r, &g, &b) == 3) {
                        custom_r = (uint8_t)(r > 255 ? 255 : (r < 0 ? 0 : r));
                        custom_g = (uint8_t)(g > 255 ? 255 : (g < 0 ? 0 : g));
                        custom_b = (uint8_t)(b > 255 ? 255 : (b < 0 ? 0 : b));
                        current_state = STATE_CUSTOM;
                        mode_changed_flag = 1;
                    }
                }
                /* Hex RGB command "#RRGGBB" */
                else if (uart_rx_buf[0] == '#') {
                    unsigned int rgb = 0;
                    if (sscanf(&uart_rx_buf[1], "%x", &rgb) == 1) {
                        custom_r = (rgb >> 16) & 0xFF;
                        custom_g = (rgb >> 8) & 0xFF;
                        custom_b = rgb & 0xFF;
                        current_state = STATE_CUSTOM;
                        mode_changed_flag = 1;
                    }
                }
                /* Mode command "M:x" */
                else if ((uart_rx_buf[0] == 'M' || uart_rx_buf[0] == 'm') && uart_rx_buf[1] == ':') {
                    char m = uart_rx_buf[2];
                    if (m == '1') current_state = STATE_SOLID;
                    else if (m == '2') current_state = STATE_BREATHING;
                    else if (m == '3') current_state = STATE_RAINBOW;
                    else if (m == '4') current_state = STATE_CUSTOM;
                    else if (m == '0') current_state = STATE_SLEEP;
                    mode_changed_flag = 1;
                }
                /* Single key commands */
                else {
                    char c = uart_rx_buf[0];
                    if (c == '1' || c == 'w' || c == 'W') { current_state = STATE_SOLID; mode_changed_flag = 1; }
                    else if (c == '2' || c == 'b' || c == 'B') { current_state = STATE_BREATHING; mode_changed_flag = 1; }
                    else if (c == '3' || c == 'r' || c == 'R') { current_state = STATE_RAINBOW; mode_changed_flag = 1; }
                    else if (c == '4' || c == 'c' || c == 'C') { current_state = STATE_CUSTOM; mode_changed_flag = 1; }
                    else if (c == '0' || c == 's' || c == 'S') { current_state = STATE_SLEEP; mode_changed_flag = 1; }
                    else if (c == 'n' || c == 'N' || c == ' ') {
                        current_state++;
                        if (current_state > STATE_SLEEP) current_state = STATE_SOLID;
                        mode_changed_flag = 1;
                    }
                }
                uart_rx_idx = 0;
            }
        } else {
            if (ch >= 32 && ch <= 126) {
                if (uart_rx_idx < sizeof(uart_rx_buf) - 1) {
                    uart_rx_buf[uart_rx_idx++] = (char)ch;
                }
            }
        }
    }
}

/* UART error recovery callback */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) {
        __HAL_UART_CLEAR_PEFLAG(huart);
        __HAL_UART_CLEAR_FEFLAG(huart);
        __HAL_UART_CLEAR_NEFLAG(huart);
        __HAL_UART_CLEAR_OREFLAG(huart);
        HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
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
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
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
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */