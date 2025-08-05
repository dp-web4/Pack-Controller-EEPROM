/**************************************************************************************************************
 * @file           : debug.h                                                        P A C K   C O N T R O L L E R
 * @brief          : Debug message system definitions and configuration
 ***************************************************************************************************************
 * Copyright (C) 2023-2024 Modular Battery Technologies, Inc.
 * US Patents 11,380,942; 11,469,470; 11,575,270; others. All rights reserved
 **************************************************************************************************************/
#ifndef DEBUG_H_
#define DEBUG_H_

#include <stdint.h>

/***************************************************************************************************************
 * Debug Level Definitions
 * These control which categories of messages are enabled
 ***************************************************************************************************************/
#define DBG_DISABLED    0x00
#define DBG_ERRORS      0x01
#define DBG_COMMS       0x02  // TX and RX messages
#define DBG_MCU         0x08
#define DBG_VCU         0x10
#define DBG_VERBOSE     0x80
#define DBG_ALL         0xFF

/***************************************************************************************************************
 * Debug Message Flags
 * Individual control for each message type
 ***************************************************************************************************************/
#define DBG_MSG_NONE              0x00000000
#define DBG_MSG_ANNOUNCE_REQ      0x00000001  // 0x51D TX
#define DBG_MSG_ANNOUNCE          0x00000002  // 0x500 RX
#define DBG_MSG_REGISTRATION      0x00000004  // 0x510 TX
#define DBG_MSG_STATUS_REQ        0x00000008  // 0x512 TX
#define DBG_MSG_STATUS1           0x00000010  // 0x502 RX
#define DBG_MSG_STATUS2           0x00000020  // 0x503 RX
#define DBG_MSG_STATUS3           0x00000040  // 0x504 RX
#define DBG_MSG_STATE_CHANGE      0x00000080  // 0x514 TX
#define DBG_MSG_HARDWARE_REQ      0x00000100  // 0x511 TX
#define DBG_MSG_HARDWARE          0x00000200  // 0x501 RX
#define DBG_MSG_CELL_DETAIL       0x00000400  // 0x505 RX
#define DBG_MSG_CELL_STATUS1      0x00000800  // 0x507 RX
#define DBG_MSG_CELL_STATUS2      0x00001000  // 0x508 RX
#define DBG_MSG_TIME_REQ          0x00002000  // 0x506 RX
#define DBG_MSG_SET_TIME          0x00004000  // 0x516 TX
#define DBG_MSG_MAX_STATE         0x00008000  // 0x517 TX
#define DBG_MSG_DEREGISTER        0x00010000  // 0x518 TX
#define DBG_MSG_ISOLATE_ALL       0x00020000  // 0x51F TX
#define DBG_MSG_DEREGISTER_ALL    0x00040000  // 0x51E TX
#define DBG_MSG_POLLING           0x00080000  // Round-robin polling
#define DBG_MSG_TIMEOUT           0x00100000  // Timeout events
#define DBG_MSG_MINIMAL           0x00200000  // Minimal status pulse output
#define DBG_MSG_ALL               0xFFFFFFFF

// Message groups for convenience
#define DBG_MSG_REGISTRATION_GROUP (DBG_MSG_ANNOUNCE_REQ | DBG_MSG_ANNOUNCE | DBG_MSG_REGISTRATION)
#define DBG_MSG_STATUS_GROUP      (DBG_MSG_STATUS_REQ | DBG_MSG_STATUS1 | DBG_MSG_STATUS2 | DBG_MSG_STATUS3)
#define DBG_MSG_CELL_GROUP        (DBG_MSG_CELL_DETAIL | DBG_MSG_CELL_STATUS1 | DBG_MSG_CELL_STATUS2)

/***************************************************************************************************************
 * Debug Configuration
 * Edit these to control what gets displayed
 ***************************************************************************************************************/
#define DEBUG_LEVEL     (DBG_ERRORS)
#define DEBUG_MESSAGES  (DBG_MSG_REGISTRATION_GROUP | DBG_MSG_DEREGISTER | DBG_MSG_DEREGISTER_ALL | DBG_MSG_TIMEOUT | DBG_MSG_STATUS1 | DBG_MSG_MINIMAL)

/***************************************************************************************************************
 * Function Prototypes
 ***************************************************************************************************************/
void ShowDebugMessage(uint16_t messageId);

#endif /* DEBUG_H_ */