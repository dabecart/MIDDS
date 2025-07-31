/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32g4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define POWER_GOOD_OUT_3V3_Pin GPIO_PIN_13
#define POWER_GOOD_OUT_3V3_GPIO_Port GPIOC
#define SELECTED_SCK_Pin GPIO_PIN_14
#define SELECTED_SCK_GPIO_Port GPIOC
#define CH13_Pin GPIO_PIN_0
#define CH13_GPIO_Port GPIOA
#define CH14_Pin GPIO_PIN_1
#define CH14_GPIO_Port GPIOA
#define CH15_Pin GPIO_PIN_2
#define CH15_GPIO_Port GPIOA
#define CH00_Pin GPIO_PIN_3
#define CH00_GPIO_Port GPIOA
#define CH01_Pin GPIO_PIN_4
#define CH01_GPIO_Port GPIOA
#define CH02_Pin GPIO_PIN_6
#define CH02_GPIO_Port GPIOA
#define CH03_Pin GPIO_PIN_0
#define CH03_GPIO_Port GPIOB
#define CH04_Pin GPIO_PIN_1
#define CH04_GPIO_Port GPIOB
#define CH05_Pin GPIO_PIN_2
#define CH05_GPIO_Port GPIOB
#define CH06_Pin GPIO_PIN_10
#define CH06_GPIO_Port GPIOB
#define CH07_Pin GPIO_PIN_11
#define CH07_GPIO_Port GPIOB
#define SHIFT_REG_ENABLE_Pin GPIO_PIN_12
#define SHIFT_REG_ENABLE_GPIO_Port GPIOB
#define CH08_Pin GPIO_PIN_9
#define CH08_GPIO_Port GPIOA
#define CH09_Pin GPIO_PIN_10
#define CH09_GPIO_Port GPIOA
#define POWER_GOOD_3V3_Pin GPIO_PIN_10
#define POWER_GOOD_3V3_GPIO_Port GPIOC
#define POWER_GOOD_1V8_Pin GPIO_PIN_11
#define POWER_GOOD_1V8_GPIO_Port GPIOC
#define CH10_Pin GPIO_PIN_3
#define CH10_GPIO_Port GPIOB
#define POWER_GOOD_5V_Pin GPIO_PIN_4
#define POWER_GOOD_5V_GPIO_Port GPIOB
#define POWER_GOOD_OUT_5V_Pin GPIO_PIN_5
#define POWER_GOOD_OUT_5V_GPIO_Port GPIOB
#define CH11_Pin GPIO_PIN_6
#define CH11_GPIO_Port GPIOB
#define CH12_Pin GPIO_PIN_9
#define CH12_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
