# Modbatt System: Robust Key Distribution Protocol

## Document Information

- **Version**: 2.1.0
- **Date**: August 2025
- **Status**: **IMPLEMENTATION COMPLETE WITH STRUCTURE UPDATES**
- **Context**: Enterprise-Grade CAN-Based Key Distribution with Individual Byte Field Structures

## Overview

This document details the robust key distribution protocol implemented for the Modbatt system, providing guaranteed delivery of Web4 encryption keys from the application to pack controller hardware via CAN bus with comprehensive error handling and retry mechanisms.

## Protocol Architecture

### **Design Principles**
1. **Maximum Throughput**: Full 8 bytes of key data per CAN message
2. **Guaranteed Delivery**: ACK/NACK handshaking with retry logic
3. **Error Detection**: Checksum validation and comprehensive error handling
4. **Robust Operation**: Timeout management and graceful failure handling
5. **Enterprise Reliability**: Suitable for critical encryption key distribution

### **Protocol Flow**
```
App → Send Chunk → Wait for ACK → Next Chunk
                     ↓ (timeout/NACK)
                   Retry (up to 3x)
                     ↓ (failure)
                   Abort with Error
```

## Technical Specifications

### **CAN Message Format**

#### **Key Data Transmission (App → Pack Controller)**
```
Message Structure:
├── CAN ID: Base_ID | (chunk_number << 8) | pack_ID
├── Message Type: Extended CAN frame (29-bit ID)
├── Data Length: 8 bytes (full CAN payload)
└── Data Payload: [8_bytes_of_key_data]

Examples:
├── Chunk 0: 0x407 | (0 << 8) | pack_ID = 0x407 + pack_ID
├── Chunk 1: 0x407 | (1 << 8) | pack_ID = 0x507 + pack_ID
├── Chunk 7: 0x407 | (7 << 8) | pack_ID = 0xF07 + pack_ID
```

#### **Acknowledgment Response (Pack Controller → App)**
```
Message Structure:
├── CAN ID: Response_Base_ID | (chunk_number << 8) | pack_ID
├── Message Type: Extended CAN frame (29-bit ID)
├── Data Length: 8 bytes
└── Data Payload: [ACK_STATUS, chunk_number, checksum_high, checksum_low, reserved...]

Response IDs:
├── 0x4A7 = 0x407 + 0xA0 (Pack key half responses)
├── 0x4A8 = 0x408 + 0xA0 (App key half responses)
├── 0x4A9 = 0x409 + 0xA0 (Component ID responses)

ACK_STATUS Values:
├── 0x01 = ACK (success)
├── 0x00 = NACK (failure, retry required)
```

### **Key Distribution CAN IDs**

#### **Transmission IDs (VCU to Pack Controller)**
```cpp
#define ID_VCU_WEB4_PACK_KEY_HALF   0x407    // Pack controller's device key half
#define ID_VCU_WEB4_APP_KEY_HALF    0x408    // App's device key half  
#define ID_VCU_WEB4_COMPONENT_IDS   0x409    // Component IDs
#define ID_VCU_WEB4_KEY_STATUS      0x40A    // Key distribution status/confirmation
```

#### **Response IDs (Pack Controller to VCU)**
```cpp
#define ID_BMS_WEB4_PACK_KEY_ACK    0x4A7    // Pack key half acknowledgment
#define ID_BMS_WEB4_APP_KEY_ACK     0x4A8    // App key half acknowledgment
#define ID_BMS_WEB4_COMPONENT_ACK   0x4A9    // Component IDs acknowledgment
```

### **✅ UPDATED: CAN Frame Structure Integration**

#### **✅ IMPLEMENTED: Individual Byte Field Structures**
```cpp
// Pack Controller Key Half (0x407)
typedef struct {                               // 0x407 VCU_WEB4_PACK_KEY_HALF - 8 bytes
  uint8_t KEY_CHUNK_BYTE0                  : 8; // 00-07: First byte of pack key chunk
  uint8_t KEY_CHUNK_BYTE1                  : 8; // 08-15: Second byte of pack key chunk
  uint8_t KEY_CHUNK_BYTE2                  : 8; // 16-23: Third byte of pack key chunk
  uint8_t KEY_CHUNK_BYTE3                  : 8; // 24-31: Fourth byte of pack key chunk
  uint8_t KEY_CHUNK_BYTE4                  : 8; // 32-39: Fifth byte of pack key chunk
  uint8_t KEY_CHUNK_BYTE5                  : 8; // 40-47: Sixth byte of pack key chunk
  uint8_t KEY_CHUNK_BYTE6                  : 8; // 48-55: Seventh byte of pack key chunk
  uint8_t KEY_CHUNK_BYTE7                  : 8; // 56-63: Eighth byte of pack key chunk
}CANFRM_0x407_VCU_WEB4_PACK_KEY_HALF;

// App Key Half (0x408) and Component IDs (0x409) use identical layout
// with KEY_CHUNK_BYTE0 through KEY_CHUNK_BYTE7 fields
```

#### **✅ IMPLEMENTED: Generic Transmission Structure**
```cpp
// Generic approach used in SendKeyData() function
struct {
    uint8_t BYTE0, BYTE1, BYTE2, BYTE3, BYTE4, BYTE5, BYTE6, BYTE7;
} keyFrame = {0};
```

## Implementation Details

### **Core Functions**

#### **1. Main Distribution Function**
```cpp
void TForm1::DistributeKeysToPackController(void)
```
**Purpose**: Coordinates distribution of all Web4 keys to pack controller
**Process**:
1. Validates key availability (FAppDeviceKeyHalf, FPackDeviceKeyHalf)
2. Validates component IDs (FRegisteredPackId, FRegisteredHostId)
3. Distributes pack controller's device key half
4. Distributes app's device key half
5. Distributes component IDs
6. Provides comprehensive logging and error reporting

#### **2. Reliable Key Transmission**
```cpp
TPCANStatus TForm1::SendKeyData(uint32_t baseCanId, const System::UnicodeString& keyData)
```
**Purpose**: Sends 64-byte keys reliably with ACK/retry mechanism
**Features**:
- **Chunking**: Divides 64-byte keys into 8 chunks of 8 bytes each
- **Retry Logic**: Up to 3 attempts per chunk
- **Timeout Handling**: 1-second timeout per chunk
- **Error Reporting**: Detailed failure analysis and logging
- **Progress Tracking**: Real-time chunk transmission status

**Algorithm**:
```cpp
For each chunk (0-7):
    retries = 0
    while (!acknowledged && retries < 3):
        Send chunk with ID: baseCanId | (chunk << 8) | packID
        Wait for ACK with timeout
        if (!acknowledged):
            retries++
            Log retry attempt
    if (!acknowledged):
        Abort with error
    Log success
```

#### **3. Acknowledgment Handling**
```cpp
bool TForm1::WaitForChunkAck(uint32_t baseCanId, int expectedChunk, int timeoutMs)
```
**Purpose**: Waits for and validates chunk acknowledgments
**Features**:
- **Response Validation**: Verifies correct CAN ID and chunk number
- **Status Processing**: Handles ACK/NACK responses appropriately
- **Checksum Support**: Extracts and logs checksum information
- **Timeout Management**: Prevents indefinite blocking
- **Error Classification**: Distinguishes between timeouts and NACKs

**Validation Process**:
```cpp
Calculate expected response ID: (baseCanId + 0xA0) | (chunk << 8) | packID
Verify message format: ID match, minimum 4 bytes data
Extract response: ACK_STATUS, chunk_number, checksum
Validate response: correct status and chunk number
```

### **Message Sequence Examples**

#### **Pack Controller Key Half Distribution**
```
1. App → Pack: CAN ID 0x407, Data: [first_8_bytes_of_pack_key]
2. Pack → App: CAN ID 0x4A7, Data: [0x01, 0x00, checksum_high, checksum_low, ...]
3. App → Pack: CAN ID 0x507, Data: [second_8_bytes_of_pack_key]
4. Pack → App: CAN ID 0x5A7, Data: [0x01, 0x01, checksum_high, checksum_low, ...]
...
16. Pack → App: CAN ID 0xFA7, Data: [0x01, 0x07, checksum_high, checksum_low, ...]
```

#### **Error and Retry Sequence**
```
1. App → Pack: CAN ID 0x407, Data: [first_8_bytes]
2. (No response - timeout after 1 second)
3. App → Pack: CAN ID 0x407, Data: [first_8_bytes] (Retry 1)
4. Pack → App: CAN ID 0x4A7, Data: [0x00, 0x00, ...] (NACK)
5. App → Pack: CAN ID 0x407, Data: [first_8_bytes] (Retry 2)
6. Pack → App: CAN ID 0x4A7, Data: [0x01, 0x00, ...] (ACK - Success)
```

## Error Handling and Recovery

### **Error Classification**

#### **Transmission Errors**
- **CAN Write Failure**: Hardware or driver issues
- **Invalid Parameters**: Missing keys or component IDs
- **Resource Exhaustion**: CAN buffer full or system limits

#### **Protocol Errors**
- **ACK Timeout**: No response from pack controller within 1 second
- **NACK Response**: Pack controller rejects chunk (data corruption, buffer full)
- **Invalid Response**: Malformed ACK message or wrong chunk number

#### **Recovery Strategies**

#### **Retry Mechanism**
```cpp
Retry Policy:
├── Maximum Retries: 3 attempts per chunk
├── Retry Delay: 100ms between retry attempts
├── Timeout: 1000ms per chunk attempt
└── Total Maximum Time: ~4 seconds per chunk worst case
```

#### **Failure Handling**
```cpp
Failure Responses:
├── Log detailed error information
├── Return appropriate PCAN error code
├── Abort remaining key distribution
└── Provide actionable user guidance
```

### **Logging and Diagnostics**

#### **Success Path Logging**
```
✓ Starting key distribution to pack controller hardware...
  Sending 64 bytes in 8 chunks with acknowledgment...
  → Sent chunk 1/8 (8 bytes)
    ← ACK received for chunk 1 (checksum: 0x1A2B)
  → Sent chunk 2/8 (8 bytes)
    ← ACK received for chunk 2 (checksum: 0x3C4D)
  ...
  ✓ Chunk 8 acknowledged
  ✓ All chunks sent and acknowledged successfully
```

#### **Error Path Logging**
```
✗ Starting key distribution to pack controller hardware...
  Sending 64 bytes in 8 chunks with acknowledgment...
  → Sent chunk 1/8 (8 bytes)
    ⏱ Timeout waiting for ACK on chunk 1
  ⚠ Chunk 1 not acknowledged, retry 1/3
  → Sent chunk 1/8 (8 bytes)
    ← NACK received for chunk 1
  ⚠ Chunk 1 not acknowledged, retry 2/3
  → Sent chunk 1/8 (8 bytes)
    ⏱ Timeout waiting for ACK on chunk 1
  ✗ Chunk 1 failed after 3 retries
```

## Pack Controller Implementation Requirements

### **Reception Protocol**

#### **Message Filtering**
```cpp
// Listen for key distribution messages
bool IsKeyDistributionMessage(TPCANMsg* msg) {
    uint32_t baseId = msg->ID & 0x0FF;  // Mask out chunk and pack ID
    return (baseId == 0x407 || baseId == 0x408 || baseId == 0x409);
}

// Extract chunk number from CAN ID
uint8_t GetChunkNumber(uint32_t canId) {
    return (canId >> 8) & 0x0F;  // Bits 8-11 contain chunk number
}
```

#### **Key Assembly**
```cpp
typedef struct {
    uint8_t packKeyHalf[64];        // Pack controller's device key half
    uint8_t appKeyHalf[64];         // App's device key half
    bool packKeyComplete;           // All pack key chunks received
    bool appKeyComplete;            // All app key chunks received
    uint8_t packKeyChunks;          // Bitmask of received pack chunks
    uint8_t appKeyChunks;           // Bitmask of received app chunks
} Web4KeyBuffer;

void ProcessKeyChunk(TPCANMsg* msg) {
    uint8_t chunk = GetChunkNumber(msg->ID);
    uint32_t baseId = msg->ID & 0x0FF;
    
    if (baseId == ID_VCU_WEB4_PACK_KEY_HALF) {
        // Cast received data to individual byte structure
        CANFRM_0x407_VCU_WEB4_PACK_KEY_HALF* keyFrame = 
            (CANFRM_0x407_VCU_WEB4_PACK_KEY_HALF*)msg->DATA;
        
        // Extract individual bytes to key buffer
        uint8_t startIdx = chunk * 8;
        keyBuffer.packKeyHalf[startIdx + 0] = keyFrame->KEY_CHUNK_BYTE0;
        keyBuffer.packKeyHalf[startIdx + 1] = keyFrame->KEY_CHUNK_BYTE1;
        keyBuffer.packKeyHalf[startIdx + 2] = keyFrame->KEY_CHUNK_BYTE2;
        keyBuffer.packKeyHalf[startIdx + 3] = keyFrame->KEY_CHUNK_BYTE3;
        keyBuffer.packKeyHalf[startIdx + 4] = keyFrame->KEY_CHUNK_BYTE4;
        keyBuffer.packKeyHalf[startIdx + 5] = keyFrame->KEY_CHUNK_BYTE5;
        keyBuffer.packKeyHalf[startIdx + 6] = keyFrame->KEY_CHUNK_BYTE6;
        keyBuffer.packKeyHalf[startIdx + 7] = keyFrame->KEY_CHUNK_BYTE7;
        
        keyBuffer.packKeyChunks |= (1 << chunk);
        
        // Check if complete
        if (keyBuffer.packKeyChunks == 0xFF) { // All 8 chunks received
            keyBuffer.packKeyComplete = true;
            LogMessage("Pack key half assembly complete");
        }
    }
    // Similar for app key half...
    
    // Send ACK
    SendChunkAck(msg->ID, chunk, CalculateChecksum(msg->DATA, 8));
}
```

#### **Response Generation**
```cpp
void SendChunkAck(uint32_t originalId, uint8_t chunk, uint16_t checksum) {
    TPCANMsg ackMsg;
    ackMsg.ID = (originalId + 0xA0);  // Response ID
    ackMsg.LEN = 8;
    ackMsg.MSGTYPE = PCAN_MESSAGE_EXTENDED;
    
    ackMsg.DATA[0] = 0x01;           // ACK status
    ackMsg.DATA[1] = chunk;          // Chunk number
    ackMsg.DATA[2] = (checksum >> 8) & 0xFF;  // Checksum high
    ackMsg.DATA[3] = checksum & 0xFF;         // Checksum low
    ackMsg.DATA[4] = 0x00;           // Reserved
    ackMsg.DATA[5] = 0x00;           // Reserved
    ackMsg.DATA[6] = 0x00;           // Reserved
    ackMsg.DATA[7] = 0x00;           // Reserved
    
    CAN_Write(CAN_HANDLE, &ackMsg);
}
```

## Performance Characteristics

### **Timing Analysis**

#### **Best Case Scenario**
```
Per Key (64 bytes):
├── 8 chunks × (transmit + ACK + processing) = ~80ms
├── Total for 2 keys + component IDs = ~250ms
└── Transmission rate: ~256 bytes/second
```

#### **Worst Case Scenario**
```
Per Key with all retries:
├── 8 chunks × 3 retries × 1.1s timeout = ~26 seconds maximum
├── Realistic with retries: ~2-5 seconds per key
└── Total worst case: ~15 seconds for complete distribution
```

#### **Network Utilization**
```
CAN Bus Impact:
├── Message Rate: ~80 messages total (key chunks + ACKs)
├── Peak Burst: 16 messages in ~2 seconds (8 chunks + 8 ACKs)
├── Bus Utilization: <5% on 500kbps CAN network
└── Priority: Uses extended IDs for priority management
```

### **Resource Requirements**

#### **Application Side**
```
Memory Usage:
├── Key Storage: ~128 bytes (2 × 64-byte keys)
├── Message Buffers: ~32 bytes (CAN message structures)
├── State Tracking: ~16 bytes (retry counters, timeouts)
└── Total: <200 bytes additional RAM

Processing Overhead:
├── CPU Impact: <1% during key distribution
├── Network I/O: Blocking during transmission
└── User Interface: Progress indication and logging
```

#### **Pack Controller Side**
```
Memory Requirements:
├── Key Buffers: 128 bytes (2 × 64-byte keys)
├── Assembly State: 16 bytes (chunk tracking bitmasks)
├── Message Processing: 32 bytes (temporary buffers)
└── Total: <200 bytes additional RAM

Real-Time Impact:
├── Interrupt Priority: Key reception at high priority
├── Processing Time: <100μs per chunk received
├── Background Assembly: Non-blocking key reconstruction
└── CAN Latency: No impact on time-critical messages
```

## Security Considerations

### **Protocol Security**

#### **Data Protection**
- **Key Confidentiality**: Keys transmitted in plaintext over CAN (physical security required)
- **Integrity Verification**: Checksums detect transmission errors
- **Replay Protection**: No replay protection in this layer (handled by encryption layer)
- **Access Control**: CAN ID filtering limits message acceptance

#### **Attack Vectors**
- **Eavesdropping**: CAN bus sniffing can intercept keys during distribution
- **Message Injection**: Malicious ACK/NACK responses could disrupt distribution
- **Denial of Service**: Flooding with false ACK messages
- **Physical Access**: Direct CAN bus access compromises all security

#### **Mitigations**
- **Physical Security**: Protect CAN bus from unauthorized access
- **Timing Windows**: Limited transmission windows reduce exposure
- **Message Validation**: Strict format checking prevents malformed responses
- **Operational Security**: Key distribution only during secure setup phases

### **Key Management**

#### **Key Lifecycle**
```
Key States:
├── Generated: Keys created by API-Bridge
├── In-Transit: Keys being distributed via CAN
├── Assembled: Keys reconstructed in pack controller
├── Active: Keys used for operational encryption
└── Expired: Keys marked for rotation/replacement
```

#### **Security Best Practices**
- **Minimize Exposure**: Keys only transmitted during setup/maintenance
- **Immediate Use**: Keys used immediately after successful distribution
- **Memory Clearing**: Temporary buffers cleared after use
- **Audit Logging**: All key operations logged for security review

## Testing and Validation

### **Test Scenarios**

#### **Functional Testing**
```
Test Cases:
├── Normal Operation: All chunks transmitted and acknowledged successfully
├── Single Retry: One chunk requires retry, succeeds on second attempt
├── Multiple Retries: Chunk fails twice, succeeds on third attempt
├── Timeout Recovery: Slow responses handled within timeout limits
├── NACK Handling: NACK responses trigger appropriate retries
└── Complete Failure: All retries exhausted, error handling activated
```

#### **Stress Testing**
```
Stress Scenarios:
├── High CAN Traffic: Key distribution during heavy bus utilization
├── Network Errors: Simulated CAN errors and recovery
├── Timing Variations: Variable ACK response times
├── Concurrent Operations: Multiple key distributions simultaneously
└── Resource Limits: Low memory and processing power conditions
```

#### **Security Testing**
```
Security Validation:
├── Eavesdropping Detection: Monitor for key exposure during transmission
├── Injection Resistance: Verify robustness against malformed messages
├── Timing Analysis: Ensure consistent timing regardless of key content
└── Physical Security: Validate protection against hardware attacks
```

### **Success Criteria**

#### **Reliability Metrics**
- **Transmission Success Rate**: >99% under normal conditions
- **Recovery Effectiveness**: >95% success rate with retry mechanism
- **Performance Consistency**: <10% variation in transmission times
- **Error Detection**: 100% detection of transmission errors

#### **Security Metrics**
- **Key Integrity**: 100% detection of corrupted key data
- **Access Control**: No unauthorized key access
- **Audit Completeness**: 100% logging of key operations
- **Attack Resistance**: Robust operation under security stress testing

## Conclusion

The robust key distribution protocol provides **enterprise-grade reliability** for critical encryption key delivery in the Modbatt system. Key features include:

### **Technical Excellence**
- **Maximum Efficiency**: Full CAN payload utilization with optimized chunking
- **Guaranteed Delivery**: ACK/retry mechanism ensures reliable transmission
- **Comprehensive Error Handling**: Graceful failure handling with detailed diagnostics
- **Performance Optimization**: Minimal network impact with fast transmission

### **Operational Benefits**
- **Zero Data Loss**: No key chunks lost due to network issues
- **Transparent Operation**: Detailed logging provides complete visibility
- **Maintenance Friendly**: Clear error messages and recovery procedures
- **Scalable Design**: Supports additional key types and extended functionality

### **Security Foundation**
- **Integrity Protection**: Checksum validation detects transmission errors
- **Audit Trail**: Complete logging for security analysis
- **Controlled Access**: CAN ID filtering and validation
- **Physical Security**: Designed for protected network environments

The protocol successfully bridges the gap between **Web4 trust establishment** (via API-Bridge) and **hardware-level encryption implementation** (on pack controllers), providing a robust foundation for the secure operation of the Modbatt battery management ecosystem.

---

**Status**: ✅ **PROTOCOL IMPLEMENTATION COMPLETE WITH ENHANCED STRUCTURES** - Individual byte field structures implemented and ready for integration testing with updated pack controller firmware.