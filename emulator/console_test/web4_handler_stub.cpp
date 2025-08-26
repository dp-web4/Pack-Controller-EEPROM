// Simplified WEB4 handler for console testing
// This removes STM32 HAL dependencies

#include "web4_handler.h"
#include "can_id_bms_vcu.h"
#include "debug_messages.h"
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

// External functions - already declared in header or provided by test code
extern uint32_t HAL_GetTick(void);
extern void ShowDebugMessage(uint16_t messageId, ...);

// Private variables
static web4_key_state_t rxState;
static web4_keys_t storedKeys;

// Private function prototypes  
static bool ProcessKeyChunk(uint32_t canId, uint8_t* data, uint8_t length);
static uint8_t ExtractChunkNumber(uint32_t canId);
static web4_key_type_t GetKeyTypeFromCanId(uint32_t canId);
static uint8_t CalculateChecksum(uint8_t* data, uint16_t length);
static void ResetReceptionState(void);

void WEB4_Init(void) {
    memset(&rxState, 0, sizeof(rxState));
    memset(&storedKeys, 0, sizeof(storedKeys));
    
    if (WEB4_LoadKeysFromEEPROM()) {
        ShowDebugMessage(MSG_WEB4_KEYS_LOADED, 0, 0);
    } else {
        ShowDebugMessage(MSG_WEB4_NO_STORED_KEYS, 0, 0);
    }
}

bool WEB4_HandleCANMessage(uint32_t canId, uint8_t* data, uint8_t length) {
    uint16_t baseId = canId & 0x7FF;
    
    switch (baseId) {
        case ID_VCU_WEB4_PACK_KEY_HALF:
        case ID_VCU_WEB4_APP_KEY_HALF:
        case ID_VCU_WEB4_COMPONENT_IDS:
            return ProcessKeyChunk(canId, data, length);
            
        case ID_VCU_WEB4_KEY_STATUS:
            ShowDebugMessage(MSG_WEB4_STATUS_RECEIVED, canId, 0);
            return true;
            
        default:
            return false;
    }
}

static bool ProcessKeyChunk(uint32_t canId, uint8_t* data, uint8_t length) {
    if (length != WEB4_CHUNK_SIZE) {
        ShowDebugMessage(MSG_WEB4_INVALID_LENGTH, canId, length);
        return false;
    }
    
    uint8_t chunkNum = ExtractChunkNumber(canId);
    if (chunkNum >= WEB4_NUM_CHUNKS) {
        ShowDebugMessage(MSG_WEB4_INVALID_CHUNK, canId, chunkNum);
        WEB4_SendAcknowledgment(GetKeyTypeFromCanId(canId), chunkNum, WEB4_ACK_SEQUENCE_ERROR);
        return false;
    }
    
    web4_key_type_t keyType = GetKeyTypeFromCanId(canId);
    
    if (!rxState.reception_active || rxState.current_key_type != keyType) {
        ResetReceptionState();
        rxState.reception_active = true;
        rxState.current_key_type = keyType;
        rxState.expected_chunks = WEB4_NUM_CHUNKS;
        ShowDebugMessage(MSG_WEB4_RECEPTION_START, canId, keyType);
    }
    
    if (rxState.chunks_received & (1 << chunkNum)) {
        ShowDebugMessage(MSG_WEB4_DUPLICATE_CHUNK, canId, chunkNum);
        WEB4_SendAcknowledgment(keyType, chunkNum, WEB4_ACK_SUCCESS);
        return true;
    }
    
    memcpy(&rxState.buffer[chunkNum * WEB4_CHUNK_SIZE], data, WEB4_CHUNK_SIZE);
    rxState.chunks_received |= (1 << chunkNum);
    rxState.last_chunk_time = HAL_GetTick();
    
    ShowDebugMessage(MSG_WEB4_CHUNK_RECEIVED, canId, chunkNum);
    WEB4_SendAcknowledgment(keyType, chunkNum, WEB4_ACK_SUCCESS);
    
    if (rxState.chunks_received == ((1 << WEB4_NUM_CHUNKS) - 1)) {
        uint8_t calc_checksum = CalculateChecksum(rxState.buffer, WEB4_KEY_SIZE - 1);
        uint8_t recv_checksum = rxState.buffer[WEB4_KEY_SIZE - 1];
        
        if (calc_checksum != recv_checksum) {
            ShowDebugMessage(MSG_WEB4_CHECKSUM_ERROR, calc_checksum, recv_checksum);
            WEB4_SendAcknowledgment(keyType, WEB4_NUM_CHUNKS - 1, WEB4_ACK_CHECKSUM_ERROR);
            ResetReceptionState();
            return false;
        }
        
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
                memcpy(storedKeys.pack_component_id, rxState.buffer, 32);
                memcpy(storedKeys.app_component_id, &rxState.buffer[32], 32);
                storedKeys.component_ids_valid = true;
                ShowDebugMessage(MSG_WEB4_COMPONENT_IDS_STORED, 0, 0);
                break;
        }
        
        if (storedKeys.pack_key_valid && storedKeys.app_key_valid && storedKeys.component_ids_valid) {
            if (WEB4_StoreKeysToEEPROM()) {
                ShowDebugMessage(MSG_WEB4_KEYS_SAVED_EEPROM, 0, 0);
            }
        }
        
        ResetReceptionState();
    }
    
    return true;
}

static uint8_t ExtractChunkNumber(uint32_t canId) {
    return (canId >> 8) & 0x07;
}

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
            return WEB4_KEY_PACK_DEVICE;
    }
}

static uint8_t CalculateChecksum(uint8_t* data, uint16_t length) {
    uint8_t checksum = 0;
    for (uint16_t i = 0; i < length; i++) {
        checksum ^= data[i];
    }
    return checksum;
}

static void ResetReceptionState(void) {
    memset(&rxState, 0, sizeof(rxState));
}

void WEB4_CheckTimeouts(void) {
    if (!rxState.reception_active) {
        return;
    }
    
    if ((HAL_GetTick() - rxState.last_chunk_time) > 5000) {
        ShowDebugMessage(MSG_WEB4_RECEPTION_TIMEOUT, rxState.current_key_type, rxState.chunks_received);
        ResetReceptionState();
    }
}

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

bool WEB4_KeysValid(void) {
    return storedKeys.pack_key_valid && 
           storedKeys.app_key_valid && 
           storedKeys.component_ids_valid;
}

void WEB4_PrintKeyStatus(void) {
    ShowDebugMessage(MSG_WEB4_KEY_STATUS, 
                     storedKeys.pack_key_valid, 
                     storedKeys.app_key_valid);
    ShowDebugMessage(MSG_WEB4_COMPONENT_STATUS, 
                     storedKeys.component_ids_valid, 0);
}

void WEB4_PrintReceivedChunk(uint32_t canId, uint8_t* data) {
    ShowDebugMessage(MSG_WEB4_CHUNK_DATA, canId, data[0]);
}