// Console test application for Pack Controller WEB4 functionality
// This can be built with free toolchains (MinGW-w64, MSVC Community, g++)

#include <iostream>
#include <iomanip>
#include <vector>
#include <cstring>
#include <thread>
#include <chrono>

// Include the WEB4 handler
extern "C" {
    #include "web4_handler.h"
    #include "can_id_bms_vcu.h"
}

// Mock HAL_GetTick for testing
extern "C" uint32_t HAL_GetTick() {
    static auto start = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
}

// Mock debug output
extern "C" void ShowDebugMessage(uint16_t messageId, ...) {
    std::cout << "[DEBUG] Message ID: 0x" << std::hex << messageId << std::dec << std::endl;
}

class WEB4Tester {
public:
    WEB4Tester() {
        std::cout << "=== Pack Controller WEB4 Console Test ===" << std::endl;
        std::cout << "Testing WEB4 key distribution without GUI" << std::endl << std::endl;
    }

    void run() {
        // Initialize WEB4 handler
        std::cout << "1. Initializing WEB4 handler..." << std::endl;
        WEB4_Init();
        
        // Test receiving pack device key
        std::cout << "\n2. Testing Pack Device Key Reception..." << std::endl;
        testKeyReception(ID_VCU_WEB4_PACK_KEY_HALF, "PACK");
        
        // Test receiving app device key
        std::cout << "\n3. Testing App Device Key Reception..." << std::endl;
        testKeyReception(ID_VCU_WEB4_APP_KEY_HALF, "APP");
        
        // Test receiving component IDs
        std::cout << "\n4. Testing Component ID Reception..." << std::endl;
        testKeyReception(ID_VCU_WEB4_COMPONENT_IDS, "COMPONENT");
        
        // Check final status
        std::cout << "\n5. Checking Key Validity..." << std::endl;
        if (WEB4_KeysValid()) {
            std::cout << "✓ All keys successfully received and validated!" << std::endl;
        } else {
            std::cout << "✗ Keys not fully validated" << std::endl;
        }
        
        WEB4_PrintKeyStatus();
    }

private:
    void testKeyReception(uint16_t baseCanId, const std::string& keyName) {
        std::cout << "Sending " << keyName << " key in 8 chunks..." << std::endl;
        
        // Create a test 64-byte key
        uint8_t testKey[64];
        for (int i = 0; i < 64; i++) {
            testKey[i] = (uint8_t)(i + baseCanId);  // Simple pattern for testing
        }
        
        // Calculate checksum (XOR of first 63 bytes)
        uint8_t checksum = 0;
        for (int i = 0; i < 63; i++) {
            checksum ^= testKey[i];
        }
        testKey[63] = checksum;
        
        // Send in 8-byte chunks
        for (int chunk = 0; chunk < 8; chunk++) {
            uint32_t canId = baseCanId | (chunk << 8);  // Add chunk number to bits 8-10
            uint8_t* chunkData = &testKey[chunk * 8];
            
            std::cout << "  Chunk " << chunk << " - CAN ID: 0x" << std::hex << canId;
            std::cout << " Data: ";
            for (int i = 0; i < 8; i++) {
                std::cout << std::setw(2) << std::setfill('0') << (int)chunkData[i] << " ";
            }
            std::cout << std::dec << std::endl;
            
            // Send to handler
            bool handled = WEB4_HandleCANMessage(canId, chunkData, 8);
            if (!handled) {
                std::cout << "  ✗ Message not handled!" << std::endl;
            }
            
            // Small delay between chunks
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
        std::cout << "  ✓ All chunks sent for " << keyName << " key" << std::endl;
    }
};

int main() {
    try {
        WEB4Tester tester;
        tester.run();
        
        std::cout << "\nPress Enter to exit..." << std::endl;
        std::cin.get();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}