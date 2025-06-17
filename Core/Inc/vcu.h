 /**************************************************************************************************************
 * @file           : vcu.h                                                         P A C K   C O N T R O L L E R
 * @brief          : Header for vcu communications
 ***************************************************************************************************************
 * Copyright (C) 2023-2024 Modular Battery Technologies, Inc.
 * US Patents 11,380,942; 11,469,470; 11,575,270; others. All rights reserved
 **************************************************************************************************************/
#ifndef VCU_H_
#define VCU_H_

// Include files
#include "canfdspi_api.h"
#include "bms.h"


/***************************************************************************************************************
*
*                      Section: Type Definitions                                   P A C K   C O N T R O L L E R
*
***************************************************************************************************************/

//#define TEST_SPI

//! Use RX and TX Interrupt pins to check FIFO status
#define VCU_USE_RX_INT

// Transmit Channels
#define VCU_TX_FIFO CAN_FIFO_CH2

// Receive Channels
#define VCU_RX_FIFO CAN_FIFO_CH1


#define VCU_CURRENT_BASE            -1600       // amps
#define VCU_CURRENT_FACTOR          0.05        // amps
#define VCU_VOLTAGE_BASE            0           // volts
#define VCU_VOLTAGE_FACTOR          0.05        // volts
#define VCU_TEMPERATURE_BASE        -273        // degrees C
#define VCU_TEMPERATURE_FACTOR      0.03125     // degrees C
#define VCU_CELL_VOLTAGE_BASE       0           // volts
#define VCU_CELL_VOLTAGE_FACTOR     0.001       // volts
#define VCU_SOC_PERCENTAGE_BASE     0           // %
#define VCU_SOC_PERCENTAGE_FACTOR   0.0015625   // %
#define VCU_SOH_PERCENTAGE_BASE     0           // %
#define VCU_SOH_PERCENTAGE_FACTOR   0.4         // %
#define VCU_ISOLATION_FACTOR        0.001       // Ohms/Volt



extern packState vcuStateRequested;
extern uint32_t VCU_TicksSinceLastMessage(void);
extern void VCU_ReceiveMessages(void);
extern void VCU_TransmitBmsState(void);
extern void VCU_TransmitBmsData1(void);
extern void VCU_TransmitBmsData2(void);
extern void VCU_TransmitBmsData3(void);
extern void VCU_TransmitBmsData5(void);
extern void VCU_TransmitBmsData8(void);
extern void VCU_TransmitBmsData9(void);
extern void VCU_TransmitBmsData10(void);
extern void VCU_RequestTime(void);

#endif /* VCU_H_ */
