/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include <stdio.h>
#include <stdbool.h>

#include <Found_I2C_Address.h>
#include <i2c_lcd.h>
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
DMA_HandleTypeDef hdma_adc1;

I2C_HandleTypeDef hi2c2;

TIM_HandleTypeDef htim2;

/* USER CODE BEGIN PV */
I2C_LCD_HandleTypeDef lcd; // LCD handler

char display[16]; // Display value

int displayMode; // Display mode value
int hour, minute, second, sec; // Time part value
int start; // Start mode value
int getTemplate[100], template, minTemplate, maxtemplate; // Template value

bool pompState, motorState, fastState; // Pomp, Motor and Fast modes value

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM2_Init(void);
static void MX_I2C2_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
	if(hadc == &hadc1) {
		for(int len = 0; len < 100; len++) {
			template += getTemplate[len]; // Get template
		}
		// Convert template
		template /= 100;
		template *= 3.3;
		template *= 100;
		template /= 4095;
	}
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if(htim == &htim2) {

		// Set clock(hour and minute and second)
		if(start == 1) {
			second--;
			if(second < 0 && minute != 0) {second = 59; minute--;}
			if(second < 0 && minute == 0) {second = 59; minute = 59; hour--;}
			if(minute < 0 && hour != 0) {minute = 59; hour--;}
			if(hour < 0) {hour = 0;}
		}
		sec++;
	}
}

void setting(void) { // Setting function

	//LCD initializes
	lcd.address = FoundAddres(&hi2c2);
	lcd.hi2c = &hi2c2;
	lcd_init(&lcd);
	lcd_clear(&lcd);

	// GPIO initializes
	// Input pins
	GPIO_InitTypeDef gpio;
	gpio.Pin = MODE_Pin | PLUS_Pin | MINUS_Pin | ENTER_Pin;
	gpio.Mode = GPIO_MODE_INPUT;
	gpio.Pull = GPIO_PULLUP;
	gpio.Speed = GPIO_SPEED_LOW;
	HAL_GPIO_Init(GPIOB, &gpio);
	// Output pins
	gpio.Pin = POMP_Pin | MOTOR_Pin | FAST_Pin;
	gpio.Mode = GPIO_MODE_OUTPUT_PP;
	gpio.Pull = GPIO_NOPULL;
	gpio.Speed = GPIO_SPEED_LOW;
	HAL_GPIO_Init(GPIOB, &gpio);
	// TIM2 initializes
	htim2.Init.AutoReloadPreload = DISABLE;
	htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV2;
	htim2.Init.Prescaler = 31999;
	htim2.Init.Period = 999;
	HAL_TIM_Base_Init(&htim2);

	//ADC initializes
	ADC_ChannelConfTypeDef adc;
	hadc1.Init.ContinuousConvMode = ENABLE;
	hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
	hadc1.Init.DiscontinuousConvMode = DISABLE;
	hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
	hadc1.Init.NbrOfConversion = 1;
	hadc1.Init.NbrOfDiscConversion = 0;
	hadc1.Init.ScanConvMode = DISABLE;
	adc.Channel = ADC_CHANNEL_0;
	adc.Rank = 1;
	adc.SamplingTime = ADC_SAMPLETIME_41CYCLES_5;
	HAL_ADC_Init(&hadc1);
	HAL_ADC_ConfigChannel(&hadc1, &adc);
	HAL_ADC_Start_DMA(&hadc1,(uint32_t *)getTemplate, 100);
}

void run(void) { // Run function (Turn on pomp, motor and fast)

	switch(start) {
	// Exit from function
	case 0:
		start = 0;
		sec = 0;

		HAL_TIM_Base_Stop_IT(&htim2);
		return;

	// Run timer mode
	case 1:
		HAL_TIM_Base_Start_IT(&htim2);

		if(sec < 4) {
			POMP_GPIO_Port->BSRR = POMP_Pin;
			MOTOR_GPIO_Port->BSRR = MOTOR_Pin;
			FAST_GPIO_Port->BSRR = FAST_Pin;
		}
		else {
			POMP_GPIO_Port->BSRR = POMP_Pin;
			MOTOR_GPIO_Port->BSRR = MOTOR_Pin;
			FAST_GPIO_Port->BSRR = FAST_Pin << 16;
		}
		if(hour <= 0 && minute <= 0 && second <= 0) {
			start = false;
			POMP_GPIO_Port->BSRR = POMP_Pin << 16;
			MOTOR_GPIO_Port->BSRR = MOTOR_Pin << 16;
			FAST_GPIO_Port->BSRR = FAST_Pin << 16;
		}
		break;

	// Run template mode
	case 2:
		if(template > maxtemplate) {
			HAL_TIM_Base_Start_IT(&htim2);
		}

		if(sec < 4) {
			POMP_GPIO_Port->BSRR = POMP_Pin;
			MOTOR_GPIO_Port->BSRR = MOTOR_Pin;
			FAST_GPIO_Port->BSRR = FAST_Pin;
		}
		else {
			POMP_GPIO_Port->BSRR = POMP_Pin;
			MOTOR_GPIO_Port->BSRR = MOTOR_Pin;
			FAST_GPIO_Port->BSRR = FAST_Pin << 16;
		}
		if(template < minTemplate) {
			POMP_GPIO_Port->BSRR = POMP_Pin << 16;
			MOTOR_GPIO_Port->BSRR = MOTOR_Pin << 16;
			FAST_GPIO_Port->BSRR = FAST_Pin << 16;

			HAL_TIM_Base_Stop_IT(&htim2);
			sec = 0;
			TIM2->CNT = 0;
			return;
		}
		break;

	default:
		start = 0;
		break;
	}
}

void manual_display(void) { //Manual mode display function
	// Show on LCD
	lcd_gotoxy(&lcd, 0, 0);
	lcd_puts(&lcd, "Manual Mode");

	// Show on LCD
	sprintf(display, "P=%01d M=%01d F=%01d", pompState, motorState, fastState);
	lcd_gotoxy(&lcd, 0, 1);
	lcd_puts(&lcd, display);

	// Check input pins
	if((PLUS_GPIO_Port->IDR & PLUS_Pin) == 0) {HAL_Delay(300); pompState = !pompState;}
	if((MINUS_GPIO_Port->IDR & MINUS_Pin) == 0) {HAL_Delay(300); motorState = !motorState;}
	if((ENTER_GPIO_Port->IDR & ENTER_Pin) == 0) {HAL_Delay(300); fastState = !fastState;}

	// Set output pins state
	//POMP
	if(pompState) POMP_GPIO_Port->BSRR = POMP_Pin;
	else POMP_GPIO_Port->BSRR = POMP_Pin << 16;
	//MOTOR
	if(motorState) MOTOR_GPIO_Port->BSRR = MOTOR_Pin;
	else MOTOR_GPIO_Port->BSRR = MOTOR_Pin << 16;
	//FAST
	if(fastState) FAST_GPIO_Port->BSRR = FAST_Pin;
	else FAST_GPIO_Port->BSRR = FAST_Pin << 16;
}

void time_setting(void) { // Time mode setting function
	HAL_Delay(500);

	lcd_clear(&lcd);

	displayMode = 0;

	while(1) {

		// Check ENTER pin for change setting mode
		if((ENTER_GPIO_Port->IDR & ENTER_Pin) == 0) {HAL_Delay(300); displayMode++; lcd_clear(&lcd);}

		switch(displayMode) {
		// Hour setting
		case 0:
			// Check PLUS and MINUS pins for increase or reduction hour
			if((PLUS_GPIO_Port->IDR & PLUS_Pin) == 0) {HAL_Delay(300); hour++;}
			if((MINUS_GPIO_Port->IDR & MINUS_Pin) == 0) {HAL_Delay(300); hour--;}

			if(hour > 23) hour = 0;
			if(hour < 0) hour = 23;

			sprintf(display, "Hour: %02d", hour);
			lcd_gotoxy(&lcd, 0, 0);
			lcd_puts(&lcd, display);
			break;

		// Minute setting
		case 1:
			// Check PLUS and MINUS pins for increase or reduction minute
			if((PLUS_GPIO_Port->IDR & PLUS_Pin) == 0) {HAL_Delay(300); minute++;}
			if((MINUS_GPIO_Port->IDR & MINUS_Pin) == 0) {HAL_Delay(300); minute--;}

			if(minute > 59) minute = 0;
			if(minute < 0) minute = 59;

			sprintf(display, "Minute: %02d", minute);
			lcd_gotoxy(&lcd, 0, 0);
			lcd_puts(&lcd, display);
			break;

		// Second setting
		case 2:
			// Check PLUS and MINUS pins for increase or reduction second
			if((PLUS_GPIO_Port->IDR & PLUS_Pin) == 0) {HAL_Delay(300); second++;}
			if((MINUS_GPIO_Port->IDR & MINUS_Pin) == 0) {HAL_Delay(300); second--;}

			if(second > 59) second = 0;
			if(second < 0) second = 59;

			sprintf(display, "Second: %02d", second);
			lcd_gotoxy(&lcd, 0, 0);
			lcd_puts(&lcd, display);
			break;

		case 3:
			lcd_clear(&lcd);
			lcd_gotoxy(&lcd, 0, 0);
			lcd_puts(&lcd, "Time setting");
			lcd_gotoxy(&lcd, 0, 1);
			lcd_puts(&lcd, "Saved");
			HAL_Delay(1000);
			lcd_clear(&lcd);
			displayMode = 1;
			return;
		}
	}
}

void time_display(void) { // Time mode display function

	// Check ENTER pin for go time_setting function
	if((ENTER_GPIO_Port->IDR & ENTER_Pin) == 0) {HAL_Delay(300); time_setting();}

	// Check PLUS pin for go run function
	if((PLUS_GPIO_Port->IDR & PLUS_Pin) == 0) {
		HAL_Delay(300);

		lcd_clear(&lcd);

		// While for print on LCD start question
		while(1) {
			// Check PLUS and MINUS pin for start or stop
			if((PLUS_GPIO_Port->IDR & PLUS_Pin) == 0) {HAL_Delay(300); start = 1; lcd_clear(&lcd); break;}
			if((MINUS_GPIO_Port->IDR & MINUS_Pin) == 0) {
				HAL_Delay(300);
				start = 0;
				lcd_clear(&lcd);
				POMP_GPIO_Port->BSRR = POMP_Pin << 16;
				MOTOR_GPIO_Port->BSRR = MOTOR_Pin << 16;
				FAST_GPIO_Port->BSRR = FAST_Pin << 16;
				break;
			}

			//Show on LCD
			lcd_gotoxy(&lcd, 0, 0);
			lcd_puts(&lcd, "Do you want to");
			lcd_gotoxy(&lcd, 0, 1);
			lcd_puts(&lcd, "Start ?");
		}
	}

	// Show on LCD
	lcd_gotoxy(&lcd, 0, 0);
	lcd_puts(&lcd, "Time Mode");
	sprintf(display, "%02d:%02d:%02d", hour, minute, second);
	lcd_gotoxy(&lcd, 0, 1);
	lcd_puts(&lcd, display);
}

void template_setting(void) {// Template mode setting function
	HAL_Delay(500);

	lcd_clear(&lcd);

	displayMode = 0;

	while(1) {

		// Check ENTER pin for change setting mode
		if((ENTER_GPIO_Port->IDR & ENTER_Pin) == 0) {HAL_Delay(300); displayMode++; lcd_clear(&lcd);}

		switch(displayMode) {
		// MAX template setting
		case 0:
			// Check PLUS and MINUS pins for increase or reduction template
			if((PLUS_GPIO_Port->IDR & PLUS_Pin) == 0) {HAL_Delay(300); maxtemplate++;}
			if((MINUS_GPIO_Port->IDR & MINUS_Pin) == 0) {HAL_Delay(300); maxtemplate--;}

			if(maxtemplate > 100) maxtemplate = 0;
			if(maxtemplate < 0) maxtemplate = 100;

			sprintf(display, "MAX T: %02d", maxtemplate);
			lcd_gotoxy(&lcd, 0, 0);
			lcd_puts(&lcd, display);
			break;

		// MIN template setting
		case 1:
			// Check PLUS and MINUS pins for increase or reduction hour
			if((PLUS_GPIO_Port->IDR & PLUS_Pin) == 0) {HAL_Delay(300); minTemplate++;}
			if((MINUS_GPIO_Port->IDR & MINUS_Pin) == 0) {HAL_Delay(300); minTemplate--;}

			if(minTemplate > 100) minTemplate = 0;
			if(minTemplate < 0) minTemplate = 100;

			sprintf(display, "MIN T: %02d", minTemplate);
			lcd_gotoxy(&lcd, 0, 0);
			lcd_puts(&lcd, display);
			break;

		case 2:
			lcd_clear(&lcd);
			lcd_gotoxy(&lcd, 0, 0);
			lcd_puts(&lcd, "Template");
			lcd_gotoxy(&lcd, 0, 1);
			lcd_puts(&lcd, " Setting saved");
			HAL_Delay(1000);
			lcd_clear(&lcd);
			displayMode = 2;
			return;
		}
	}
}

void template_display(void) { // Template display mode function
	// Check ENTER pin for go template_setting function
	if((ENTER_GPIO_Port->IDR & ENTER_Pin) == 0) {HAL_Delay(300); template_setting();}

	// Check PLUS pin for go run function
	if((PLUS_GPIO_Port->IDR & PLUS_Pin) == 0) {
		HAL_Delay(300);

		lcd_clear(&lcd);

		// While for print on LCD start question
		while(1) {
			// Check PLUS and MINUS pin for start or stop
			if((PLUS_GPIO_Port->IDR & PLUS_Pin) == 0) {HAL_Delay(300); start = 2; lcd_clear(&lcd); break;}
			if((MINUS_GPIO_Port->IDR & MINUS_Pin) == 0) {HAL_Delay(300); start = 0; lcd_clear(&lcd); break;}

			//Show on LCD
			lcd_gotoxy(&lcd, 0, 0);
			lcd_puts(&lcd, "Do you want to");
			lcd_gotoxy(&lcd, 0, 1);
			lcd_puts(&lcd, "Start ?");
		}
	}

	if(start == 2) {
		lcd_gotoxy(&lcd, 7, 1);
		lcd_puts(&lcd, "ON");
	}
	else if(start == 0) {
		lcd_gotoxy(&lcd, 7, 1);
		lcd_puts(&lcd, "OFF");
	}
	// Show on LCD
	lcd_gotoxy(&lcd, 0, 0);
	lcd_puts(&lcd, "Template mode");
	sprintf(display, "T: %02d%c", template, 223);
	lcd_gotoxy(&lcd, 0, 1);
	lcd_puts(&lcd, display);
}

void set_display(void) { // Set display mode function
	// Check input pin (MODE_pin) for change display mode
	if((MODE_GPIO_Port->IDR & MODE_Pin) == 0) {HAL_Delay(300); displayMode++; lcd_clear(&lcd);}

	// Set display mode
	switch(displayMode) {

	// Manual display
	case 0:
		manual_display();
		break;

	// Time display
	case 1:
		time_display();
		break;

	// Template display
	case 2:
		template_display();
		break;

	// Return to manual display
	case 3:
		displayMode = 0;
		break;
	}
}
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
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_TIM2_Init();
  MX_I2C2_Init();
  /* USER CODE BEGIN 2 */
  setting();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  set_display();
	  run();
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL4;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
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
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief I2C2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C2_Init(void)
{

  /* USER CODE BEGIN I2C2_Init 0 */

  /* USER CODE END I2C2_Init 0 */

  /* USER CODE BEGIN I2C2_Init 1 */

  /* USER CODE END I2C2_Init 1 */
  hi2c2.Instance = I2C2;
  hi2c2.Init.ClockSpeed = 100000;
  hi2c2.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c2.Init.OwnAddress1 = 0;
  hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.OwnAddress2 = 0;
  hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C2_Init 2 */

  /* USER CODE END I2C2_Init 2 */

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
  htim2.Init.Prescaler = 0;
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
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);

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

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, FAST_Pin|MOTOR_Pin|POMP_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : FAST_Pin MOTOR_Pin POMP_Pin */
  GPIO_InitStruct.Pin = FAST_Pin|MOTOR_Pin|POMP_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : ENTER_Pin MINUS_Pin PLUS_Pin MODE_Pin */
  GPIO_InitStruct.Pin = ENTER_Pin|MINUS_Pin|PLUS_Pin|MODE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

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
