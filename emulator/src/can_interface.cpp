/******************************************************************************
 * @file    can_interface.cpp
 * @brief   PCAN-Basic wrapper implementation
 * @author  Pack Emulator Development Team
 * @date    December 2024
 * 
 * Copyright (C) 2025 Modular Battery Technologies, Inc.
 ******************************************************************************/

#include "../include/can_interface.h"
#include "../include/PCANBasicClass.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cstring>

// Global PCAN-Basic API instance
static PCANBasicClass* g_pcanAPI = NULL;

namespace PackEmulator {

// Static thread procedure that calls the member function
DWORD WINAPI CANInterface::ReceiveThreadProc(LPVOID param) {
    CANInterface* pThis = static_cast<CANInterface*>(param);
    pThis->ReceiveThreadFunc();
    return 0;
}

CANInterface::CANInterface()
    : pcanHandle(PCAN_NONEBUS)
    , connected(false)
    , receiving(false)
    , shouldStop(false)
    , callbackInterface(NULL)
    , loggingEnabled(false) {
    
    // Initialize PCAN API if not already done
    if (g_pcanAPI == NULL) {
        g_pcanAPI = new PCANBasicClass();
    }
    
    // Initialize statistics
    std::memset(&stats, 0, sizeof(stats));
    
    // Initialize critical sections
    InitializeCriticalSection(&statsCriticalSection);
    InitializeCriticalSection(&rxCriticalSection);
}

CANInterface::~CANInterface() {
    Disconnect();
    
    // Delete critical sections
    DeleteCriticalSection(&statsCriticalSection);
    DeleteCriticalSection(&rxCriticalSection);
}

bool CANInterface::Connect(uint16_t channel, uint16_t baudrate) {
    if (connected) {
        Disconnect();
    }
    
    // Initialize PCAN channel
    if (g_pcanAPI == NULL) {
        SetError("PCAN-Basic API not initialized");
        return false;
    }
    
    TPCANStatus status = g_pcanAPI->Initialize(channel, baudrate, 0, 0, 0);
    
    if (status != PCAN_ERROR_OK) {
        SetError("Failed to initialize CAN: " + GetPCANErrorText(status));
        return false;
    }
    
    pcanHandle = channel;
    connected = true;
    
    // Reset CAN controller
    g_pcanAPI->Reset(pcanHandle);
    
    return true;
}

void CANInterface::Disconnect() {
    if (!connected) {
        return;
    }
    
    StopReceiving();
    
    if (pcanHandle != PCAN_NONEBUS && g_pcanAPI != NULL) {
        g_pcanAPI->Uninitialize(pcanHandle);
        pcanHandle = PCAN_NONEBUS;
    }
    
    connected = false;
}

bool CANInterface::SendMessage(const CANMessage& msg) {
    if (!connected) {
        SetError("Not connected");
        return false;
    }
    
    TPCANMsg pcanMsg = ConvertToTPCAN(msg);
    TPCANStatus status = g_pcanAPI->Write(pcanHandle, &pcanMsg);
    
    if (status != PCAN_ERROR_OK) {
        SetError("Send failed: " + GetPCANErrorText(status));
        stats.errors++;
        return false;
    }
    
    stats.messagesSent++;
    
    if (loggingEnabled) {
        LogMessage(msg, true);
    }
    
    return true;
}

bool CANInterface::SendMessage(uint32_t id, const uint8_t* data, uint8_t length, bool extended) {
    CANMessage msg;
    msg.id = id;
    msg.length = length;
    msg.isExtended = extended;
    msg.isRtr = false;
    
    if (data && length > 0) {
        std::memcpy(msg.data, data, std::min(length, uint8_t(8)));
    }
    
    return SendMessage(msg);
}

bool CANInterface::SendModuleCommand(uint8_t moduleId, uint8_t command, const uint8_t* params, uint8_t paramLen) {
    // Note: BuildModuleCANId was designed for a different format
    // For extended frames matching Pack Controller format:
    // - Bits 28-18: Message type (0x200 range for commands) 
    // - Bits 17-0: Module ID
    // This is a broadcast command, so we don't use BuildModuleCANId
    
    // For now, these are likely not the right IDs - need to check Pack Controller
    // The 0x200 range might not be correct for module commands
    uint32_t extendedId = ((uint32_t)0x514 << 18) | moduleId;  // Using STATE_CHANGE as example
    
    uint8_t data[8] = {0};
    data[0] = command;
    
    if (params && paramLen > 0) {
        std::memcpy(&data[1], params, std::min(paramLen, uint8_t(7)));
    }
    
    return SendMessage(extendedId, data, 1 + paramLen, true);
}

bool CANInterface::SendStateChange(uint8_t moduleId, uint8_t newState) {
    return SendModuleCommand(moduleId, 0x01, &newState, 1); // Command 0x01 = State change
}

bool CANInterface::SendBalancingCommand(uint8_t moduleId, uint8_t cellMask) {
    return SendModuleCommand(moduleId, 0x02, &cellMask, 1); // Command 0x02 = Balancing
}

bool CANInterface::SendRegistrationAck(uint8_t moduleId, bool accepted) {
    uint8_t ack = accepted ? 0x01 : 0x00;
    return SendModuleCommand(moduleId, 0x10, &ack, 1); // Command 0x10 = Registration ACK
}

bool CANInterface::SendTimeSync(uint32_t timestamp) {
    uint8_t data[8];
    data[0] = 0xFF; // Broadcast time sync
    data[1] = (timestamp >> 24) & 0xFF;
    data[2] = (timestamp >> 16) & 0xFF;
    data[3] = (timestamp >> 8) & 0xFF;
    data[4] = timestamp & 0xFF;
    
    // For extended frames, the 11-bit message ID goes in bits 28-18
    // This appears to be a non-standard ID (0x2FF), keeping as-is for now
    uint32_t extendedId = ((uint32_t)0x2FF << 18) | 0;
    return SendMessage(extendedId, data, 5, true); // Broadcast ID
}

bool CANInterface::SendWeb4KeyChunk(uint8_t moduleId, uint8_t chunkNum, const uint8_t* chunk) {
    uint32_t canId = BuildModuleCANId(moduleId, 0x260); // Web4 base ID
    canId = SetExtendedIdBits(canId, chunkNum); // Add chunk number to extended bits
    
    return SendMessage(canId, chunk, 8, true); // Already using extended ID for chunking
}

void CANInterface::SetFilterForModules(uint32_t baseId, uint32_t mask) {
    if (!connected) {
        return;
    }
    
    // Set PCAN filter
    uint64_t filterValue = ((uint64_t)mask << 32) | baseId;
    g_pcanAPI->SetValue(pcanHandle, PCAN_MESSAGE_FILTER, &filterValue, sizeof(filterValue));
}

void CANInterface::StartReceiving() {
    if (!connected || receiving) {
        return;
    }
    
    shouldStop = false;
    receiving = true;
    
    // Start receive thread
    stopThread = false;
    rxThread = CreateThread(NULL, 0, ReceiveThreadProc, this, 0, NULL);
}

void CANInterface::StopReceiving() {
    if (!receiving) {
        return;
    }
    
    shouldStop = true;
    stopThread = true;
    receiving = false;
    
    if (rxThread != NULL) {
        // Wait for thread to exit
        WaitForSingleObject(rxThread, 5000);  // 5 second timeout
        CloseHandle(rxThread);
        rxThread = NULL;
    }
}

void CANInterface::ReceiveThreadFunc() {
    TPCANMsg pcanMsg;
    TPCANTimestamp timestamp;
    
    while (!shouldStop && !stopThread) {
        TPCANStatus status = g_pcanAPI->Read(pcanHandle, &pcanMsg, &timestamp);
        
        if (status == PCAN_ERROR_OK) {
            CANMessage msg = ConvertFromTPCAN(pcanMsg, timestamp);
            
            stats.messagesReceived++;
            
            if (loggingEnabled) {
                LogMessage(msg, false);
            }
            
            // Call callback if set
            if (callbackInterface) {
                callbackInterface->OnMessage(msg);
            }
        } else if (status != PCAN_ERROR_QRCVEMPTY) {
            // Error occurred
            stats.errors++;
            if (callbackInterface) {
                callbackInterface->OnError(status, GetPCANErrorText(status));
            }
        }
        
        // Small delay to prevent CPU spinning
        Sleep(1);  // 1 millisecond delay
    }
}

CANInterface::Statistics CANInterface::GetStatistics() {
    EnterCriticalSection(&statsCriticalSection);
    
    // Get bus status
    if (connected && g_pcanAPI != NULL) {
        TPCANStatus status = g_pcanAPI->GetStatus(pcanHandle);
        
        stats.busOff = (status & PCAN_ERROR_BUSOFF) != 0;
        stats.errorPassive = (status & PCAN_ERROR_BUSPASSIVE) != 0;
        stats.errorWarning = (status & PCAN_ERROR_BUSWARNING) != 0;
    }
    
    Statistics result = stats;
    LeaveCriticalSection(&statsCriticalSection);
    return result;
}

void CANInterface::ResetStatistics() {
    EnterCriticalSection(&statsCriticalSection);
    stats.messagesSent = 0;
    stats.messagesReceived = 0;
    stats.errors = 0;
    LeaveCriticalSection(&statsCriticalSection);
}

bool CANInterface::ResetBus() {
    if (!connected) {
        return false;
    }
    
    return (g_pcanAPI != NULL) ? (g_pcanAPI->Reset(pcanHandle) == PCAN_ERROR_OK) : false;
}

bool CANInterface::SetBusParameters(uint16_t baudrate) {
    if (!connected) {
        return false;
    }
    
    // Disconnect and reconnect with new parameters
    TPCANHandle handle = pcanHandle;
    Disconnect();
    return Connect(handle, baudrate);
}

void CANInterface::EnableLogging(const std::string& filename) {
    logFilename = filename;
    loggingEnabled = true;
}

void CANInterface::DisableLogging() {
    loggingEnabled = false;
    logFilename.clear();
}

void CANInterface::SetError(const std::string& error) {
    lastError = error;
}

std::string CANInterface::GetPCANErrorText(TPCANStatus status) {
    char buffer[256];
    if (g_pcanAPI != NULL) {
        g_pcanAPI->GetErrorText(status, 0, buffer);
    } else {
        sprintf(buffer, "Error code: 0x%X", status);
    }
    return std::string(buffer);
}

TPCANMsg CANInterface::ConvertToTPCAN(const CANMessage& msg) {
    TPCANMsg pcanMsg;
    
    pcanMsg.ID = msg.id;
    pcanMsg.LEN = msg.length;
    
    pcanMsg.MSGTYPE = PCAN_MESSAGE_STANDARD;
    if (msg.isExtended) {
        pcanMsg.MSGTYPE = PCAN_MESSAGE_EXTENDED;
    }
    if (msg.isRtr) {
        pcanMsg.MSGTYPE |= PCAN_MESSAGE_RTR;
    }
    
    std::memcpy(pcanMsg.DATA, msg.data, msg.length);
    
    return pcanMsg;
}

CANMessage CANInterface::ConvertFromTPCAN(const TPCANMsg& pcanMsg, const TPCANTimestamp& timestamp) {
    CANMessage msg;
    
    msg.id = pcanMsg.ID;
    msg.length = pcanMsg.LEN;
    msg.isExtended = (pcanMsg.MSGTYPE & PCAN_MESSAGE_EXTENDED) != 0;
    msg.isRtr = (pcanMsg.MSGTYPE & PCAN_MESSAGE_RTR) != 0;
    
    std::memcpy(msg.data, pcanMsg.DATA, pcanMsg.LEN);
    
    // Convert timestamp
    msg.timestamp = ((uint64_t)timestamp.millis * 1000) + timestamp.micros;
    
    return msg;
}

uint32_t CANInterface::BuildModuleCANId(uint8_t moduleId, uint16_t messageType) {
    return messageType | (moduleId & 0x1F);
}

void CANInterface::LogMessage(const CANMessage& msg, bool isTx) {
    if (!loggingEnabled || logFilename.empty()) {
        return;
    }
    
    std::ofstream logFile(logFilename.c_str(), std::ios::app);
    if (!logFile.is_open()) {
        return;
    }
    
    // Format: Timestamp, Direction, ID, Length, Data
    logFile << msg.timestamp << ","
            << (isTx ? "TX" : "RX") << ","
            << std::hex << std::setfill('0') << std::setw(3) << msg.id << ","
            << std::dec << (int)msg.length << ",";
    
    for (int i = 0; i < msg.length; i++) {
        logFile << std::hex << std::setfill('0') << std::setw(2) 
                << (int)msg.data[i];
        if (i < msg.length - 1) {
            logFile << " ";
        }
    }
    
    logFile << std::endl;
    logFile.close();
}

} // namespace PackEmulator
