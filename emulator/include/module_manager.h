/******************************************************************************
 * @file    module_manager.h
 * @brief   Battery module management for Pack Controller Emulator
 * @author  Pack Emulator Development Team  
 * @date    December 2024
 * 
 * Copyright (C) 2025 Modular Battery Technologies, Inc.
 ******************************************************************************/

#ifndef MODULE_MANAGER_H
#define MODULE_MANAGER_H

#include <vector>
#include <map>
#include <string>
#include <cstdint>
#include <windows.h>  // For GetTickCount()

// Include battery structures from embedded code
extern "C" {
    #include <stdint.h>
    #include <stdbool.h>
    #include "../../Core/Inc/bms.h"              // Module states and battery structures
    #include "../../Core/Inc/can_id_module.h"    // Module CAN message IDs
    #include "../../Core/Inc/can_id_bms_vcu.h"   // VCU CAN message IDs  
    #include "../../Core/Inc/can_frm_mod.h"      // Module CAN frame structures
}

namespace PackEmulator {

// Module states - using values from bms.h moduleState enum
enum class ModuleState {
    OFF = moduleOff,           // 0
    STANDBY = moduleStandby,   // 1  
    PRECHARGE = modulePrecharge, // 2
    ON = moduleOn,             // 3
    UNKNOWN = 255
};

// Module information structure
struct ModuleInfo {
    uint8_t moduleId;
    uint32_t uniqueId;
    ModuleState state;
    bool isRegistered;
    bool isResponding;
    bool statusPending;
    
    // Electrical data
    float voltage;          // Module voltage in V
    float current;          // Module current in A  
    float temperature;      // Average temperature in C
    float soc;             // State of charge in %
    float soh;             // State of health in %
    
    // Cell statistics from STATUS_2
    float minCellVoltage;   // Minimum cell voltage in V
    float maxCellVoltage;   // Maximum cell voltage in V
    float avgCellVoltage;   // Average cell voltage in V
    float totalCellVoltage; // Total of all cell voltages in V
    
    // Temperature statistics from STATUS_3
    float minCellTemp;      // Minimum cell temperature in °C
    float maxCellTemp;      // Maximum cell temperature in °C
    float avgCellTemp;      // Average cell temperature in °C
    
    // Hardware capabilities from HARDWARE message
    float maxChargeCurrent;    // Maximum charge current in A
    float maxDischargeCurrent; // Maximum discharge current in A
    float maxChargeVoltage;    // Maximum charge voltage in V
    uint16_t hardwareVersion;  // Hardware version
    
    // Cell data
    std::vector<float> cellVoltages;
    std::vector<float> cellTemperatures;
    
    // Timing (using Windows GetTickCount - milliseconds since boot)
    DWORD lastMessageTime;
    uint32_t messageCount;
    uint32_t errorCount;
    
    // Web4 data
    bool hasWeb4Keys;
    uint8_t web4DeviceKeyHalf[64];
    uint8_t web4LctKeyHalf[64];
    std::string web4ComponentId;
};

class ModuleManager {
public:
    ModuleManager();
    ~ModuleManager();
    
    // Module discovery and registration
    void StartDiscovery();
    void StopDiscovery();
    bool IsDiscoveryActive() const { return discoveryActive; }
    bool RegisterModule(uint8_t moduleId, uint32_t uniqueId);
    bool DeregisterModule(uint8_t moduleId);
    void DeregisterAllModules();
    
    // Module control
    bool SetModuleState(uint8_t moduleId, ModuleState state);
    bool SetAllModulesState(ModuleState state);
    bool IsolateModule(uint8_t moduleId);
    bool EnableBalancing(uint8_t moduleId, uint8_t cellMask);
    
    // Data updates from CAN messages
    void UpdateModuleStatus(uint8_t moduleId, const uint8_t* data);
    void UpdateCellVoltages(uint8_t moduleId, uint8_t startCell, const uint16_t* voltages, uint8_t count);
    void UpdateCellTemperatures(uint8_t moduleId, uint8_t startCell, const uint16_t* temps, uint8_t count);
    void UpdateModuleElectrical(uint8_t moduleId, float voltage, float current, float temp);
    
    // Module queries
    ModuleInfo* GetModule(uint8_t moduleId);
    ModuleInfo GetModuleInfo(uint8_t moduleId);
    std::vector<ModuleInfo*> GetAllModules();
    std::vector<uint8_t> GetRegisteredModuleIds();
    int GetModuleCount() const { return modules.size(); }
    bool IsModuleRegistered(uint8_t moduleId);
    bool IsModuleResponding(uint8_t moduleId);
    void SetStatusPending(uint8_t moduleId, bool pending);
    
    // Pack calculations
    float GetPackVoltage();
    float GetPackCurrent();
    float GetPackSoc();
    float GetMinCellVoltage();
    float GetMaxCellVoltage();
    float GetAverageTemperature();
    
    // Fault detection
    bool CheckForFaults();
    std::vector<std::string> GetActiveFaults();
    void ClearFaults();
    
    // Web4 key management
    bool DistributeWeb4Keys(uint8_t moduleId, const uint8_t* deviceKey, const uint8_t* lctKey);
    bool StoreWeb4ComponentId(uint8_t moduleId, const std::string& componentId);
    
    // Timeouts and health monitoring  
    void CheckTimeouts();
    void UpdateStatistics();
    
    // Configuration
    void SetTimeout(uint32_t timeoutMs) { moduleTimeoutMs = timeoutMs; }
    void SetMaxModules(uint8_t max) { maxModules = max; }
    
private:
    std::map<uint8_t, ModuleInfo> modules;
    bool discoveryActive;
    uint8_t minDiscoveryId;
    
    // Configuration
    uint32_t moduleTimeoutMs;
    uint8_t maxModules;
    
    // Statistics
    uint32_t totalMessages;
    uint32_t totalErrors;
    DWORD startTime;  // Using Windows GetTickCount
    
    // Helper functions
    bool ValidateModuleId(uint8_t moduleId);
    bool ValidateCellData(uint8_t moduleId, uint8_t cellIndex);
    void CalculatePackValues();
    void DetectFaults(ModuleInfo& module);
};

} // namespace PackEmulator

#endif // MODULE_MANAGER_H