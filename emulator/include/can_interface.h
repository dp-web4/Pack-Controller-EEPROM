/******************************************************************************
 * @file    can_interface.h
 * @brief   PCAN-Basic wrapper for Pack Controller Emulator
 * @author  Pack Emulator Development Team
 * @date    December 2024
 * 
 * Copyright (C) 2025 Modular Battery Technologies, Inc.
 ******************************************************************************/

#ifndef CAN_INTERFACE_H
#define CAN_INTERFACE_H

#include <windows.h>
#include <cstdint>
#include <string>
#include <queue>

// PCAN-Basic API (assuming it's in the project like modbatt-CAN)
#include "PCANBasic.h"

// Include CAN ID definitions from STM32 firmware
extern "C" {
    #include "../../Core/Inc/can_id_module.h"    // Module CAN message IDs
    #include "../../Core/Inc/can_id_bms_vcu.h"   // VCU CAN message IDs
}

namespace PackEmulator {

// CAN message structure
struct CANMessage {
    uint32_t id;        // CAN ID (11-bit or 29-bit)
    uint8_t data[8];    // Data bytes
    uint8_t length;     // Data length (0-8)
    bool isExtended;    // Extended ID flag
    bool isRtr;         // Remote transmission request
    uint64_t timestamp; // Receive timestamp
};

// Forward declaration for callback interface
// Note: Named with 'I' prefix for interface convention, but not a COM interface
// Rename to avoid BCC32 COM interface warning
class CANCallbackInterface {
public:
    virtual void OnMessage(const CANMessage& msg) = 0;
    virtual void OnError(uint32_t errorCode, const std::string& errorMsg) = 0;
};

class CANInterface {
public:
    CANInterface();
    ~CANInterface();
    
    // Connection management
    bool Connect(uint16_t channel = PCAN_USBBUS1, uint16_t baudrate = PCAN_BAUD_500K);
    void Disconnect();
    bool IsConnected() const { return connected; }
    
    // Message transmission
    bool SendMessage(const CANMessage& msg);
    bool SendMessage(uint32_t id, const uint8_t* data, uint8_t length, bool extended = false);
    
    // Specific pack controller messages
    bool SendModuleCommand(uint8_t moduleId, uint8_t command, const uint8_t* params = NULL, uint8_t paramLen = 0);
    bool SendStateChange(uint8_t moduleId, uint8_t newState);
    bool SendBalancingCommand(uint8_t moduleId, uint8_t cellMask);
    bool SendRegistrationAck(uint8_t moduleId, bool accepted);
    bool SendTimeSync(uint32_t timestamp);
    bool SendWeb4KeyChunk(uint8_t moduleId, uint8_t chunkNum, const uint8_t* chunk);
    bool SendDetailRequest(uint8_t moduleId, uint8_t cellId);
    
    // Message reception
    void SetCallback(CANCallbackInterface* callback) { callbackInterface = callback; }
    void SetFilterForModules(uint32_t baseId, uint32_t mask);
    
    // Reception control
    void StartReceiving();
    void StopReceiving();
    bool IsReceiving() const { return receiving; }
    
    // Statistics and diagnostics
    struct Statistics {
        uint32_t messagesSent;
        uint32_t messagesReceived;
        uint32_t errors;
        uint32_t busLoad;  // Percentage
        bool busOff;
        bool errorPassive;
        bool errorWarning;
    };
    
    Statistics GetStatistics();
    void ResetStatistics();
    std::string GetLastError() { return lastError; }
    
    // Bus control
    bool ResetBus();
    bool SetBusParameters(uint16_t baudrate);
    
    // Logging
    void EnableLogging(const std::string& filename);
    void DisableLogging();
    
private:
    // PCAN handle and state
    TPCANHandle pcanHandle;
    bool connected;
    volatile bool receiving;
    volatile bool shouldStop;
    
    // Callback interface
    CANCallbackInterface* callbackInterface;
    
    // Reception thread
    HANDLE rxThread;
    static DWORD WINAPI ReceiveThreadProc(LPVOID param);
    void ReceiveThreadFunc();
    volatile bool stopThread;
    
    // Message queue for thread-safe handling
    std::queue<CANMessage> rxQueue;
    CRITICAL_SECTION rxCriticalSection;
    
    // Statistics
    Statistics stats;
    CRITICAL_SECTION statsCriticalSection;
    
    // Error handling
    std::string lastError;
    void SetError(const std::string& error);
    std::string GetPCANErrorText(TPCANStatus status);
    
    // Logging
    bool loggingEnabled;
    std::string logFilename;
    void LogMessage(const CANMessage& msg, bool isTx);
    
    // Helper functions
    TPCANMsg ConvertToTPCAN(const CANMessage& msg);
    CANMessage ConvertFromTPCAN(const TPCANMsg& msg, const TPCANTimestamp& timestamp);
    uint32_t BuildModuleCANId(uint8_t moduleId, uint16_t messageType);
};

// Utility functions for CAN ID manipulation
inline uint32_t SetExtendedIdBits(uint32_t baseId, uint8_t bits8to10) {
    return baseId | ((bits8to10 & 0x07) << 8);
}

inline bool IsModuleMessage(uint32_t canId) {
    // Check if CAN ID is in module message range
    uint32_t baseId = canId & 0x7FF;
    return (baseId >= ID_MODULE_ANNOUNCEMENT && baseId <= ID_MODULE_ALL_ISOLATE);
}

inline uint8_t GetModuleIdFromCAN(uint32_t canId) {
    // Extract module ID from extended bits or data
    return (canId >> 8) & 0x1F;
}

inline uint8_t GetExtendedIdBits(uint32_t canId) {
    return (canId >> 8) & 0x07;
}

} // namespace PackEmulator

#endif // CAN_INTERFACE_H