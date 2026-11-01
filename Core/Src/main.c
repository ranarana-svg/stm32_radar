/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "usart.h"
#include "gpio.h"
#include "oled.h"

#include <stdio.h>
#include <string.h>

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "radar_parse.h"
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
volatile uint8_t  g_heartRate    = 0;
volatile uint8_t  g_breathRate   = 0;
volatile uint8_t  g_bodyMove     = 0;
volatile uint8_t  g_humanExist   = 0;
volatile uint8_t  g_hr_updated   = 0;
volatile uint8_t  g_br_updated   = 0;

uint8_t g_rx_byte;

static uint32_t s_last_oled_tick = 0;
#define OLED_REFRESH_MS  500

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void OLED_ShowUI(void);

/* USER CODE BEGIN PV */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        RadarRingBuf_Put(g_rx_byte);
        HAL_UART_Receive_IT(&huart2, &g_rx_byte, 1);
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

  HAL_Init();

  /* USER CODE BEGIN SysInit */
  /* USER CODE END SysInit */

  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_USART2_UART_Init();

  /* USER CODE BEGIN 2 */
  OLED_Init();
  OLED_Clear();
  OLED_ShowUI();
  OLED_Refresh();

  RadarParser_Init();

  HAL_UART_Receive_IT(&huart2, &g_rx_byte, 1);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* 1. ??????????????? */
    RadarParser_FeedFromRingBuf();

    /* 2. ???? OLED */
    if (HAL_GetTick() - s_last_oled_tick >= OLED_REFRESH_MS)
    {
      s_last_oled_tick = HAL_GetTick();

      OLED_Clear();
      OLED_ShowUI();
      OLED_Refresh();
    }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief OLED ????(??+??)
  */
static void OLED_ShowUI(void)
{
  /* ---- ?? ---- */
   OLED_ShowString(0, 0, (uint8_t *)"Health Monitor", 16);

    /* ?? */
    OLED_ShowString(0, 20, (uint8_t *)"HR :", 16);
    if (g_hr_updated)   // ? ???
    {
        OLED_ShowNum(40, 20, g_heartRate, 3, 16);
        OLED_ShowString(72, 20, (uint8_t *)"bpm", 16);
    }
    else
    {
        OLED_ShowString(40, 20, (uint8_t *)"---", 16);
    }

    /* ?? */
    OLED_ShowString(0, 40, (uint8_t *)"BR :", 16);
    if (g_br_updated)   // ? ???
    {
        OLED_ShowNum(40, 40, g_breathRate, 3, 16);
        OLED_ShowString(72, 40, (uint8_t *)"r/m", 16);
    }
    else
    {
        OLED_ShowString(40, 40, (uint8_t *)"---", 16);
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

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK

                              | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
/* USER CODE END 4 */

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* USER CODE END 6 */
}
#endif