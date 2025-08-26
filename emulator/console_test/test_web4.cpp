// Additional WEB4 testing functions
#include <iostream>
#include <cstdint>
#include <cstring>

extern "C" {
    #include "web4_handler.h"
}

// Mock implementations for EEPROM functions that WEB4 handler needs
extern "C" {
    
// Mock EEPROM storage (just in memory for testing)
static uint8_t mock_eeprom[4096];
static bool eeprom_initialized = false;

bool WEB4_StoreKeysToEEPROM() {
    std::cout << "[EEPROM] Storing keys to EEPROM (mocked)" << std::endl;
    // In a real implementation, this would write to EEPROM
    // For testing, we just return success
    eeprom_initialized = true;
    return true;
}

bool WEB4_LoadKeysFromEEPROM() {
    std::cout << "[EEPROM] Loading keys from EEPROM (mocked)" << std::endl;
    // In a real implementation, this would read from EEPROM
    // For testing, return false to simulate no stored keys
    return false;
}

void WEB4_SendAcknowledgment(web4_key_type_t keyType, uint8_t chunkNum, web4_ack_status_t status) {
    const char* keyTypeName = "UNKNOWN";
    switch(keyType) {
        case WEB4_KEY_PACK_DEVICE: keyTypeName = "PACK_DEVICE"; break;
        case WEB4_KEY_APP_DEVICE: keyTypeName = "APP_DEVICE"; break;
        case WEB4_KEY_COMPONENT_ID: keyTypeName = "COMPONENT_ID"; break;
    }
    
    const char* statusName = "UNKNOWN";
    switch(status) {
        case WEB4_ACK_SUCCESS: statusName = "SUCCESS"; break;
        case WEB4_ACK_CHECKSUM_ERROR: statusName = "CHECKSUM_ERROR"; break;
        case WEB4_ACK_SEQUENCE_ERROR: statusName = "SEQUENCE_ERROR"; break;
        case WEB4_ACK_TIMEOUT: statusName = "TIMEOUT"; break;
        case WEB4_ACK_STORAGE_ERROR: statusName = "STORAGE_ERROR"; break;
        default: statusName = "UNKNOWN"; break;
    }
    
    std::cout << "[CAN TX] Sending ACK - Type: " << keyTypeName 
              << ", Chunk: " << (int)chunkNum 
              << ", Status: " << statusName << std::endl;
}

} // extern "C"