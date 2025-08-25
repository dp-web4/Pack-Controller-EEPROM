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
#define DBG_MSG_VOLTAGE_SEL       0x00400000  // Voltage selection messages
#define DBG_MSG_CAN_ERRORS        0x00800000  // CAN error messages (unknown IDs, etc)
#define DBG_MSG_REG_EVENTS        0x01000000  // Registration events
#define DBG_MSG_CELL_DETAIL_REQ   0x02000000  // Cell detail request
#define DBG_MSG_TX_FIFO_ERROR     0x04000000  // TX FIFO error messages (separate flag)
#define DBG_MSG_POLLING_DETAIL    0x08000000  // Detailed polling information
#define DBG_MSG_STATE_MACHINE     0x10000000  // State machine transitions
#define DBG_MSG_ALL               0xFFFFFFFF

// Message groups for convenience
#define DBG_MSG_REGISTRATION_GROUP (DBG_MSG_ANNOUNCE_REQ | DBG_MSG_ANNOUNCE | DBG_MSG_REGISTRATION)
#define DBG_MSG_STATUS_GROUP      (DBG_MSG_STATUS_REQ | DBG_MSG_STATUS1 | DBG_MSG_STATUS2 | DBG_MSG_STATUS3)
#define DBG_MSG_CELL_GROUP        (DBG_MSG_CELL_DETAIL | DBG_MSG_CELL_STATUS1 | DBG_MSG_CELL_STATUS2)

/***************************************************************************************************************
 * Debug Configuration
 * Edit these to control what gets displayed
 ***************************************************************************************************************/
#define DEBUG_LEVEL     (DBG_ERRORS | DBG_COMMS | DBG_MCU)
#define DEBUG_MESSAGES  (DBG_MSG_REGISTRATION_GROUP | DBG_MSG_DEREGISTER | DBG_MSG_DEREGISTER_ALL | \
                        DBG_MSG_TIMEOUT | DBG_MSG_STATUS_REQ | DBG_MSG_STATUS1 | DBG_MSG_MINIMAL | \
                        DBG_MSG_REG_EVENTS | DBG_MSG_CAN_ERRORS | \
                        DBG_MSG_POLLING_DETAIL | DBG_MSG_STATE_MACHINE)

// Define which message types should only be shown once per power cycle
// These messages will show the first occurrence only, then be suppressed until reset
#define DEBUG_ONCE_ONLY (DBG_MSG_CAN_ERRORS | DBG_MSG_TX_FIFO_ERROR | \
                        DBG_MSG_POLLING_DETAIL | DBG_MSG_STATE_MACHINE)

/***************************************************************************************************************
 * Type Definitions
 ***************************************************************************************************************/
typedef struct {
    uint16_t messageId;        // CAN message ID or special message type
    uint8_t  requiredLevel;    // Required debug level (DBG_COMMS, DBG_ERRORS, etc)
    uint32_t requiredFlag;     // Required message flag (DBG_MSG_STATUS1, etc)
    const char* fullFormat;    // Full message format string (NULL if none)
    const char* minFormat;     // Minimal format string (NULL if none)
} DebugMessageDef;

// Special message IDs for non-CAN messages (internal Pack Controller events)
// These use 0xF000+ range to distinguish from actual CAN message IDs (0x500-0x51F)
#define MSG_TIMEOUT_WARNING    0xF001  // Module timeout detected
#define MSG_DEREGISTER        0xF002  // Module being removed from pack
#define MSG_MODULE_TIMEOUT    0xF003  // Module final timeout
#define MSG_VOLTAGE_SELECTION  0xF004  // Module voltage selection info
#define MSG_UNKNOWN_CAN_ID     0xF005  // Unknown CAN message received
#define MSG_TX_FIFO_ERROR      0xF006  // CAN TX FIFO error
#define MSG_MODULE_REREGISTER  0xF007  // Module re-registration
#define MSG_NEW_MODULE_REG     0xF008  // New module registration
#define MSG_UNREGISTERED_MOD   0xF009  // Unregistered module error
#define MSG_TIMEOUT_RESET      0xF00A  // Timeout counter reset
#define MSG_CELL_DETAIL_REQ    0xF00B  // Cell detail request

// Polling and status monitoring messages (0xF00C - 0xF00F range)
#define MSG_POLLING_CYCLE      0xF00C  // Start of polling cycle
#define MSG_MODULE_CHECK       0xF00D  // Per-module status check
#define MSG_STATUS_REQUEST     0xF00E  // Status request sent
#define MSG_STATE_TRANSITION   0xF00F  // Module state machine transition

// WEB4 Key Distribution Messages (0xF010 - 0xF021 range)
#define MSG_WEB4_KEYS_LOADED        0xF010  // Keys loaded from EEPROM
#define MSG_WEB4_NO_STORED_KEYS     0xF011  // No keys in EEPROM
#define MSG_WEB4_STATUS_RECEIVED    0xF012  // Key status message received
#define MSG_WEB4_INVALID_LENGTH     0xF013  // Invalid message length
#define MSG_WEB4_INVALID_CHUNK      0xF014  // Invalid chunk number
#define MSG_WEB4_RECEPTION_START    0xF015  // Key reception started
#define MSG_WEB4_DUPLICATE_CHUNK    0xF016  // Duplicate chunk received
#define MSG_WEB4_CHUNK_RECEIVED     0xF017  // Chunk successfully received
#define MSG_WEB4_CHECKSUM_ERROR     0xF018  // Checksum validation failed
#define MSG_WEB4_PACK_KEY_STORED    0xF019  // Pack key stored
#define MSG_WEB4_APP_KEY_STORED     0xF01A  // App key stored
#define MSG_WEB4_COMPONENT_IDS_STORED 0xF01B  // Component IDs stored
#define MSG_WEB4_KEYS_SAVED_EEPROM  0xF01C  // Keys saved to EEPROM
#define MSG_WEB4_ACK_SENT           0xF01D  // Acknowledgment sent
#define MSG_WEB4_RECEPTION_TIMEOUT  0xF01E  // Reception timeout
#define MSG_WEB4_KEY_STATUS         0xF01F  // Key validity status
#define MSG_WEB4_COMPONENT_STATUS   0xF020  // Component ID status
#define MSG_WEB4_CHUNK_DATA         0xF021  // Chunk data debug

/***************************************************************************************************************
 * Function Prototypes
 ***************************************************************************************************************/
void ShowDebugMessage(uint16_t messageId, ...);
void ResetDebugOnceOnly(void);  // Reset once-only tracking

// Global tracking for once-only messages (bits set as messages are shown)
extern uint32_t debugOnceShown;

#endif /* DEBUG_H_ */