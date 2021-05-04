/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
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
#include "cardata.h"

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

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart2_rx;

/* USER CODE BEGIN PV */
volatile enum Notch notch = N;
volatile enum Mode mode = DEMO;
volatile enum HallState hallstate = STOP;
volatile enum InvState invstate = INVOFF;
const enum InputMode inputmode = SERIAL;
const enum CtrlMode ctrlmode = SPEAKER;

volatile uint32_t start = 0;  // for process time measurement
volatile uint32_t stop = 0;   // for process time measurement
uint32_t theta_est = 0;  // estimated rotor electric angle (65536^2/2pi * omega[rad])
uint32_t theta_u = 0;    // U voltage phase angle (65536^2/2pi * omega[rad])
float omega_est = 0.0;  // estimated rotor electric angular velocity [rad/s]
float omega_ref = 0.0;  // (for demo mode only) omega command [rad/s]
float CtrlPrd = 0.0;    // control period [s]
float speed = 0.0;      // bicycle speed [km/h]
float fs = 0.0;  // estimated rotor electric frequency [Hz]
float fc = INITIALFC;  // carrier frequency (include random modulation) [Hz]
float fc0 = INITIALFC; // carrier frequency (exclude random modulation) [Hz]
float frand = 0.0;     // random modulation frequency
float Vd = 0.0;  // d-axis voltage command [-1 1]
float Vq = 0.0;  // q-axis voltage command [-1 1]
float Vs = 0.0;  // signal voltage =sqrt(Vd^2+Vq^2) [-1 1]
float Vdc = 36.0;
float Id = 0.0;
float Iq = 0.0;
float Iac = 0.0;
float acc = 0.0;  // (for demo) acceleration set to change omega_ref [rad/s/s]
int pulsemode = 0;  // pulse mode
int pmNo = 0;       // pulse mode index
int pmNo_ref = 0;
int pulsemode_ref = 0;
static uint8_t dma_rx_buf[RXBUFFERSIZE];  // rx buffer written by DMA
static uint32_t rd_ptr, dma_write_ptr;  // rx pointer
uint8_t sector = 0;

/* array for pulse pattern data */
float list_fs[PULSEMODESIZE], list_fc1[PULSEMODESIZE], list_fc2[PULSEMODESIZE], list_frand1[PULSEMODESIZE], list_frand2[PULSEMODESIZE];
int list_pulsemode[PULSEMODESIZE], pulsenum;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_TIM1_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */
int USART2_RX_IsEmpty(void);
uint8_t USART2_RX_Read(void);
void decode_pulsemode(const char*);
void inv_reset(void);
float newton_downslope(float, float, float, uint32_t);
float newton_upslope(float, float, float, uint32_t);
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
  MX_DMA_Init();
  MX_TIM1_Init();
  MX_USART2_UART_Init();
  MX_ADC1_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */

  /* to measure process time (if you want to get CPU cycle, access DWT->CYCCNT) */
  CoreDebug -> DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT -> CTRL |= DWT_CTRL_CYCCNTENA_Msk;

  /* initialize TIM1 parameters */
  TIM1->PSC = PRSC-1;
  TIM1->ARR = (uint16_t)((float)FCLK/2/PRSC/fc);
  CtrlPrd = 1.0/2.0/fc;

  /* start peripherals */
  HAL_UART_Receive_DMA(&huart2, dma_rx_buf, RXBUFFERSIZE);
  HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED);
  HAL_ADCEx_InjectedStart_IT(&hadc1);  // ADC sampling and control interrupt
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);
//  HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_1);
//  HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_2);
//  HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_3);
  if (ctrlmode == HALL) {
	  HAL_TIM_IC_Start(&htim2, TIM_CHANNEL_1);  // hall sensor input capture
	  HAL_TIM_Base_Start_IT(&htim2);            // hall sensor transition interrupt
  } else {
	  HAL_TIM_IC_Stop(&htim2, TIM_CHANNEL_1);
	  HAL_TIM_Base_Stop_IT(&htim2);
  }

  int carno = -1;
  struct CarData* car;// = &cardata[carno];  // car data in use

  if (inputmode == GPIO) {
	  invstate = INVON;
	  car = &cardata[1];
	  decode_pulsemode(car->pattern);
  }

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  char rxbuf[RXBUFFERSIZE];
  char txbuf[TXBUFFERSIZE];

  while (1)
  {
	// initialize buffer
	memset(rxbuf, 0, RXBUFFERSIZE);
	memset(txbuf, 0, TXBUFFERSIZE);

	// receive
	int i = 0;
	while (!USART2_RX_IsEmpty()) {
		rxbuf[i] = USART2_RX_Read();
		i++;
	}
	HAL_UART_Transmit(&huart2, (uint8_t*)rxbuf, i, UARTTIMEOUT);

	// when receive "invoff"
	if (!strncmp(rxbuf, "invoff", 6)) {
		HAL_UART_Transmit(&huart2, (uint8_t*)rxbuf, i, UARTTIMEOUT);
		inv_reset();
	}
	// when receive "carno=%d", change car only when the inverter is off
	else if (!strncmp(rxbuf, "carno=", 6)) {
		HAL_UART_Transmit(&huart2, (uint8_t*)rxbuf, i, UARTTIMEOUT);
		if (invstate == INVOFF) {
			HAL_UART_Transmit(&huart2, (uint8_t*)rxbuf, i, UARTTIMEOUT);
			sscanf(rxbuf, "carno=%d", &carno);
			if (carno < NUMOFCAR) {
				HAL_UART_Transmit(&huart2, (uint8_t*)rxbuf, i, UARTTIMEOUT);
				car = &cardata[carno];
				decode_pulsemode(car->pattern);
				invstate = INVON;
			} else {
				carno = -1;
			}
		}
	}
	// when receive "mode=%d", change mode only when the inverter is off
	else if (!strncmp(rxbuf, "mode=", 5)) {
		if (invstate == INVOFF) {
			sscanf(rxbuf, "mode=%d", (int*)&mode);
		}
	}
	// when receive notch command
	if (invstate == INVON) {
		if (!strncmp(rxbuf, "notch=", 6)) {
			sscanf(rxbuf, "notch=%d", (int*)&notch);
		}
		else if (rxbuf[0] == 'P') {
			notch = P5;
		}
		else if (rxbuf[0] == 'N') {
			notch = N;
		}
		else if (rxbuf[0] == 'B') {
			notch = B8;
		}
	} else {
		notch = N;
	}

	// apply notch
	if (inputmode == SERIAL) {
		if (notch > N) {  // P
			acc = car->acc0*(notch-N)/(P5-N)/3.6/car->rwheel*car->gr*car->pp;
		} else if (EB < notch) {
			acc = car->brk0*(notch-N)/(B8-N)/3.6/car->rwheel*car->gr*car->pp;
		} else {
			acc = car->eb0/3.6/car->rwheel*car->gr*car->pp;
		}
	} else if (inputmode == GPIO) {
		if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_8)) {
			notch = P5;
			acc = car->acc0/3.6/car->rwheel*car->gr*car->pp;
		} else {
			notch = B8;
			acc = car->brk0/3.6/car->rwheel*car->gr*car->pp;
		}
	}

	HAL_Delay(50);

	// send data to serial
	Iac = 0.0;
	sprintf(txbuf, "{\"speed\":%f, \"fs\":%f, \"frand\":%f, \"Vs\":%f, \"pulsemode\":%d, \"fc\":%f, \"Vdc\":%f, \"Imm\":%f, \"notch\":%d, \"carno\":%d, \"mode\":%d, \"invstate\":%d, \"pedal\":%d}\n",speed,fs,frand,Vs,pulsemode,fc0,Vdc,Iac,(int)notch,carno,(int)mode,(int)invstate,0);
	//sprintf(txbuf, "\"sector\":%d\n", sector);
	//sprintf(txbuf, "{\"speed\":%f, \"fs\":%f, \"fs_ref\":%f, \"Vs\":%f,}\n", speed,fs,omega_ref/2/PI,Vs*100);
	HAL_UART_Transmit(&huart2, (uint8_t*)txbuf, strlen(txbuf), UARTTIMEOUT);


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
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_TIM1|RCC_PERIPHCLK_ADC1;
  PeriphClkInit.Tim1ClockSelection = RCC_TIM1CLK_HCLK;
  PeriphClkInit.Adc1ClockSelection = RCC_ADC1PLLCLK_DIV1;

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

  ADC_InjectionConfTypeDef sConfigInjected = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */
  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = ADC_SCAN_ENABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc1.Init.LowPowerAutoWait = DISABLE;
  hadc1.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure Injected Channel
  */
  sConfigInjected.InjectedChannel = ADC_CHANNEL_1;
  sConfigInjected.InjectedRank = ADC_INJECTED_RANK_1;
  sConfigInjected.InjectedSingleDiff = ADC_SINGLE_ENDED;
  sConfigInjected.InjectedNbrOfConversion = 3;
  sConfigInjected.InjectedSamplingTime = ADC_SAMPLETIME_19CYCLES_5;
  sConfigInjected.ExternalTrigInjecConvEdge = ADC_EXTERNALTRIGINJECCONV_EDGE_RISING;
  sConfigInjected.ExternalTrigInjecConv = ADC_EXTERNALTRIGINJECCONV_T1_TRGO;
  sConfigInjected.AutoInjectedConv = DISABLE;
  sConfigInjected.InjectedDiscontinuousConvMode = DISABLE;
  sConfigInjected.QueueInjectedContext = DISABLE;
  sConfigInjected.InjectedOffset = 0;
  sConfigInjected.InjectedOffsetNumber = ADC_OFFSET_NONE;
  if (HAL_ADCEx_InjectedConfigChannel(&hadc1, &sConfigInjected) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure Injected Channel
  */
  sConfigInjected.InjectedChannel = ADC_CHANNEL_2;
  sConfigInjected.InjectedRank = ADC_INJECTED_RANK_2;
  if (HAL_ADCEx_InjectedConfigChannel(&hadc1, &sConfigInjected) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure Injected Channel
  */
  sConfigInjected.InjectedChannel = ADC_CHANNEL_5;
  sConfigInjected.InjectedRank = ADC_INJECTED_RANK_3;
  sConfigInjected.InjectedSamplingTime = ADC_SAMPLETIME_7CYCLES_5;
  if (HAL_ADCEx_InjectedConfigChannel(&hadc1, &sConfigInjected) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 9;
  htim1.Init.CounterMode = TIM_COUNTERMODE_CENTERALIGNED1;
  htim1.Init.Period = 65535;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
  sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_ENABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_LOW;
  sBreakDeadTimeConfig.BreakFilter = 0;
  sBreakDeadTimeConfig.Break2State = TIM_BREAK2_DISABLE;
  sBreakDeadTimeConfig.Break2Polarity = TIM_BREAK2POLARITY_LOW;
  sBreakDeadTimeConfig.Break2Filter = 0;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_ENABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */
  HAL_TIM_MspPostInit(&htim1);

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
  TIM_HallSensor_InitTypeDef sConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 4294967295;
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
  sConfig.IC1Polarity = TIM_ICPOLARITY_RISING;
  sConfig.IC1Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC1Filter = 14;
  sConfig.Commutation_Delay = 0;
  if (HAL_TIMEx_HallSensor_Init(&htim2, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_OC2REF;
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
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* initialization to use UART2 DMA */
  memset(dma_rx_buf, 0, RXBUFFERSIZE);
  rd_ptr = 0;
  // These UART interrupts halt any ongoing transfer if an error occurs, disable them
  __HAL_UART_DISABLE_IT(&huart2, UART_IT_PE);   // Disable the UART Parity Error Interrupt
  __HAL_UART_DISABLE_IT(&huart2, UART_IT_ERR);  // Disable the UART Error Interrupt: (Frame error, noise error, overrun error)

  /* USER CODE END USART2_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel6_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel6_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel6_IRQn);

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
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin : PB8 */
  GPIO_InitStruct.Pin = GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */
int USART2_RX_IsEmpty(void)
{
	dma_write_ptr = (RXBUFFERSIZE - huart2.hdmarx->Instance->CNDTR) % RXBUFFERSIZE;
    return (rd_ptr == dma_write_ptr);
}

uint8_t USART2_RX_Read(void)
{
    uint8_t c = 0;
    dma_write_ptr = (RXBUFFERSIZE - huart2.hdmarx->Instance->CNDTR) % RXBUFFERSIZE;
    if(rd_ptr != dma_write_ptr) {
        c = dma_rx_buf[rd_ptr++];
        rd_ptr %= RXBUFFERSIZE;
    }
    return c;
}

/**
  * @brief Decode text data which describe pulse pattern
  * @param char*
  * @retval None
  */
void decode_pulsemode(const char* str)
{
	char buf[10];
	int row, i, j;
	enum DataKind {FS, PULSEMODE, FC1, FC2, FRAND1, FRAND2} datakind = 0;

	for (row = 0; row < PULSEMODESIZE; row++) {  // initialize pulse mode list
		list_fs[row] = 0.0;
		list_pulsemode[row] = 0;
		list_fc1[row] = 0.0;
		list_fc2[row] = 0.0;
		list_frand1[row] = 0.0;
		list_frand2[row] = 0.0;
	}

	row = 0; i = 0; j = 0;

	while (str[i] != '\0') {
	    if (str[i] == ',' || str[i] == '\n') {  // at the end of a data
    		buf[j] = '\0';
			switch (datakind) {
			case FS:
				list_fs[row] = atof(buf); break;
			case PULSEMODE:
				list_pulsemode[row] = atoi(buf); break;
			case FC1:
				list_fc1[row] = atof(buf); break;
			case FC2:
				list_fc2[row] = atof(buf); break;
			case FRAND1:
    			list_frand1[row] = atof(buf); break;
			case FRAND2:
				list_frand2[row] = atof(buf); break;
			}
			j = 0;
		    datakind++;  // move to the next data
		} else {
			buf[j] = str[i];  // read a character
			j++;
		}
	    if (str[i] == '\n') {  // at the end of a line
			datakind = 0;
			row++; // move to the next row
	    }
	    i++;
	}
	pulsenum = row + 1;
}

void inv_reset() {
	notch = N;
	hallstate = STOP;
	invstate = INVOFF;
	theta_est = 0;
	theta_u = 0;
	omega_est = 0.0;
	omega_ref = 0.0;
	CtrlPrd = 0.0;
	speed = 0.0;
	fs = 0.0;
	fc = INITIALFC;
	fc0 = INITIALFC;
	Vd = 0.0;
	Vq = 0.0;
	Vs = 0.0;
	Vdc = 36.0;
	Id = 0.0;
	Iq = 0.0;
	Iac = 0.0;
	acc = 0.0;
	pulsemode = 0;
	pmNo = 0;
	pmNo_ref = 0;
	pulsemode_ref = 0;
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
