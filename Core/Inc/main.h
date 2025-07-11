/* USER CODE BEGIN Header */
 /**************************************************************************************************************
 * @file           : main.h                                                        P A C K   C O N T R O L L E R
 * @brief          : 
 ***************************************************************************************************************
 * Copyright (C) 2023-2024 Modular Battery Technologies, Inc.
 * US Patents 11,380,942; 11,469,470; 11,575,270; others. All rights reserved
 **************************************************************************************************************/
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32wbxx_hal.h"
#include "time.h"
#include "eeprom_emul_types.h"
#include "eeprom_emul_conf.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
extern uint16_t        CAN1_INT0_Pin;
extern GPIO_TypeDef  * CAN1_INT0_GPIO_Port;
extern uint16_t        CAN1_INT0_EXTI_IRQn ;
extern uint16_t        CAN1_INT1_Pin ;
extern GPIO_TypeDef  * CAN1_INT1_GPIO_Port ;
extern uint16_t        CAN1_INT1_EXTI_IRQn ;
extern uint16_t        CAN1_INT_Pin ;
extern GPIO_TypeDef  * CAN1_INT_GPIO_Port ;
extern uint16_t        CAN1_INT_EXTI_IRQn ;
extern uint16_t        SPI2_CS1_Pin ;
extern GPIO_TypeDef  * SPI2_CS1_GPIO_Port ;
extern uint16_t        SPI2_CS2_Pin ;
extern GPIO_TypeDef  * SPI2_CS2_GPIO_Port ;
extern uint16_t        CAN1_CS_Pin ;
extern GPIO_TypeDef  * CAN1_CS_GPIO_Port ;
extern uint16_t        BUTTON1_Pin ;
extern GPIO_TypeDef  * BUTTON1_GPIO_Port ;
extern uint16_t        BUTTON1_EXTI_IRQn ;
extern uint16_t        CAN2_CS_Pin ;
extern GPIO_TypeDef  * CAN2_CS_GPIO_Port ;
extern uint16_t        LED_GREEN_Pin ;
extern GPIO_TypeDef  * LED_GREEN_GPIO_Port ;
extern uint16_t        LED_RED_Pin ;
extern GPIO_TypeDef  * LED_RED_GPIO_Port ;
extern uint16_t        CAN2_INT_Pin ;
extern GPIO_TypeDef  * CAN2_INT_GPIO_Port ;
extern uint16_t        CAN2_INT_EXTI_IRQn ;
extern uint16_t        CAN2_INT0_Pin ;
extern GPIO_TypeDef  * CAN2_INT0_GPIO_Port ;
extern uint16_t        CAN2_INT0_EXTI_IRQn ;
extern uint16_t        CAN2_INT1_Pin ;
extern GPIO_TypeDef  * CAN2_INT1_GPIO_Port ;
extern uint16_t        CAN2_INT1_EXTI_IRQn ;
extern uint16_t        BUTTON2_Pin ;
extern GPIO_TypeDef  * BUTTON2_GPIO_Port ;
extern uint16_t        BUTTON2_EXTI_IRQn ;
extern uint16_t        BUTTON3_Pin ;
extern GPIO_TypeDef  * BUTTON3_GPIO_Port ;
extern uint16_t        BUTTON3_EXTI_IRQn ;
extern uint16_t        LED_BLUE_Pin ;
extern GPIO_TypeDef  * LED_BLUE_GPIO_Port ;

extern uint16_t        CAN3_INT_Pin ;
extern GPIO_TypeDef  * CAN3_INT_GPIO_Port ;
extern uint16_t        CAN3_INT_EXTI_IRQn ;
extern uint16_t        CAN3_INT0_Pin ;
extern GPIO_TypeDef  * CAN3_INT0_GPIO_Port ;
extern uint16_t        CAN3_INT0_EXTI_IRQn ;
extern uint16_t        CAN3_INT1_Pin ;
extern GPIO_TypeDef  * CAN3_INT1_GPIO_Port ;
extern uint16_t        CAN3_INT1_EXTI_IRQn ;
extern uint16_t        CAN3_CS_Pin ;
extern GPIO_TypeDef  * CAN3_CS_GPIO_Port ;

extern uint16_t        BUTTON4_Pin ;
extern GPIO_TypeDef  * BUTTON4_GPIO_Port ;
extern uint16_t        BUTTON4_EXTI_IRQn ;

extern uint16_t        LED_CAN1_Pin ;
extern GPIO_TypeDef  * LED_CAN1_GPIO_Port ;
extern uint16_t        LED_CAN2_Pin ;
extern GPIO_TypeDef  * LED_CAN2_GPIO_Port ;
extern uint16_t        LED_CAN3_Pin ;
extern GPIO_TypeDef  * LED_CAN3_GPIO_Port ;
extern uint16_t        LED_HBEAT_Pin ;
extern GPIO_TypeDef  * LED_HBEAT_GPIO_Port ;

// ANALOG
extern uint16_t        VDETECT_5V_Pin;
extern GPIO_TypeDef  * VDETECT_5V_GPIO_Port;

// ENABLES
extern uint16_t        CAN_CLK_EN_Pin;
extern GPIO_TypeDef  * CAN_CLK_EN_GPIO_Port;
extern uint16_t        BAT_CHRG_EN_Pin;
extern GPIO_TypeDef  * BAT_CHRG_EN_GPIO_Port;

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
/*
#define CAN1_INT0_Pin GPIO_PIN_8
#define CAN1_INT0_GPIO_Port GPIOB
#define CAN1_INT0_EXTI_IRQn EXTI9_5_IRQn
#define CAN1_INT1_Pin GPIO_PIN_9
#define CAN1_INT1_GPIO_Port GPIOB
#define CAN1_INT1_EXTI_IRQn EXTI9_5_IRQn
#define SPI2_CS1_Pin GPIO_PIN_0
#define SPI2_CS1_GPIO_Port GPIOC
#define SPI2_CS2_Pin GPIO_PIN_3
#define SPI2_CS2_GPIO_Port GPIOC
#define CAN1_CS_Pin GPIO_PIN_5
#define CAN1_CS_GPIO_Port GPIOA
#define BUTTON1_Pin GPIO_PIN_4
#define BUTTON1_GPIO_Port GPIOC
#define BUTTON1_EXTI_IRQn EXTI4_IRQn
#define CAN2_CS_Pin GPIO_PIN_5
#define CAN2_CS_GPIO_Port GPIOC
#define LED_GREEN_Pin GPIO_PIN_0
#define LED_GREEN_GPIO_Port GPIOB
#define LED_RED_Pin GPIO_PIN_1
#define LED_RED_GPIO_Port GPIOB
#define CAN2_INT_Pin GPIO_PIN_12
#define CAN2_INT_GPIO_Port GPIOB
#define CAN2_INT_EXTI_IRQn EXTI15_10_IRQn
#define CAN2_INT0_Pin GPIO_PIN_13
#define CAN2_INT0_GPIO_Port GPIOB
#define CAN2_INT0_EXTI_IRQn EXTI15_10_IRQn
#define CAN2_INT1_Pin GPIO_PIN_14
#define CAN2_INT1_GPIO_Port GPIOB
#define CAN2_INT1_EXTI_IRQn EXTI15_10_IRQn
#define CAN1_INT_Pin GPIO_PIN_10
#define CAN1_INT_GPIO_Port GPIOA
#define CAN1_INT_EXTI_IRQn EXTI15_10_IRQn
#define BUTTON2_Pin GPIO_PIN_0
#define BUTTON2_GPIO_Port GPIOD
#define BUTTON2_EXTI_IRQn EXTI0_IRQn
#define BUTTON3_Pin GPIO_PIN_1
#define BUTTON3_GPIO_Port GPIOD
#define BUTTON3_EXTI_IRQn EXTI1_IRQn
#define LED_BLUE_Pin GPIO_PIN_5
#define LED_BLUE_GPIO_Port GPIOB
*/
/* USER CODE BEGIN Private defines */
//Timeout used for SPI functions

#define PLATFORM_NUCLEO   0
#define PLATFORM_MODBATT  1

#define MAX_BUFFER      250
#define UART_TIMEOUT    1000

#define SPI_TIMEOUT			100

#define CAN1            0
#define CAN2            1
#define CAN3            2
#define VCU_CAN         CAN1
#define MCU_CAN         CAN2
#define MCU2_CAN        CAN3


// Debug Levels
#define DEBUG_LEVEL     (DBG_MCU | DBG_ERRORS | DBG_COMMS)
#define DBG_DISABLED    0x00
#define DBG_ERRORS      0x01
#define DBG_COMMS       0x02  // TX and RX messages
#define DBG_MCU         0x08
#define DBG_VCU         0x10
#define DBG_VERBOSE     0x80
#define DBG_ALL         0xFF

#define VALIDATE_HARDWARE 1 // checks for maximum charge/discharge in range - put module in fault if invalid

extern uint8_t debugLevel;


//LED's

#define VCU_RX_LED      0
#define MCU_RX_LED      1
#define MCU2_RX_LED     2
#define HBEAT_LED       3


// Code anchor for break points
#define Nop() asm("nop")

extern uint32_t      eeVarDataTab[NB_OF_VARIABLES+1];

extern uint8_t hwPlatform;
extern uint8_t  EE_PACK_ID;
extern void switchLedOn(uint8_t led);
extern void switchLedOff(uint8_t led);
extern char tempBuffer[MAX_BUFFER];
extern uint8_t can2RxInterrupt;
extern uint8_t can2TxInterrupt;
extern uint8_t can1RxInterrupt;
extern uint8_t can1TxInterrupt;
extern void serialOut(char* message);
extern uint8_t deRegisterAll;
extern uint32_t etTimerOverflows;
extern TIM_HandleTypeDef htim1;
extern uint8_t decSec;
extern uint8_t sendState;
extern uint8_t sendMaxState;
extern void writeRTC(time_t now);
extern time_t readRTC(void);

extern EE_Status LoadAllEEPROM(void);
extern EE_Status LoadFromEEPROM(uint16_t virtAddress, uint32_t *eeData);
extern EE_Status StoreEEPROM(uint16_t virtAddress, uint32_t data);

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
