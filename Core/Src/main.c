/* USER CODE BEGIN Header */

/***************************************************************************************************************
 * @file           : main.c                                                       P A C K   C O N T R O L L E R
 * @brief          : Hardware initialisation, main while loop
 **************************************************************************************************************
 * Copyright (C) 2023-2024 Modular Battery Technologies, Inc.
 * US Patents 11,380,942; 11,469,470; 11,575,270; others. All rights reserved
 **************************************************************************************************************/
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "mcu.h"
#include "bms.h"
#include "stdio.h"
#include "string.h"
#include "canfdspi_api.h"
#include "canfdspi_defines.h"
#include "canfdspi_register.h"
#include "vcu.h"
#include "time.h"
#include "eeprom_emul.h"
#include "eeprom_data.h"


/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define PWR_FLAG_WUF PWR_FLAG_WUF3
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef hlpuart1;
UART_HandleTypeDef huart1;

PKA_HandleTypeDef hpka;

RTC_HandleTypeDef hrtc;

SPI_HandleTypeDef hspi1;
SPI_HandleTypeDef hspi2;

TIM_HandleTypeDef htim1;

PCD_HandleTypeDef hpcd_USB_FS;

/* USER CODE BEGIN PV */





uint16_t      eeIndex = 1;
__IO uint32_t eeErasingOnGoing = 0;
uint32_t      eeVarDataTab[NB_OF_VARIABLES+1] = {0};
uint32_t      eeVarValue = 0;

uint8_t hwPlatform = PLATFORM_NUCLEO;


char logtime[9];
char txBuffer[MAX_BUFFER];
char tempBuffer[MAX_BUFFER];


uint16_t        CAN1_INT_Pin ;
GPIO_TypeDef  * CAN1_INT_GPIO_Port ;
uint16_t        CAN1_INT_EXTI_IRQn ;
uint16_t        CAN1_INT0_Pin;
GPIO_TypeDef  * CAN1_INT0_GPIO_Port;
uint16_t        CAN1_INT0_EXTI_IRQn ;
uint16_t        CAN1_INT1_Pin ;
GPIO_TypeDef  * CAN1_INT1_GPIO_Port ;
uint16_t        CAN1_INT1_EXTI_IRQn ;
uint16_t        CAN1_CS_Pin ;
GPIO_TypeDef  * CAN1_CS_GPIO_Port ;

uint16_t        CAN2_INT_Pin ;
GPIO_TypeDef  * CAN2_INT_GPIO_Port ;
uint16_t        CAN2_INT_EXTI_IRQn ;
uint16_t        CAN2_INT0_Pin ;
GPIO_TypeDef  * CAN2_INT0_GPIO_Port ;
uint16_t        CAN2_INT0_EXTI_IRQn ;
uint16_t        CAN2_INT1_Pin ;
GPIO_TypeDef  * CAN2_INT1_GPIO_Port ;
uint16_t        CAN2_INT1_EXTI_IRQn ;
uint16_t        CAN2_CS_Pin ;
GPIO_TypeDef  * CAN2_CS_GPIO_Port ;

uint16_t        CAN3_INT_Pin ;
GPIO_TypeDef  * CAN3_INT_GPIO_Port ;
uint16_t        CAN3_INT_EXTI_IRQn ;
uint16_t        CAN3_INT0_Pin ;
GPIO_TypeDef  * CAN3_INT0_GPIO_Port ;
uint16_t        CAN3_INT0_EXTI_IRQn ;
uint16_t        CAN3_INT1_Pin ;
GPIO_TypeDef  * CAN3_INT1_GPIO_Port ;
uint16_t        CAN3_INT1_EXTI_IRQn ;
uint16_t        CAN3_CS_Pin ;
GPIO_TypeDef  * CAN3_CS_GPIO_Port ;

uint16_t        SPI2_CS1_Pin ;
GPIO_TypeDef  * SPI2_CS1_GPIO_Port ;
uint16_t        SPI2_CS2_Pin ;
GPIO_TypeDef  * SPI2_CS2_GPIO_Port ;

uint16_t        BUTTON1_Pin ;
GPIO_TypeDef  * BUTTON1_GPIO_Port ;
uint16_t        BUTTON1_EXTI_IRQn ;
uint16_t        BUTTON2_Pin ;
GPIO_TypeDef  * BUTTON2_GPIO_Port ;
uint16_t        BUTTON2_EXTI_IRQn ;
uint16_t        BUTTON3_Pin ;
GPIO_TypeDef  * BUTTON3_GPIO_Port ;
uint16_t        BUTTON3_EXTI_IRQn ;

uint16_t        LED_GREEN_Pin ;
GPIO_TypeDef  * LED_GREEN_GPIO_Port ;
uint16_t        LED_RED_Pin ;
GPIO_TypeDef  * LED_RED_GPIO_Port ;
uint16_t        LED_BLUE_Pin ;
GPIO_TypeDef  * LED_BLUE_GPIO_Port ;

uint16_t        LED_CAN1_Pin ;
GPIO_TypeDef  * LED_CAN1_GPIO_Port ;
uint16_t        LED_CAN2_Pin ;
GPIO_TypeDef  * LED_CAN2_GPIO_Port ;
uint16_t        LED_CAN3_Pin ;
GPIO_TypeDef  * LED_CAN3_GPIO_Port ;
uint16_t        LED_HBEAT_Pin ;
GPIO_TypeDef  * LED_HBEAT_GPIO_Port ;


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void PeriphCommonClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_SPI2_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USB_PCD_Init(void);
static void MX_PKA_Init(void);
static void MX_RTC_Init(void);
static void MX_LPUART1_UART_Init(void);
static void MX_TIM1_Init(void);
/* USER CODE BEGIN PFP */
void getTimeBCD(void);
void writeRTC(time_t now);
time_t readRTC(void);
void serialOut(char* message);
uint8_t can2RxInterrupt = 0;
uint8_t can2TxInterrupt = 0;
uint8_t can1RxInterrupt = 0;
uint8_t can1TxInterrupt = 0;
//uint8_t deRegisterAll   = 0;

uint32_t etTimerOverflows = 0;




//EEPROM variables - initial declaration and setting - these are read from eeprom in PCU_Initialise() and updated accordingly
uint8_t EE_PACK_ID = 0;





uint16_t  packOffset = 0;

uint8_t decSec = 0;
uint8_t sendState = 0;

uint8_t debugLevel = DEBUG_LEVEL; //Using a global variable "debugLevel" as we may add dynamic debugging via jumpers/switches at a later date

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/***************************************************************************************************************
*     S E R I A L   O U T                                                          P A C K   C O N T R O L L E R
***************************************************************************************************************/
void serialOut(char* message){
  getTimeBCD();
  sprintf(txBuffer,"%s %s\r\n",logtime, message);

  //HAL_UART_Transmit_IT(&huart1, (uint8_t*)txBuffer, strlen(txBuffer));
  if (HAL_UART_Transmit(&huart1, (uint8_t*)txBuffer, strlen(txBuffer), 2000) != HAL_OK){// Sending in normal mode
    if(debugLevel & DBG_ERRORS){ sprintf(tempBuffer,"ERROR - Serial debug output error"); serialOut(tempBuffer);}
  }
}

/***************************************************************************************************************
*     T I M E S T A M P                                                            P A C K   C O N T R O L L E R
***************************************************************************************************************/
void getTimeBCD(void){

  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef sDate = {0};

  HAL_RTC_GetTime(&hrtc,&sTime, RTC_FORMAT_BCD);
  HAL_RTC_GetDate(&hrtc,&sDate, RTC_FORMAT_BCD);
  uint8_t seconds = sTime.Seconds;
  uint8_t minutes = sTime.Minutes;
  uint8_t hours = sTime.Hours;
  sprintf(logtime,"%02x:%02x:%02x",hours,minutes,seconds);
}

/***************************************************************************************************************
*     w r i t e R T C                                                              P A C K   C O N T R O L L E R
***************************************************************************************************************/
void writeRTC(time_t now)
{

 RTC_TimeTypeDef sTime;
 RTC_DateTypeDef sDate;

 struct tm time_tm;
 time_tm = *(localtime(&now));

 sTime.Hours = (uint8_t)time_tm.tm_hour;
 sTime.Minutes = (uint8_t)time_tm.tm_min;
 sTime.Seconds = (uint8_t)time_tm.tm_sec;
 // Set the time in the RTC
 if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
 {
   if(debugLevel & DBG_ERRORS){ sprintf(tempBuffer,"ERROR - HAL_RTC_SetTime returned error"); serialOut(tempBuffer);}
 }

 if (time_tm.tm_wday == 0) { time_tm.tm_wday = 7; }     // STM32 day order is Monday to Sunday
 sDate.WeekDay = (uint8_t)time_tm.tm_wday;
 sDate.Month = (uint8_t)time_tm.tm_mon+1;               // STM32 month starts at 1
 sDate.Date = (uint8_t)time_tm.tm_mday;
 sDate.Year = (uint16_t)(time_tm.tm_year + 1900-2000);  // time.h is years since 1900, STM32 is years since 2000

//Set the Date in the RTC
 if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK)
 {
   if(debugLevel & DBG_ERRORS){ sprintf(tempBuffer,"ERROR - HAL_RTC_SetDate returned error"); serialOut(tempBuffer);}
 }

 HAL_RTCEx_BKUPWrite(&hrtc,RTC_BKP_DR0,0x32F2);       // lock it in with the backup registers

}

/***************************************************************************************************************
*     r e a d R T C                                                                P A C K   C O N T R O L L E R
***************************************************************************************************************/
time_t readRTC(void)
{

  RTC_DateTypeDef rtcDate;
  RTC_TimeTypeDef rtcTime;
  time_t currentTime = {0};
  struct tm tim = {0};

  HAL_RTC_GetTime(&hrtc, &rtcTime, RTC_FORMAT_BIN);
  HAL_RTC_GetDate(&hrtc, &rtcDate, RTC_FORMAT_BIN);
  uint8_t hh  = rtcTime.Hours;
  uint8_t mm  = rtcTime.Minutes;
  uint8_t ss  = rtcTime.Seconds;
  uint8_t d   = rtcDate.Date;
  uint8_t m   = rtcDate.Month;
  uint16_t y  = rtcDate.Year;
  uint16_t yr = (uint16_t)(y + 2000-1900);  // time.h is years since 1900, STM32 is years since 2000

  tim.tm_year = yr;
  tim.tm_mon  = m - 1;       // adjust the month numbering (STM32 starts at one, time.h starts at 0)
  tim.tm_mday = d;
  tim.tm_hour = hh;
  tim.tm_min  = mm;
  tim.tm_sec  = ss;
  currentTime = mktime(&tim);

  return currentTime;

  /*
  struct tm printTm = {0};
  printTm = *(localtime(&currentTime));
  char buffer[80];
  strftime(buffer,80,'RTC %m/%d/%y %H:%M:%S', &printTm);
  sprintf('%s  ', buffer);
*/
}

/***************************************************************************************************************
*     t e s t T i m e                                                              P A C K   C O N T R O L L E R
***************************************************************************************************************/
time_t testTime(void)
{
  // test function to check that the 2038 bug does not exist in the time library (years > 2038 results in overflow if bug exists)

  struct timeval time;
  struct tm t_decomp;
  time_t seconds = {0};
  uint8_t sizeBytes = 0;

  time.tv_sec = -1;

  t_decomp.tm_year  = 139; //2039
  t_decomp.tm_mon   = 0;
  t_decomp.tm_mday  = 1;
  t_decomp.tm_hour  = 8;
  t_decomp.tm_min   = 37;
  t_decomp.tm_sec   = 53;
  t_decomp.tm_isdst = -1;

  time.tv_sec = mktime(&t_decomp);
  time.tv_usec = 0;

  sizeBytes = sizeof(time.tv_sec); // check sizeBytes = 8 (64 bit)
  sizeBytes = sizeBytes;           // added to get rid of warning for variables set but not used!
  seconds   = time.tv_sec;         // Check that seconds is positive for year 2039 (2177483873)
  return seconds;

  // If seconds returned by mktime() is negative then the 2038 bug is an issue - the time is decoded into 1902-11-26 02:09:37,000 (instead of 2039-01-01)
  // TESTED and all good!
}

/***************************************************************************************************************
*     E E P R O M                                                                  P A C K   C O N T R O L L E R
***************************************************************************************************************/

EE_Status LoadAllEEPROM(void)
{

  uint16_t virtAddress;
  EE_Status eeStatus = EE_OK;

  for(virtAddress = 1; virtAddress < (NB_OF_VARIABLES + 1); virtAddress++) {
    eeStatus |= EE_ReadVariable32bits(virtAddress, &eeVarDataTab[virtAddress]);
  }
  // Update any system variables that are set from EEPROM values
  if(eeStatus == EE_OK){

  }


  return eeStatus;
}

EE_Status LoadFromEEPROM(uint16_t virtAddress, uint32_t *eeData)
{

  EE_Status eeStatus = EE_OK;

  eeStatus = EE_ReadVariable32bits(virtAddress, eeData);
  return eeStatus;
}


EE_Status StoreEEPROM(uint16_t virtAddress, uint32_t data)
{

  EE_Status eeStatus = EE_OK;

  /* Unlock the Flash Program Erase controller */
  HAL_FLASH_Unlock();

  //Clear OPTVERR bit
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_OPTVERR);
  while(__HAL_FLASH_GET_FLAG(FLASH_FLAG_OPTVERR) != RESET) ;

  while (eeErasingOnGoing == 1) { }

  eeStatus = EE_WriteVariable32bits(virtAddress, data);
  eeStatus|= EE_ReadVariable32bits(virtAddress, &data);

  // Start cleanup IT mode, if cleanup is needed
  if ((eeStatus & EE_STATUSMASK_CLEANUP) == EE_STATUSMASK_CLEANUP) {eeErasingOnGoing = 1;eeStatus|= EE_CleanUp_IT();}
 // if ((ee_status & EE_STATUSMASK_ERROR) == EE_STATUSMASK_ERROR) {Error_Handler();}

  /* Lock the Flash Program Erase controller */
  HAL_FLASH_Lock();

  return eeStatus;



  /*
      //Store 10 values of all variables in EEPROM, ascending order
        for (eeVarValue = 1; eeVarValue <= 10; eeVarValue++)
        {
          for (eeIndex = 1; eeIndex < NB_OF_VARIABLES+1; eeIndex++)
          {
            // Wait any cleanup is completed before accessing flash again
            while (eeErasingOnGoing == 1) { }

            ee_status = EE_WriteVariable32bits(eeIndex, eeIndex*eeVarValue);
            ee_status|= EE_ReadVariable32bits(eeIndex, &eeVarDataTab[eeIndex-1]);
            if (eeIndex*eeVarValue != eeVarDataTab[eeIndex-1]) {Error_Handler();}

            // Start cleanup IT mode, if cleanup is needed
            if ((ee_status & EE_STATUSMASK_CLEANUP) == EE_STATUSMASK_CLEANUP) {eeErasingOnGoing = 1;ee_status|= EE_CleanUp_IT();}
            if ((ee_status & EE_STATUSMASK_ERROR) == EE_STATUSMASK_ERROR) {Error_Handler();}
          }
        }

        // Read all the variables
        for (eeIndex = 1; eeIndex < NB_OF_VARIABLES+1; eeIndex++)
        {
          ee_status = EE_ReadVariable32bits(eeIndex, &eeVarValue);
          if (eeVarValue != eeVarDataTab[eeIndex-1]) {Error_Handler();}
          if (ee_status != EE_OK) {Error_Handler();}
        }
        // Test is completed successfully

    */



}




/***************************************************************************************************************
*     L E D   O N / O F F                                                          P A C K   C O N T R O L L E R
***************************************************************************************************************/
void switchLedOn(uint8_t led)
{
    switch (led) {
        case RED_LED:
          HAL_GPIO_WritePin(LED_RED_GPIO_Port,  LED_RED_Pin , GPIO_PIN_SET);
            break;
        case GREEN_LED:
          HAL_GPIO_WritePin(LED_GREEN_GPIO_Port,  LED_GREEN_Pin , GPIO_PIN_SET);
            break;
        case BLUE_LED:
            HAL_GPIO_WritePin(LED_BLUE_GPIO_Port,  LED_BLUE_Pin , GPIO_PIN_SET);
            break;
        default: break;
    }
}

void switchLedOff(uint8_t led)
{
    switch (led) {
        case RED_LED:
          HAL_GPIO_WritePin(LED_RED_GPIO_Port,  LED_RED_Pin , GPIO_PIN_RESET);
            break;
        case GREEN_LED:
          HAL_GPIO_WritePin(LED_GREEN_GPIO_Port,  LED_GREEN_Pin , GPIO_PIN_RESET);
            break;
        case BLUE_LED:
            HAL_GPIO_WritePin(LED_BLUE_GPIO_Port,  LED_BLUE_Pin , GPIO_PIN_RESET);
            break;
        default: break;
    }
}


/***************************************************************************************************************
 *   E E P R O M     I N T E R R U P T    C A L L B A C K                          P A C K   C O N T R O L L E R
***************************************************************************************************************/
/**
  * @brief  FLASH end of operation interrupt callback.
  * @param  ReturnValue: The value saved in this parameter depends on the ongoing procedure
  *                  Mass Erase: Bank number which has been requested to erase
  *                  Page Erase: Page which has been erased
  *                    (if 0xFFFFFFFF, it means that all the selected pages have been erased)
  *                  Program: Address which was selected for data program
  * @retval None
  */
void HAL_FLASH_EndOfOperationCallback(uint32_t ReturnValue)
{
  /* Call CleanUp callback when all requested pages have been erased */
  if( (ReturnValue == (START_PAGE+PAGES_NUMBER/2-1)) || (ReturnValue == (START_PAGE+PAGES_NUMBER-1)) )
  {
    EE_EndOfCleanup_UserCallback();
  }
}

/**
  * @brief  Clean Up end of operation interrupt callback.
  * @param  None
  * @retval None
  */
void EE_EndOfCleanup_UserCallback(void)
{
  eeErasingOnGoing = 0;
}


/***************************************************************************************************************
 *   G P I O     I N T E R R U P T    C A L L B A C K                              P A C K   C O N T R O L L E R
***************************************************************************************************************/
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {

  // Clear Wake Up Flag (when woken from STOP
  //__HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);

  if(GPIO_Pin == CAN1_INT_Pin){
    // CAN1 (VCU) Interrupt
  }else if (GPIO_Pin == CAN1_INT0_Pin){
    // CAN1 (VCU) TX Interrupt
    can1TxInterrupt = 1;
  }else if (GPIO_Pin == CAN1_INT1_Pin){
     // CAN1 (VCU) RX Interrupt
     can1RxInterrupt = !HAL_GPIO_ReadPin(CAN1_INT1_GPIO_Port, CAN1_INT1_Pin); // Active Low - inverted with !
     if (can1RxInterrupt) switchLedOn(VCU_RX_LED);
     else switchLedOff(VCU_RX_LED);
  }else if (GPIO_Pin == CAN2_INT_Pin){
    // CAN2 (MCU) Interrupt
  }else if (GPIO_Pin == CAN2_INT0_Pin){
    // CAN2 (MCU) TX Interrupt
    can2TxInterrupt = 1;
  }else if (GPIO_Pin == CAN2_INT1_Pin){
    // CAN2 (MCU) RX Interrupt
    can2RxInterrupt = !HAL_GPIO_ReadPin(CAN2_INT1_GPIO_Port, CAN2_INT1_Pin); // Active Low - inverted with !
    if (can2RxInterrupt) switchLedOn(MCU_RX_LED);
    else switchLedOff(MCU_RX_LED);
  }else if(GPIO_Pin == BUTTON1_Pin){

  }else if(GPIO_Pin == BUTTON2_Pin){

  }else if(GPIO_Pin == BUTTON3_Pin){

  }
}

/***************************************************************************************************************
*     T I M E R     P E R I O D    E L A P S E D    C A L L B A C K                P A C K   C O N T R O L L E R
***************************************************************************************************************/
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {

  // This callback is automatically called by the HAL on the UEV event
  if(htim->Instance == htim1.Instance){
    // last contact timer overflow
    etTimerOverflows++;
    decSec++;
    if(decSec == 10){
      decSec = 0;
      HAL_GPIO_TogglePin(LED_BLUE_GPIO_Port,  LED_BLUE_Pin);
    }
    if((decSec % 5) == 0) sendState = 1;
  }
}



/***************************************************************************************************************
*     E E P R O M   D E F A U L T   V A L U E S                                    P A C K   C O N T R O L L E R
***************************************************************************************************************/
EE_Status eepromDefaults(void)
{
  EE_Status eeStatus = EE_OK;

  /* Unlock the Flash Program Erase controller */
  HAL_FLASH_Unlock();

  // Check EEPROM has been set up
  eeStatus |= EE_WriteVariable32bits(EE_MAGIC1, MAGIC1);            // Add Magic data
  // Start cleanup IT mode, if cleanup is needed
  if ((eeStatus & EE_STATUSMASK_CLEANUP) == EE_STATUSMASK_CLEANUP) {eeErasingOnGoing = 1;eeStatus|= EE_CleanUp_IT();}
  while(eeErasingOnGoing){};
  eeStatus |= EE_WriteVariable32bits(EE_MAGIC2, MAGIC2);
  if ((eeStatus & EE_STATUSMASK_CLEANUP) == EE_STATUSMASK_CLEANUP) {eeErasingOnGoing = 1;eeStatus|= EE_CleanUp_IT();}
  while(eeErasingOnGoing){};
  eeStatus |= EE_WriteVariable32bits(EE_PACK_CONTROLLER_ID, 0);     // Pack controller ID 0 is default
  if ((eeStatus & EE_STATUSMASK_CLEANUP) == EE_STATUSMASK_CLEANUP) {eeErasingOnGoing = 1;eeStatus|= EE_CleanUp_IT();}
  while(eeErasingOnGoing){};

  // Lock the Flash Program Erase controller */
  HAL_FLASH_Lock();

  return eeStatus;

}
/***************************************************************************************************************
*     M A I N                                                                      P A C K   C O N T R O L L E R
***************************************************************************************************************/
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
  EE_Status eeStatus = EE_OK;

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

/* Configure the peripherals common clocks */
  PeriphCommonClock_Config();

  /* USER CODE BEGIN SysInit */

  /* Enable and set FLASH Interrupt priority */
    /* FLASH interrupt is used for the purpose of pages clean up under interrupt */
    HAL_NVIC_SetPriority(FLASH_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(FLASH_IRQn);

    /* Unlock the Flash Program Erase controller */
    HAL_FLASH_Unlock();

    /* Clear OPTVERR bit */
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_OPTVERR);
    while(__HAL_FLASH_GET_FLAG(FLASH_FLAG_OPTVERR) != RESET) ;


  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_SPI1_Init();
  MX_SPI2_Init();
  MX_USART1_UART_Init();
  MX_USB_PCD_Init();
  MX_PKA_Init();
  MX_RTC_Init();
  MX_LPUART1_UART_Init();
  MX_TIM1_Init();
  /* USER CODE BEGIN 2 */

  //HAL_GPIO_WritePin(LED_GREEN_GPIO_Port,  LED_GREEN_Pin , GPIO_PIN_SET);    // on


  //start the TIM Base generation in interrupt mode
  //HAL_TIM_OC_Start_IT( &htim1, TIM_CHANNEL_1 );
  HAL_TIM_Base_Start_IT(&htim1);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  // Set EEPROM emulation firmware to erase all potentially incompletely erased
  // pages if the system came from an asynchronous reset. Conditional erase is
  // safe to use if all Flash operations where completed before the system reset */
  if(__HAL_PWR_GET_FLAG(PWR_FLAG_SB) == RESET)
  {
    /* Blink LED_OK (Green) twice at startup
    BSP_LED_On(LED_OK);
    HAL_Delay(100);
    BSP_LED_Off(LED_OK);
    HAL_Delay(100);
    BSP_LED_On(LED_OK);
    HAL_Delay(100);
    BSP_LED_Off(LED_OK);
    */
    /* System reset comes from a power-on reset: Forced Erase */
    /* Initialize EEPROM emulation driver (mandatory) */
    eeStatus = EE_Init(EE_FORCED_ERASE);
    if(eeStatus != EE_OK) {Error_Handler();}
  }
  else
  {
    /* Clear the Standby flag */
    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_SB);

    /* Check and Clear the Wakeup flag */
    if (__HAL_PWR_GET_FLAG(PWR_FLAG_WUF) != RESET)
    {
      __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WUF);
    }

    /* Blink LED_OK (Green) upon wakeup
    BSP_LED_On(LED_OK);
    HAL_Delay(100);
    BSP_LED_Off(LED_OK);
    */
    /* System reset comes from a STANDBY wakeup: Conditional Erase*/
    /* Initialize EEPROM emulation driver (mandatory) */
    eeStatus = EE_Init(EE_CONDITIONAL_ERASE);
    if(eeStatus != EE_OK) {Error_Handler();}
  }


  LoadAllEEPROM();

  // Check EEPROM has been set up
  if(eeVarDataTab[EE_MAGIC1] != MAGIC1 || eeVarDataTab[EE_MAGIC2] != MAGIC2 ){
    // Not set up. Set up initial default values for EEPROM data
    eeStatus = eepromDefaults();

    LoadAllEEPROM();
  }
  HAL_FLASH_Lock();



  // Lock the Flash Program Erase controller */
  //HAL_FLASH_Lock();

  DRV_CANFDSPI_Reset(CAN2);
  DRV_CANFDSPI_Reset(CAN1);
  PCU_Initialize();

  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    PCU_Tasks();
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

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_MEDIUMHIGH);

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_HSE
                              |RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV2;
  RCC_OscInitStruct.PLL.PLLN = 8;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure the SYSCLKSource, HCLK, PCLK1 and PCLK2 clocks dividers
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK4|RCC_CLOCKTYPE_HCLK2
                              |RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.AHBCLK2Divider = RCC_SYSCLK_DIV2;
  RCC_ClkInitStruct.AHBCLK4Divider = RCC_SYSCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief Peripherals Common Clock Configuration
  * @retval None
  */
void PeriphCommonClock_Config(void)
{
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Initializes the peripherals clock
  */
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_SMPS;
  PeriphClkInitStruct.SmpsClockSelection = RCC_SMPSCLKSOURCE_HSE;
  PeriphClkInitStruct.SmpsDivSelection = RCC_SMPSCLKDIV_RANGE1;

  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN Smps */

  /* USER CODE END Smps */
}

/**
  * @brief LPUART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_LPUART1_UART_Init(void)
{

  /* USER CODE BEGIN LPUART1_Init 0 */

  /* USER CODE END LPUART1_Init 0 */

  /* USER CODE BEGIN LPUART1_Init 1 */

  /* USER CODE END LPUART1_Init 1 */
  hlpuart1.Instance = LPUART1;
  hlpuart1.Init.BaudRate = 115200;
  hlpuart1.Init.WordLength = UART_WORDLENGTH_8B;
  hlpuart1.Init.StopBits = UART_STOPBITS_1;
  hlpuart1.Init.Parity = UART_PARITY_NONE;
  hlpuart1.Init.Mode = UART_MODE_TX_RX;
  hlpuart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  hlpuart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  hlpuart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  hlpuart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  hlpuart1.FifoMode = UART_FIFOMODE_DISABLE;
  if (HAL_UART_Init(&hlpuart1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&hlpuart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&hlpuart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&hlpuart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN LPUART1_Init 2 */

  /* USER CODE END LPUART1_Init 2 */

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
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief PKA Initialization Function
  * @param None
  * @retval None
  */
static void MX_PKA_Init(void)
{

  /* USER CODE BEGIN PKA_Init 0 */

  /* USER CODE END PKA_Init 0 */

  /* USER CODE BEGIN PKA_Init 1 */

  /* USER CODE END PKA_Init 1 */
  hpka.Instance = PKA;
  if (HAL_PKA_Init(&hpka) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN PKA_Init 2 */

  /* USER CODE END PKA_Init 2 */

}

/**
  * @brief RTC Initialization Function
  * @param None
  * @retval None
  */
static void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */


  /* USER CODE END RTC_Init 0 */

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */

  /** Initialize RTC Only
  */
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  hrtc.Init.OutPutRemap = RTC_OUTPUT_REMAP_NONE;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */

  /*
   *
   *
  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef sDate = {0};

  //Initialize RTC and set the Time and Date
  sTime.Hours = 0x0;
  sTime.Minutes = 0x0;
  sTime.Seconds = 0x0;
  sTime.SubSeconds = 0x0;
  sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  sTime.StoreOperation = RTC_STOREOPERATION_RESET;
  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }
  sDate.WeekDay = RTC_WEEKDAY_MONDAY;
  sDate.Month = RTC_MONTH_JANUARY;
  sDate.Date = 0x1;
  sDate.Year = 0x0;


  if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }
  */
  /* USER CODE END RTC_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI2_Init(void)
{

  /* USER CODE BEGIN SPI2_Init 0 */

  /* USER CODE END SPI2_Init 0 */

  /* USER CODE BEGIN SPI2_Init 1 */

  /* USER CODE END SPI2_Init 1 */
  /* SPI2 parameter configuration*/
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_4BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 7;
  hspi2.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi2.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI2_Init 2 */

  /* USER CODE END SPI2_Init 2 */

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
  htim1.Init.Prescaler = 63999;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 99;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_OC_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_TIMING;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_OC_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.BreakFilter = 0;
  sBreakDeadTimeConfig.BreakAFMode = TIM_BREAK_AFMODE_INPUT;
  sBreakDeadTimeConfig.Break2State = TIM_BREAK2_DISABLE;
  sBreakDeadTimeConfig.Break2Polarity = TIM_BREAK2POLARITY_HIGH;
  sBreakDeadTimeConfig.Break2Filter = 0;
  sBreakDeadTimeConfig.Break2AFMode = TIM_BREAK_AFMODE_INPUT;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */

}

/**
  * @brief USB Initialization Function
  * @param None
  * @retval None
  */
static void MX_USB_PCD_Init(void)
{

  /* USER CODE BEGIN USB_Init 0 */

  /* USER CODE END USB_Init 0 */

  /* USER CODE BEGIN USB_Init 1 */

  /* USER CODE END USB_Init 1 */
  hpcd_USB_FS.Instance = USB;
  hpcd_USB_FS.Init.dev_endpoints = 8;
  hpcd_USB_FS.Init.speed = PCD_SPEED_FULL;
  hpcd_USB_FS.Init.phy_itface = PCD_PHY_EMBEDDED;
  hpcd_USB_FS.Init.Sof_enable = DISABLE;
  hpcd_USB_FS.Init.low_power_enable = DISABLE;
  hpcd_USB_FS.Init.lpm_enable = DISABLE;
  hpcd_USB_FS.Init.battery_charging_enable = DISABLE;
  if (HAL_PCD_Init(&hpcd_USB_FS) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USB_Init 2 */

  /* USER CODE END USB_Init 2 */

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

  if(hwPlatform == PLATFORM_NUCLEO){



CAN1_INT0_Pin         = GPIO_PIN_8;
CAN1_INT0_GPIO_Port   = GPIOB;
CAN1_INT0_EXTI_IRQn   = EXTI9_5_IRQn;
CAN1_INT1_Pin         = GPIO_PIN_9;
CAN1_INT1_GPIO_Port   = GPIOB;
CAN1_INT1_EXTI_IRQn   = EXTI9_5_IRQn;
SPI2_CS1_Pin          = GPIO_PIN_0;
SPI2_CS1_GPIO_Port    = GPIOC;
SPI2_CS2_Pin          = GPIO_PIN_3;
SPI2_CS2_GPIO_Port    = GPIOC;
CAN1_CS_Pin           = GPIO_PIN_5;
CAN1_CS_GPIO_Port     = GPIOA;
BUTTON1_Pin           = GPIO_PIN_4;
BUTTON1_GPIO_Port     = GPIOC;
BUTTON1_EXTI_IRQn     = EXTI4_IRQn;
CAN2_CS_Pin           = GPIO_PIN_5;
CAN2_CS_GPIO_Port     = GPIOC;
LED_GREEN_Pin         = GPIO_PIN_0;
LED_GREEN_GPIO_Port   = GPIOB;
LED_RED_Pin           = GPIO_PIN_1;
LED_RED_GPIO_Port     = GPIOB;
CAN2_INT_Pin          = GPIO_PIN_12;
CAN2_INT_GPIO_Port    = GPIOB;
CAN2_INT_EXTI_IRQn    = EXTI15_10_IRQn;
CAN2_INT0_Pin         = GPIO_PIN_13;
CAN2_INT0_GPIO_Port   = GPIOB;
CAN2_INT0_EXTI_IRQn   = EXTI15_10_IRQn;
CAN2_INT1_Pin         = GPIO_PIN_14;
CAN2_INT1_GPIO_Port   = GPIOB;
CAN2_INT1_EXTI_IRQn   = EXTI15_10_IRQn;
CAN1_INT_Pin          = GPIO_PIN_10;
CAN1_INT_GPIO_Port    = GPIOA;
CAN1_INT_EXTI_IRQn    = EXTI15_10_IRQn;
BUTTON2_Pin           = GPIO_PIN_0;
BUTTON2_GPIO_Port     = GPIOD;
BUTTON2_EXTI_IRQn     = EXTI0_IRQn;
BUTTON3_Pin           = GPIO_PIN_1;
BUTTON3_GPIO_Port     = GPIOD;
BUTTON3_EXTI_IRQn     = EXTI1_IRQn;
LED_BLUE_Pin          = GPIO_PIN_5;
LED_BLUE_GPIO_Port    = GPIOB;




/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, SPI2_CS1_Pin|SPI2_CS2_Pin|CAN2_CS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(CAN1_CS_GPIO_Port, CAN1_CS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LED_GREEN_Pin|LED_RED_Pin|LED_BLUE_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : CAN1_INT0_Pin CAN1_INT1_Pin CAN2_INT_Pin CAN2_INT0_Pin
                           CAN2_INT1_Pin */
  GPIO_InitStruct.Pin = CAN1_INT0_Pin|CAN1_INT1_Pin|CAN2_INT_Pin|CAN2_INT0_Pin
                          |CAN2_INT1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : SPI2_CS1_Pin SPI2_CS2_Pin CAN2_CS_Pin */
  GPIO_InitStruct.Pin = SPI2_CS1_Pin|SPI2_CS2_Pin|CAN2_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : CAN1_CS_Pin */
  GPIO_InitStruct.Pin = CAN1_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(CAN1_CS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : BUTTON1_Pin */
  GPIO_InitStruct.Pin = BUTTON1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(BUTTON1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LED_GREEN_Pin LED_RED_Pin LED_BLUE_Pin */
  GPIO_InitStruct.Pin = LED_GREEN_Pin|LED_RED_Pin|LED_BLUE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : CAN1_INT_Pin */
  GPIO_InitStruct.Pin = CAN1_INT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(CAN1_INT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : BUTTON2_Pin BUTTON3_Pin */
  GPIO_InitStruct.Pin = BUTTON2_Pin|BUTTON3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

  HAL_NVIC_SetPriority(EXTI1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);

  HAL_NVIC_SetPriority(EXTI4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI4_IRQn);

  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

/* USER CODE BEGIN MX_GPIO_Init_2 */
  } else{
    // PLATFORM_MODBATT

    // CAN2 - VCU
    CAN1_INT_Pin          = GPIO_PIN_15;
    CAN1_INT_GPIO_Port    = GPIOB;
    CAN1_INT_EXTI_IRQn    = EXTI15_10_IRQn;
    CAN1_INT0_Pin         = GPIO_PIN_6;
    CAN1_INT0_GPIO_Port   = GPIOC;
    CAN1_INT0_EXTI_IRQn   = EXTI9_5_IRQn;
    CAN1_INT1_Pin         = GPIO_PIN_10;
    CAN1_INT1_GPIO_Port   = GPIOA;
    CAN1_INT1_EXTI_IRQn   = EXTI9_5_IRQn;
    CAN1_CS_Pin           = GPIO_PIN_14;
    CAN1_CS_GPIO_Port     = GPIOB;

    // CAN2 - Module bank 0
    CAN2_INT_Pin          = GPIO_PIN_11;
    CAN2_INT_GPIO_Port    = GPIOC;
    CAN2_INT_EXTI_IRQn    = EXTI15_10_IRQn;
    CAN2_INT0_Pin         = GPIO_PIN_12;
    CAN2_INT0_GPIO_Port   = GPIOC;
    CAN2_INT0_EXTI_IRQn   = EXTI15_10_IRQn;
    CAN2_INT1_Pin         = GPIO_PIN_0;
    CAN2_INT1_GPIO_Port   = GPIOD;
    CAN2_INT1_EXTI_IRQn   = EXTI0_IRQn;;
    CAN2_CS_Pin           = GPIO_PIN_10;
    CAN2_CS_GPIO_Port     = GPIOC;

    // CAN3 - Module bank 1
    CAN3_INT_Pin          = GPIO_PIN_8;
    CAN3_INT_GPIO_Port    = GPIOB;
    CAN3_INT_EXTI_IRQn    = EXTI9_5_IRQn;
    CAN3_INT0_Pin         = GPIO_PIN_4;
    CAN3_INT0_GPIO_Port   = GPIOB;
    CAN3_INT0_EXTI_IRQn   = EXTI4_IRQn;
    CAN3_INT1_Pin         = GPIO_PIN_5;
    CAN3_INT1_GPIO_Port   = GPIOB;
    CAN3_INT1_EXTI_IRQn   = EXTI9_5_IRQn;;
    CAN3_CS_Pin           = GPIO_PIN_1;
    CAN3_CS_GPIO_Port     = GPIOD;

    // SPI2 = SDCARD
    SPI2_CS1_Pin          = GPIO_PIN_0;
    SPI2_CS1_GPIO_Port    = GPIOC;
    SPI2_CS2_Pin          = GPIO_PIN_3;
    SPI2_CS2_GPIO_Port    = GPIOC;


    // Buttons
    BUTTON1_Pin           = GPIO_PIN_11;
    BUTTON1_GPIO_Port     = GPIOB;
    BUTTON1_EXTI_IRQn     = EXTI15_10_IRQn;
    BUTTON2_Pin           = GPIO_PIN_10;
    BUTTON2_GPIO_Port     = GPIOB;
    BUTTON2_EXTI_IRQn     = EXTI0_IRQn;
    BUTTON3_Pin           = GPIO_PIN_2;
    BUTTON3_GPIO_Port     = GPIOB;
    BUTTON3_EXTI_IRQn     = EXTI2_IRQn;

      // LED's
    LED_CAN1_Pin         = GPIO_PIN_0;
    LED_CAN1_GPIO_Port   = GPIOB;
    LED_CAN2_Pin         = GPIO_PIN_1;
    LED_CAN2_GPIO_Port   = GPIOB;
    LED_CAN3_Pin         = GPIO_PIN_4;
    LED_CAN3_GPIO_Port   = GPIOE;
    LED_HBEAT_Pin        = GPIO_PIN_12;
    LED_HBEAT_GPIO_Port  = GPIOB;


  }
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
  //__disable_irq();

  sprintf(tempBuffer,"ERROR_HANDLER"); serialOut(tempBuffer);
  return;

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
