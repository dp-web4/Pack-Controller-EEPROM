/**************************************************************************************************************
 * @file           : web4_handler.h                                                P A C K   C O N T R O L L E R
 * @brief          : WEB4 key distribution and encryption handler
 ***************************************************************************************************************
 * Copyright (C) 2025 Modular Battery Technologies, Inc.
 * US Patents 11,380,942; 11,469,470; 11,575,270; others. All rights reserved
 **************************************************************************************************************/
#ifndef INC_WEB4_HANDLER_H_
#define INC_WEB4_HANDLER_H_

#include <stdint.h>
#include <stdbool.h>

/* WEB4 Key Storage Sizes */
#define WEB4_KEY_SIZE           64      // 64 bytes (512 bits) per key half
#define WEB4_COMPONENT_ID_SIZE  64      // Component ID max size
#define WEB4_CHUNK_SIZE         8       // 8 bytes per CAN message
#define WEB4_NUM_CHUNKS         8       // 64 bytes / 8 bytes per chunk

/* WEB4 Key Types */
typedef enum {
    WEB4_KEY_PACK_DEVICE,    // Pack controller's device key half
    WEB4_KEY_APP_DEVICE,     // App's device key half
    WEB4_KEY_COMPONENT_ID    // Component IDs
} web4_key_type_t;

/* WEB4 ACK/NACK Types */
typedef enum {
    WEB4_ACK_SUCCESS = 0x00,
    WEB4_ACK_CHECKSUM_ERROR = 0x01,
    WEB4_ACK_SEQUENCE_ERROR = 0x02,
    WEB4_ACK_STORAGE_ERROR = 0x03,
    WEB4_ACK_TIMEOUT = 0x04
} web4_ack_status_t;

/* WEB4 Key Reception State */
typedef struct {
    uint8_t buffer[WEB4_KEY_SIZE];     // Buffer for incoming key
    uint8_t chunks_received;           // Bitmask of received chunks
    uint8_t expected_chunks;           // Total number of chunks expected
    uint8_t current_key_type;          // Type of key being received
    uint32_t last_chunk_time;          // Timestamp of last chunk (for timeout)
    bool reception_active;             // True when receiving a key
} web4_key_state_t;

/* WEB4 Stored Keys */
typedef struct {
    uint8_t pack_device_key[WEB4_KEY_SIZE];
    uint8_t app_device_key[WEB4_KEY_SIZE];
    uint8_t pack_component_id[WEB4_COMPONENT_ID_SIZE];
    uint8_t app_component_id[WEB4_COMPONENT_ID_SIZE];
    bool pack_key_valid;
    bool app_key_valid;
    bool component_ids_valid;
} web4_keys_t;

/* Function Prototypes */

// Initialize WEB4 handler
void WEB4_Init(void);

// Main CAN message handler - call from mcu.c when WEB4 message received
bool WEB4_HandleCANMessage(uint32_t canId, uint8_t* data, uint8_t length);

// Send ACK/NACK response
void WEB4_SendAcknowledgment(web4_key_type_t keyType, uint8_t chunkNum, web4_ack_status_t status);

// Check for timeouts (call periodically from main loop)
void WEB4_CheckTimeouts(void);

// Store keys to EEPROM
bool WEB4_StoreKeysToEEPROM(void);

// Load keys from EEPROM
bool WEB4_LoadKeysFromEEPROM(void);

// Get key for encryption/decryption
bool WEB4_GetKey(web4_key_type_t keyType, uint8_t* keyBuffer);

// Check if keys are valid
bool WEB4_KeysValid(void);

// Debug functions
void WEB4_PrintKeyStatus(void);
void WEB4_PrintReceivedChunk(uint32_t canId, uint8_t* data);

#endif /* INC_WEB4_HANDLER_H_ */