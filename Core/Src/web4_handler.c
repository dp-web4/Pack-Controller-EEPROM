/**************************************************************************************************************
 * @file           : web4_handler.c                                                P A C K   C O N T R O L L E R
 * @brief          : WEB4 key distribution and encryption handler implementation
 ***************************************************************************************************************
 * Copyright (C) 2025 Modular Battery Technologies, Inc.
 * US Patents 11,380,942; 11,469,470; 11,575,270; others. All rights reserved
 **************************************************************************************************************/

#include "web4_handler.h"
#include "can_id_bms_vcu.h"
#include "debug.h"
#include "main.h"  // For HAL functions
#include <string.h>

/* Private Variables */
static web4_key_state_t rxState;
static web4_keys_t storedKeys;

/* Private Function Prototypes */
static bool ProcessKeyChunk(uint32_t canId, uint8_t* data, uint8_t length);
static uint8_t ExtractChunkNumber(uint32_t canId);
static web4_key_type_t GetKeyTypeFromCanId(uint32_t canId);
static uint8_t CalculateChecksum(uint8_t* data, uint16_t length);
static void ResetReceptionState(void);

/**
 * @brief Initialize WEB4 handler
 */
void WEB4_Init(void) {
    // Clear reception state
    memset(&rxState, 0, sizeof(rxState));
    
    // Clear stored keys
    memset(&storedKeys, 0, sizeof(storedKeys));
    
    // Try to load keys from EEPROM
    if (WEB4_LoadKeysFromEEPROM()) {
        ShowDebugMessage(MSG_WEB4_KEYS_LOADED, 0, 0);
    } else {
        ShowDebugMessage(MSG_WEB4_NO_STORED_KEYS, 0, 0);
    }
}

/**
 * @brief Main CAN message handler for WEB4 messages
 * @param canId CAN message ID (may include extended bits for chunk number)
 * @param data Pointer to message data
 * @param length Data length (should be 8 for key chunks)
 * @return true if message was handled, false otherwise
 */
bool WEB4_HandleCANMessage(uint32_t canId, uint8_t* data, uint8_t length) {
    uint16_t baseId = canId & 0x7FF;  // Extract base 11-bit ID
    
    // Check if this is a WEB4 message
    switch (baseId) {
        case ID_VCU_WEB4_PACK_KEY_HALF:
        case ID_VCU_WEB4_APP_KEY_HALF:
        case ID_VCU_WEB4_COMPONENT_IDS:
            return ProcessKeyChunk(canId, data, length);
            
        case ID_VCU_WEB4_KEY_STATUS:
            // TODO: Handle key status message
            ShowDebugMessage(MSG_WEB4_STATUS_RECEIVED, canId, 0);
            return true;
            
        default:
            return false;  // Not a WEB4 message
    }
}

/**
 * @brief Process a key chunk from CAN message
 */
static bool ProcessKeyChunk(uint32_t canId, uint8_t* data, uint8_t length) {
    if (length != WEB4_CHUNK_SIZE) {
        ShowDebugMessage(MSG_WEB4_INVALID_LENGTH, canId, length);
        return false;
    }
    
    // Extract chunk number from extended ID bits (bits 8-10)
    uint8_t chunkNum = ExtractChunkNumber(canId);
    if (chunkNum >= WEB4_NUM_CHUNKS) {
        ShowDebugMessage(MSG_WEB4_INVALID_CHUNK, canId, chunkNum);
        WEB4_SendAcknowledgment(GetKeyTypeFromCanId(canId), chunkNum, WEB4_ACK_SEQUENCE_ERROR);
        return false;
    }
    
    // Determine key type from base CAN ID
    web4_key_type_t keyType = GetKeyTypeFromCanId(canId);
    
    // Start new reception if needed
    if (!rxState.reception_active || rxState.current_key_type != keyType) {
        ResetReceptionState();
        rxState.reception_active = true;
        rxState.current_key_type = keyType;
        rxState.expected_chunks = WEB4_NUM_CHUNKS;
        ShowDebugMessage(MSG_WEB4_RECEPTION_START, canId, keyType);
    }
    
    // Check if chunk already received
    if (rxState.chunks_received & (1 << chunkNum)) {
        ShowDebugMessage(MSG_WEB4_DUPLICATE_CHUNK, canId, chunkNum);
        // Still send ACK for duplicate
        WEB4_SendAcknowledgment(keyType, chunkNum, WEB4_ACK_SUCCESS);
        return true;
    }
    
    // Store chunk data
    memcpy(&rxState.buffer[chunkNum * WEB4_CHUNK_SIZE], data, WEB4_CHUNK_SIZE);
    rxState.chunks_received |= (1 << chunkNum);
    rxState.last_chunk_time = HAL_GetTick();
    
    ShowDebugMessage(MSG_WEB4_CHUNK_RECEIVED, canId, chunkNum);
    
    // Send ACK for this chunk
    WEB4_SendAcknowledgment(keyType, chunkNum, WEB4_ACK_SUCCESS);
    
    // Check if all chunks received
    if (rxState.chunks_received == ((1 << WEB4_NUM_CHUNKS) - 1)) {
        // All chunks received - verify checksum
        uint8_t calc_checksum = CalculateChecksum(rxState.buffer, WEB4_KEY_SIZE - 1);
        uint8_t recv_checksum = rxState.buffer[WEB4_KEY_SIZE - 1];
        
        if (calc_checksum != recv_checksum) {
            ShowDebugMessage(MSG_WEB4_CHECKSUM_ERROR, calc_checksum, recv_checksum);
            // Send NACK for last chunk with checksum error
            WEB4_SendAcknowledgment(keyType, WEB4_NUM_CHUNKS - 1, WEB4_ACK_CHECKSUM_ERROR);
            ResetReceptionState();
            return false;
        }
        
        // Store complete key
        switch (keyType) {
            case WEB4_KEY_PACK_DEVICE:
                memcpy(storedKeys.pack_device_key, rxState.buffer, WEB4_KEY_SIZE);
                storedKeys.pack_key_valid = true;
                ShowDebugMessage(MSG_WEB4_PACK_KEY_STORED, 0, 0);
                break;
                
            case WEB4_KEY_APP_DEVICE:
                memcpy(storedKeys.app_device_key, rxState.buffer, WEB4_KEY_SIZE);
                storedKeys.app_key_valid = true;
                ShowDebugMessage(MSG_WEB4_APP_KEY_STORED, 0, 0);
                break;
                
            case WEB4_KEY_COMPONENT_ID:
                // Component IDs split between pack and app
                memcpy(storedKeys.pack_component_id, rxState.buffer, 32);
                memcpy(storedKeys.app_component_id, &rxState.buffer[32], 32);
                storedKeys.component_ids_valid = true;
                ShowDebugMessage(MSG_WEB4_COMPONENT_IDS_STORED, 0, 0);
                break;
        }
        
        // Save to EEPROM if all keys received
        if (storedKeys.pack_key_valid && storedKeys.app_key_valid && storedKeys.component_ids_valid) {
            if (WEB4_StoreKeysToEEPROM()) {
                ShowDebugMessage(MSG_WEB4_KEYS_SAVED_EEPROM, 0, 0);
            }
        }
        
        ResetReceptionState();
    }
    
    return true;
}

/**
 * @brief Extract chunk number from extended CAN ID
 */
static uint8_t ExtractChunkNumber(uint32_t canId) {
    // Chunk number is in bits 8-10 of the CAN ID
    return (canId >> 8) & 0x07;
}

/**
 * @brief Get key type from CAN ID
 */
static web4_key_type_t GetKeyTypeFromCanId(uint32_t canId) {
    uint16_t baseId = canId & 0x7FF;
    
    switch (baseId) {
        case ID_VCU_WEB4_PACK_KEY_HALF:
            return WEB4_KEY_PACK_DEVICE;
        case ID_VCU_WEB4_APP_KEY_HALF:
            return WEB4_KEY_APP_DEVICE;
        case ID_VCU_WEB4_COMPONENT_IDS:
            return WEB4_KEY_COMPONENT_ID;
        default:
            return WEB4_KEY_PACK_DEVICE;  // Default
    }
}

/**
 * @brief Calculate simple XOR checksum
 */
static uint8_t CalculateChecksum(uint8_t* data, uint16_t length) {
    uint8_t checksum = 0;
    for (uint16_t i = 0; i < length; i++) {
        checksum ^= data[i];
    }
    return checksum;
}

/**
 * @brief Reset reception state
 */
static void ResetReceptionState(void) {
    memset(&rxState, 0, sizeof(rxState));
}

/**
 * @brief Send ACK/NACK response
 */
void WEB4_SendAcknowledgment(web4_key_type_t keyType, uint8_t chunkNum, web4_ack_status_t status) {
    // TODO: Implement CAN transmission
    // This needs to interface with your CAN driver
    
    uint32_t ackId;
    switch (keyType) {
        case WEB4_KEY_PACK_DEVICE:
            ackId = ID_BMS_WEB4_PACK_KEY_ACK;
            break;
        case WEB4_KEY_APP_DEVICE:
            ackId = ID_BMS_WEB4_APP_KEY_ACK;
            break;
        case WEB4_KEY_COMPONENT_ID:
            ackId = ID_BMS_WEB4_COMPONENT_ACK;
            break;
        default:
            return;
    }
    
    uint8_t ackData[8] = {0};
    ackData[0] = chunkNum;
    ackData[1] = status;
    
    // TODO: Call your CAN transmit function here
    // Example: CAN_Transmit(ackId, ackData, 8);
    
    ShowDebugMessage(MSG_WEB4_ACK_SENT, ackId, status);
}

/**
 * @brief Check for reception timeouts
 */
void WEB4_CheckTimeouts(void) {
    if (!rxState.reception_active) {
        return;
    }
    
    // Check if reception has timed out (5 seconds)
    if ((HAL_GetTick() - rxState.last_chunk_time) > 5000) {
        ShowDebugMessage(MSG_WEB4_RECEPTION_TIMEOUT, rxState.current_key_type, rxState.chunks_received);
        ResetReceptionState();
    }
}

/**
 * @brief Store keys to EEPROM
 */
bool WEB4_StoreKeysToEEPROM(void) {
    // TODO: Implement EEPROM storage
    // This needs to interface with your EEPROM driver
    // Consider using the existing EEPROM emulation system
    
    // Placeholder - return success for now
    return true;
}

/**
 * @brief Load keys from EEPROM
 */
bool WEB4_LoadKeysFromEEPROM(void) {
    // TODO: Implement EEPROM loading
    // This needs to interface with your EEPROM driver
    
    // Placeholder - return false (no keys stored)
    return false;
}

/**
 * @brief Get key for encryption/decryption
 */
bool WEB4_GetKey(web4_key_type_t keyType, uint8_t* keyBuffer) {
    if (!keyBuffer) {
        return false;
    }
    
    switch (keyType) {
        case WEB4_KEY_PACK_DEVICE:
            if (!storedKeys.pack_key_valid) {
                return false;
            }
            memcpy(keyBuffer, storedKeys.pack_device_key, WEB4_KEY_SIZE);
            return true;
            
        case WEB4_KEY_APP_DEVICE:
            if (!storedKeys.app_key_valid) {
                return false;
            }
            memcpy(keyBuffer, storedKeys.app_device_key, WEB4_KEY_SIZE);
            return true;
            
        case WEB4_KEY_COMPONENT_ID:
            if (!storedKeys.component_ids_valid) {
                return false;
            }
            memcpy(keyBuffer, storedKeys.pack_component_id, WEB4_COMPONENT_ID_SIZE);
            return true;
            
        default:
            return false;
    }
}

/**
 * @brief Check if keys are valid
 */
bool WEB4_KeysValid(void) {
    return storedKeys.pack_key_valid && 
           storedKeys.app_key_valid && 
           storedKeys.component_ids_valid;
}

/**
 * @brief Print key status for debugging
 */
void WEB4_PrintKeyStatus(void) {
    ShowDebugMessage(MSG_WEB4_KEY_STATUS, 
                     storedKeys.pack_key_valid, 
                     storedKeys.app_key_valid);
    ShowDebugMessage(MSG_WEB4_COMPONENT_STATUS, 
                     storedKeys.component_ids_valid, 0);
}

/**
 * @brief Print received chunk for debugging
 */
void WEB4_PrintReceivedChunk(uint32_t canId, uint8_t* data) {
    // This would output the chunk data for debugging
    // Implementation depends on your debug output system
    ShowDebugMessage(MSG_WEB4_CHUNK_DATA, canId, data[0]);
}