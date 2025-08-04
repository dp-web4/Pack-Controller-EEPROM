 /**************************************************************************************************************
 * @file           : mcu.h                                                         P A C K   C O N T R O L L E R
 * @brief          : Header for mcu communications
 ***************************************************************************************************************
 * Copyright (C) 2023-2024 Modular Battery Technologies, Inc.
 * US Patents 11,380,942; 11,469,470; 11,575,270; others. All rights reserved
 **************************************************************************************************************/
#ifndef MCU_H_
#define MCU_H_

// Include files
#include "canfdspi_api.h"
//#include "main.h"
#include "bms.h"

/***************************************************************************************************************
*
*                      Section: Type Definitions                                   P A C K   C O N T R O L L E R
*
***************************************************************************************************************/

#define HW_VER     0
#define FW_VER     0

//! Use RX and TX Interrupt pins to check FIFO status
#define MCU_USE_RX_INT
#define MAX_TXQUEUE_ATTEMPTS 50

// Switches
#define MCU_SWITCH_RELEASED true  //Switch has an internal pullup when not pressed - input = 1
#define MCU_SWITCH_PRESSED  false  //Switch connects to ground when pressed - input  = 0
#define MCU_DEBOUNCE_TIME  100
#define MCU_S1_READ() (HAL_GPIO_ReadPin(BUTTON1_GPIO_Port, BUTTON1_Pin))


// Interrupt pin GPIO macros
#define MCU_INT()		  (HAL_GPIO_ReadPin(CAN2_INT_GPIO_Port,  CAN2_INT_Pin))
#define MCU_TX_INT()	(HAL_GPIO_ReadPin(CAN2_INT0_GPIO_Port, CAN2_INT0_Pin))
#define MCU_RX_INT()	(HAL_GPIO_ReadPin(CAN2_INT1_GPIO_Port, CAN2_INT1_Pin))

// Transmit Channels
#define MCU_TX_FIFO CAN_FIFO_CH2

// Receive Channels
#define MCU_RX_FIFO CAN_FIFO_CH1

#define MCU_STATUS_INTERVAL       2000      // Module status request interval - 2 seconds
#define MCU_STATE_TX_INTERVAL     1000      // Module state retry interval - 1 seconds
#define MCU_ET_TIMEOUT            4000      // Module timeout 4 seconds
#define VCU_ET_WARNING            600       // VCU warning 0.6 seconds
#define VCU_ET_TIMEOUT            1200      // VCU timeout 1.2 seconds
#define MCU_ANNOUNCE_REQUEST_INTERVAL 10000 // Module announcement request interval - 10 seconds
#define MCU_MAX_CONSECUTIVE_TIMEOUTS  3     // Maximum consecutive timeouts before deregistering

#define PACK_CURRENT_BASE         -1600     // amps
#define PACK_CURRENT_FACTOR       0.05      // amps
#define MODULE_VOLTAGE_BASE       0         // Volts
#define MODULE_VOLTAGE_FACTOR     0.015     // Volts
#define CELL_VOLTAGE_BASE         0         // Volts
#define CELL_VOLTAGE_FACTOR       0.001     // Volts
#define CELL_TOTAL_VOLTAGE_FACTOR 0.015     // Volts
#define MODULE_CURRENT_BASE       -655.36   // Amps
#define MODULE_CURRENT_FACTOR     0.02      // Amps
#define TEMPERATURE_BASE          -55.35    // Degrees C
#define TEMPERATURE_FACTOR        0.01      // Degrees C
#define PERCENTAGE_BASE           0         // Percent
#define PERCENTAGE_FACTOR         0.5       // Percent

#define MODULE_CURRENT_TOLERANCE  0.3       // +/- tolerance

#define PACK_EMPTY_SOC_THRESHOLD  5         // Percent
#define PACK_FULL_SOC_THRESHOLD   95        // Percent

#define MODULE_MAX_CHARGE_A       10.0      // Amps
#define MODULE_MAX_DISCHARGE_A   -42.0      // Amps


/***************************************************************************************************************
* Application states                                                               P A C K   C O N T R O L L E R

  Summary:
    Application states enumeration

  Description:
    This enumeration defines the valid application states.  These states
    determine the behavior of the application at various times.
***************************************************************************************************************/

typedef enum {
    // Initialization
    PC_STATE_INIT = 0,

    // Normal run state
    PC_STATE_RUN

} PC_STATES;

//extern PC_STATES PC_STATE;

/***************************************************************************************************************
* Application Data                                                                 P A C K   C O N T R O L L E R

  Summary:
    Holds application data

  Description:
    This structure holds the application's data.

  Remarks:
    Application strings and buffers are be defined outside this structure.
***************************************************************************************************************/

typedef struct {
    /* The application's current state */
    PC_STATES state;

    /* TODO: Define any additional data used by the application. */


} MCU_DATA;

extern MCU_DATA appData;
//extern batteryPack pack;

extern batteryModule module[MAX_MODULES_PER_PACK];

/***************************************************************************************************************
*
*       Section: Application Initialization and State Machine Functions            P A C K   C O N T R O L L E R
*
***************************************************************************************************************/
//! Application Initialization
void PCU_Initialize(void);

//! Application Tasks
// This routine must be called from SYS_Tasks() routine.
void PCU_Tasks(void);


//! Initialize CANFDSPI
void DRV_CANFDSPI_Init(CANFDSPI_MODULE_ID index);

//! Add message to transmit FIFO
void MCU_TransmitMessageQueue(CANFDSPI_MODULE_ID index);

//! Decode received messages
void MCU_ReceiveMessages(void);

void MCU_RegisterModule(void);
void MCU_DeRegisterModule(uint8_t moduleId);
void MCU_DeRegisterAllModules(void);
void MCU_IsolateAllModules(void);
void MCU_RequestModuleAnnouncement(void);
void MCU_RequestModuleStatus(uint8_t moduleId);
void MCU_ProcessModuleStatus1(void);
void MCU_ProcessModuleStatus2(void);
void MCU_ProcessModuleStatus3(void);

void MCU_RequestCellDetail(uint8_t moduleId);
void MCU_ProcessCellDetail(void);
void MCU_TransmitMaxState(moduleState state);

extern void MCU_TransmitState(uint8_t moduleId, moduleState state);

//! Transmit switch state
void MCU_TransmitSwitchState(void);

extern uint8_t MCU_FindMaxVoltageModule(void);
extern uint8_t MCU_ModuleIndexFromId(uint8_t moduleId);
extern void MCU_UpdateModuleCounts(void);
extern void MCU_UpdateModuleContact(uint8_t moduleIndex);
extern void MCU_ResetAllModuleTimeouts(void);
//extern void MCU_TransmitState(uint8_t moduleId, moduleState state);

//extern batteryModule module[MAX_MODULES_PER_PACK];

uint32_t MCU_TicksSinceLastMessage(uint8_t moduleId);

//! Test CAN SPI
bool CAN_TestRegisterAccess(CANFDSPI_MODULE_ID index);
bool CAN_TestRamAccess(CANFDSPI_MODULE_ID index);

#endif /* MCU_H_ */
