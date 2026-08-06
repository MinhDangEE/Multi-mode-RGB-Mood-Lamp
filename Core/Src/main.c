/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "ssd1306_tests.h"
#include <string.h>
#include <stdio.h>
#include <math.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

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
/* USER CODE BEGIN PV */
typedef enum {
    STATE_SOLID,     // 1. MÀU TRẮNG TĨNH 100% (Solid White - All 3 LEDs PA6, PA7, PB0)
    STATE_BREATHING, // 2. THỞ MÀU TRẮNG (White Breathing - Gamma 2.2)
    STATE_RAINBOW,   // 3. CẦU VỒNG 6 GIAI ĐOẠN (Rainbow Spectrum)
    STATE_SLEEP      // 4. TẮT TẤT CẢ (Sleep Mode)
} SystemState;

SystemState current_state = STATE_SOLID;
uint8_t mode_changed_flag = 1;
int16_t breath_val = 0;
int8_t breath_step = 15;
uint8_t rx_byte = 0; // Biến nhận dữ liệu từ Terminal qua UART
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
  // Bật tất cả các kênh PWM của TIM3 cho LED RGB
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1); // Kênh Đỏ (PA6)
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2); // Kênh Xanh lá (PA7)
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3); // Kênh Xanh dương (PB0)

  // Bật ngắt nhận dữ liệu từ Terminal qua UART1
  HAL_UART_Receive_IT(&huart1, &rx_byte, 1);

  char startup_msg[] = "\r\n==================================================\r\n"
                       "  HE THONG DEN RGB MOOD LAMP - DIEU KHIEN TERMINAL\r\n"
                       "    Go '1': Nac 1 - MAU TRANG TINH (Solid White 100%)\r\n"
                       "    Go '2': Nac 2 - THO MAU TRANG (White Breathing Gamma 2.2)\r\n"
                       "    Go '3': Nac 3 - CAU VONG 6 GIAI DOAN (Rainbow Spectrum)\r\n"
                       "    Go '0': Nac 4 - TAT HET DEN (Sleep Mode)\r\n"
                       "    Go 'n' hoac Space: CHUYEN NAC TIEP THEO\r\n"
                       "==================================================\r\n";
  HAL_UART_Transmit(&huart1, (uint8_t*)startup_msg, strlen(startup_msg), 200);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    uint32_t now = HAL_GetTick();
    uint16_t max_bright = 999;

    // 1. Thông báo đổi chế độ qua UART
    if (mode_changed_flag == 1) {
        mode_changed_flag = 0;
        
        switch(current_state) {
            case STATE_SOLID: {
                char msg_solid[] = "Nac 1: SOLID WHITE (Mau Trang Tinh 100%)\r\n";
                HAL_UART_Transmit(&huart1, (uint8_t*)msg_solid, strlen(msg_solid), 100);
                break;
            }
            case STATE_BREATHING: {
                char msg_breath[] = "Nac 2: BREATHING WHITE (Tho Mau Trang Gamma 2.2)\r\n";
                HAL_UART_Transmit(&huart1, (uint8_t*)msg_breath, strlen(msg_breath), 100);
                break;
            }
            case STATE_RAINBOW: {
                char msg_rain[] = "Nac 3: RAINBOW SPECTRUM (Cau Vong 6 Giai Doan Chuyen Mau Muot)\r\n";
                HAL_UART_Transmit(&huart1, (uint8_t*)msg_rain, strlen(msg_rain), 100);
                break;
            }
            case STATE_SLEEP: {
                char msg_sleep[] = "Nac 4: SLEEP (TAT HET DEN)\r\n";
                HAL_UART_Transmit(&huart1, (uint8_t*)msg_sleep, strlen(msg_sleep), 100);
                break;
            }
        }
    }

    // 2. Điều khiển PWM theo Chế độ (Độ sáng MAX 100% Cố định)
    switch(current_state) {
        case STATE_SOLID: // NẤC 1: MÀU TRẮNG TĨNH 100% (CẢ 3 LED PA6, PA7, PB0)
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, max_bright);
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, max_bright);
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, max_bright);
            break;
            
        case STATE_BREATHING: { // NẤC 2: THỞ MÀU TRẮNG THEO CÁCH B (GAMMA 2.2)
            uint32_t t = now % 3000; // 1 chu kỳ thở = 3 giây (3000ms)
            float progress;
            if (t < 1500) {
                progress = (float)t / 1500.0f; // 0.0 -> 1.0 (Sáng dần)
            } else {
                progress = (float)(3000 - t) / 1500.0f; // 1.0 -> 0.0 (Tối dần)
            }
            
            // Áp dụng công thức Cách B: Gamma Correction 2.2 (x^2.2)
            float brightness_ratio = powf(progress, 2.2f);
            uint16_t pwm_val = (uint16_t)((float)max_bright * brightness_ratio);
            
            // Phát xung PWM đồng thời ra cả 3 màu -> MÀU TRẮNG THỞ MƯỢT
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, pwm_val);
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, pwm_val);
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, pwm_val);
            break;
        }
            
        case STATE_RAINBOW: { // NẤC 3: HIỆU ỨNG CẦU VỒNG 6 GIAI ĐOẠN (HSV/HSL Spectrum Crossfade)
            uint32_t t = now % 6000;   // 1 chu kỳ Cầu vồng = 6 giây (6000ms)
            uint32_t phase = t / 1000; // 0 -> 5 (6 Giai đoạn màu)
            uint32_t sub_t = t % 1000; // 0 -> 999ms trong từng giai đoạn
            
            uint16_t inc = (uint16_t)((sub_t * max_bright) / 1000); // Giá trị tăng 0 -> max_bright
            uint16_t dec = max_bright - inc;                        // Giá trị giảm max_bright -> 0
            
            uint16_t r = 0, g = 0, b = 0;
            switch(phase) {
                case 0: r = max_bright; g = inc;        b = 0;          break; // Đỏ -> Vàng
                case 1: r = dec;        g = max_bright; b = 0;          break; // Vàng -> Xanh lá
                case 2: r = 0;          g = max_bright; b = inc;        break; // Xanh lá -> Cyan
                case 3: r = 0;          g = dec;        b = max_bright; break; // Cyan -> Xanh dương
                case 4: r = inc;        g = 0;          b = max_bright; break; // Xanh dương -> Tím
                case 5: r = max_bright; g = 0;          b = dec;        break; // Tím -> Đỏ
            }
            
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, r); // PA6 (Đỏ)
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, g); // PA7 (Xanh lá)
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, b); // PB0 (Xanh dương)
            break;
        }

        case STATE_SLEEP: // NẤC 4: TẮT TẤT CẢ
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV2;
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

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.ContinuousConvMode = ENABLE;
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
  sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5; // Tăng thời gian lấy mẫu tối đa cho biến trở 100k
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

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
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
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
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
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
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pins : PB12 PB13 */
  GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : PA7 */
  GPIO_InitStruct.Pin = GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    uint32_t current_time = HAL_GetTick();
    static uint32_t last_pb12_time = 0;
    static uint32_t last_pb13_time = 0;

    // Xử lý nút Đổi chế độ (PB12) - Chống dội 300ms chính xác 100%
    if (GPIO_Pin == GPIO_PIN_12) { 
        if ((current_time - last_pb12_time) > 300) {
            current_state++;
            if (current_state > STATE_SLEEP) {
                current_state = STATE_SOLID;
            }
            mode_changed_flag = 1;
            last_pb12_time = current_time;
        }
    }
    
    // Xử lý nút Hẹn giờ (PB13)
    if (GPIO_Pin == GPIO_PIN_13) { 
        if ((current_time - last_pb13_time) > 300) {
            char msg_timer[] = "-> Nut Hen Gio (PB13) vua duoc nhan!\r\n";
            HAL_UART_Transmit(&huart1, (uint8_t*)msg_timer, strlen(msg_timer), 100);
            last_pb13_time = current_time;
        }
    }
}

// Callback ngắt nhận dữ liệu từ Terminal qua UART (Điều khiển LED bằng cách gõ phím)
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) {
        // Phản hồi ký tự gõ lại Terminal (Echo back)
        HAL_UART_Transmit(&huart1, &rx_byte, 1, 10);

        if (rx_byte == '1' || rx_byte == 'w' || rx_byte == 'W') {
            current_state = STATE_SOLID; // 1. MÀU TRẮNG TĨNH 100%
            mode_changed_flag = 1;
        } else if (rx_byte == '2' || rx_byte == 'b' || rx_byte == 'B') {
            current_state = STATE_BREATHING; // 2. THỞ MÀU TRẮNG (GAMMA 2.2)
            mode_changed_flag = 1;
        } else if (rx_byte == '3' || rx_byte == 'r' || rx_byte == 'R') {
            current_state = STATE_RAINBOW; // 3. CẦU VỒNG 6 GIAI ĐOẠN
            mode_changed_flag = 1;
        } else if (rx_byte == '0' || rx_byte == 's' || rx_byte == 'S') {
            current_state = STATE_SLEEP; // 4. TẮT TẤT CẢ
            mode_changed_flag = 1;
        } else if (rx_byte == 'n' || rx_byte == 'N' || rx_byte == ' ' || rx_byte == '\r' || rx_byte == '\n') {
            current_state++;
            if (current_state > STATE_SLEEP) current_state = STATE_SOLID;
            mode_changed_flag = 1;
        }
        
        // Tiếp tục đăng ký ngắt nhận ký tự tiếp theo
        HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
    }
}

// Tự động khôi phục ngắt UART nếu xảy ra lỗi tràn bộ nhớ (Overrun Error)
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
  /* User can add his own implementation to report the HAL error return state */
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
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
