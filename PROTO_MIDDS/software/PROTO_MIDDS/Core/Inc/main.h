/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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
#define GPIO03_Pin GPIO_PIN_13
#define GPIO03_GPIO_Port GPIOC
#define SHIFT_REG_ENABLE_Pin GPIO_PIN_14
#define SHIFT_REG_ENABLE_GPIO_Port GPIOC
#define TFT_CS_Pin GPIO_PIN_15
#define TFT_CS_GPIO_Port GPIOC
#define TFT_A0_Pin GPIO_PIN_1
#define TFT_A0_GPIO_Port GPIOF
#define CH13_Pin GPIO_PIN_0
#define CH13_GPIO_Port GPIOA
#define CH12_Pin GPIO_PIN_1
#define CH12_GPIO_Port GPIOA
#define CH11_Pin GPIO_PIN_2
#define CH11_GPIO_Port GPIOA
#define CH10_Pin GPIO_PIN_3
#define CH10_GPIO_Port GPIOA
#define CH00_Pin GPIO_PIN_4
#define CH00_GPIO_Port GPIOA
#define SCK_Pin GPIO_PIN_5
#define SCK_GPIO_Port GPIOA
#define CH01_Pin GPIO_PIN_6
#define CH01_GPIO_Port GPIOA
#define MOSI_Pin GPIO_PIN_7
#define MOSI_GPIO_Port GPIOA
#define GPIO11_Pin GPIO_PIN_4
#define GPIO11_GPIO_Port GPIOC
#define CH02_Pin GPIO_PIN_0
#define CH02_GPIO_Port GPIOB
#define CH03_Pin GPIO_PIN_1
#define CH03_GPIO_Port GPIOB
#define GPIO04_Pin GPIO_PIN_2
#define GPIO04_GPIO_Port GPIOB
#define GPIO12_Pin GPIO_PIN_10
#define GPIO12_GPIO_Port GPIOB
#define GPIO05_Pin GPIO_PIN_11
#define GPIO05_GPIO_Port GPIOB
#define GPIO13_Pin GPIO_PIN_12
#define GPIO13_GPIO_Port GPIOB
#define GPIO06_Pin GPIO_PIN_13
#define GPIO06_GPIO_Port GPIOB
#define GPIO14_Pin GPIO_PIN_14
#define GPIO14_GPIO_Port GPIOB
#define GPIO07_Pin GPIO_PIN_15
#define GPIO07_GPIO_Port GPIOB
#define GPIO15_Pin GPIO_PIN_6
#define GPIO15_GPIO_Port GPIOC
#define CH06_Pin GPIO_PIN_8
#define CH06_GPIO_Port GPIOA
#define CH05_Pin GPIO_PIN_9
#define CH05_GPIO_Port GPIOA
#define CH04_Pin GPIO_PIN_10
#define CH04_GPIO_Port GPIOA
#define GPIO00_Pin GPIO_PIN_15
#define GPIO00_GPIO_Port GPIOA
#define GPIO08_Pin GPIO_PIN_10
#define GPIO08_GPIO_Port GPIOC
#define GPIO01_Pin GPIO_PIN_11
#define GPIO01_GPIO_Port GPIOC
#define GPIO09_Pin GPIO_PIN_3
#define GPIO09_GPIO_Port GPIOB
#define GPIO02_Pin GPIO_PIN_4
#define GPIO02_GPIO_Port GPIOB
#define GPIO10_Pin GPIO_PIN_5
#define GPIO10_GPIO_Port GPIOB
#define CH08_Pin GPIO_PIN_6
#define CH08_GPIO_Port GPIOB
#define CH09_Pin GPIO_PIN_7
#define CH09_GPIO_Port GPIOB
#define CH07_Pin GPIO_PIN_9
#define CH07_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
