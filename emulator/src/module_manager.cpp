/******************************************************************************
 * @file    module_manager.cpp
 * @brief   Battery module management implementation
 * @author  Pack Emulator Development Team
 * @date    December 2024
 * 
 * Copyright (C) 2025 Modular Battery Technologies, Inc.
 ******************************************************************************/

#include "../include/module_manager.h"
#include <algorithm>
#include <numeric>
#include <windows.h>
#include <sstream>
#include <stdio.h>
#include <cstring>

namespace PackEmulator {

ModuleManager::ModuleManager() 
    : discoveryActive(false)
    , minDiscoveryId(1)
    , moduleTimeoutMs(5000)
    , maxModules(32)
    , totalMessages(0)
    , totalErrors(0) {
    startTime = GetTickCount();
    
    // Pre-initialize all 32 slots with uniqueId = 0 (indicates available)
    for (uint8_t id = 1; id <= 32; id++) {
        ModuleInfo module;
        memset(&module, 0, sizeof(module));
        module.moduleId = id;
        module.uniqueId = 0;  // 0 = slot available
        module.isRegistered = false;
        module.isResponding = false;
        modules[id] = module;
    }
}

ModuleManager::~ModuleManager() {
    StopDiscovery();
}

void ModuleManager::StartDiscovery() {
    discoveryActive = true;
    minDiscoveryId = 1;
    
    // Clear any existing unregistered modules
    for (std::map<uint8_t, ModuleInfo>::iterator it = modules.begin(); it != modules.end(); ) {
        if (!it->second.isRegistered) {
            it = modules.erase(it);
        } else {
            ++it;
        }
    }
}

void ModuleManager::StopDiscovery() {
    discoveryActive = false;
}

bool ModuleManager::RegisterModule(uint8_t moduleId, uint32_t uniqueId) {
    if (!ValidateModuleId(moduleId)) {
        return false;
    }
    
    // Check if this module ID already exists (possibly deregistered)
    std::map<uint8_t, ModuleInfo>::iterator it = modules.find(moduleId);
    if (it != modules.end()) {
        // Module ID exists - check if it's deregistered
        if (!it->second.isRegistered) {
            // Reuse the existing slot but update with new registration info
            it->second.uniqueId = uniqueId;
            it->second.state = ModuleState::OFF;
            it->second.commandedState = ModuleState::OFF;
            it->second.isRegistered = true;
            it->second.isResponding = true;
            it->second.statusPending = false;
            it->second.lastResponseTime = GetTickCount();
            it->second.statusRequestTime = 0;
            it->second.lastMessageTime = GetTickCount();
            it->second.messageCount = 0;
            it->second.errorCount = 0;
            
            // Reset waiting flags
            it->second.waitingForStatusResponse = false;
            it->second.waitingForCellResponse = false;
            it->second.statusRequestTime = 0;
            it->second.cellRequestTime = 0;

            // Reset cells received to 0 - will be updated from MODULE_DETAIL
            it->second.cellsReceived = 0;

            // Don't clear other electrical/cell data - keep last known values
            // This allows UI to show historical data even after re-registration
            
            return true;
        } else {
            // Module is already registered - this shouldn't happen
            // but update the unique ID in case it changed
            it->second.uniqueId = uniqueId;
            it->second.lastResponseTime = GetTickCount();
            return true;
        }
    }
    
    // New module ID - check if we have room
    if (modules.size() >= (size_t)maxModules) {
        return false;
    }
    
    // Create new module entry
    ModuleInfo module;
    module.moduleId = moduleId;
    module.uniqueId = uniqueId;
    module.state = ModuleState::OFF;
    module.commandedState = ModuleState::OFF;
    module.isRegistered = true;
    module.isResponding = true;
    module.statusPending = false;
    module.lastResponseTime = GetTickCount();
    module.statusRequestTime = 0;
    module.hasWeb4Keys = false;
    module.lastMessageTime = GetTickCount();
    module.messageCount = 0;
    module.errorCount = 0;
    
    // Initialize waiting flags
    module.waitingForStatusResponse = false;
    module.waitingForCellResponse = false;
    module.statusRequestTime = 0;
    module.cellRequestTime = 0;
    
    // Initialize electrical data
    module.voltage = 0.0f;
    module.current = 0.0f;
    module.temperature = 25.0f;
    module.soc = 0.0f;
    module.soh = 100.0f;
    
    // Initialize cell count data
    module.cellCount = 0;      // Will be updated from STATUS_1
    module.cellCountMin = 255; // Start at max (0xFF) - will decrease as cells report
    module.cellCountMax = 0;   // Start at 0 - will increase as cells report
    module.cellsReceived = 0;  // Will be updated from MODULE_DETAIL
    module.cellI2CErrors = 0;
    
    // Initialize cell statistics
    module.minCellVoltage = 0.0f;
    module.maxCellVoltage = 0.0f;
    module.avgCellVoltage = 0.0f;
    module.totalCellVoltage = 0.0f;
    
    // Initialize temperature statistics
    module.minCellTemp = 25.0f;
    module.maxCellTemp = 25.0f;
    module.avgCellTemp = 25.0f;
    
    // Initialize hardware capabilities
    module.maxChargeCurrent = 0.0f;
    module.maxDischargeCurrent = 0.0f;
    module.maxChargeVoltage = 0.0f;
    module.hardwareVersion = 0;
    
    // Don't initialize cell arrays - wait for actual cell count from MODULE_STATUS_1
    // module.cellVoltages will be empty (size 0) until we receive the status message
    module.cellVoltages.clear();
    module.cellTemperatures.clear();
    
    modules[moduleId] = module;
    return true;
}

bool ModuleManager::DeregisterModule(uint8_t moduleId) {
    std::map<uint8_t, ModuleInfo>::iterator it = modules.find(moduleId);
    if (it != modules.end()) {
        // Keep unique ID to prevent slot reuse
        it->second.isRegistered = false;
        it->second.isResponding = false;
        it->second.state = ModuleState::OFF;
        return true;
    }
    return false;
}

void ModuleManager::DeregisterAllModules() {
    // Keep unique IDs but mark all as not registered
    for (std::map<uint8_t, ModuleInfo>::iterator it = modules.begin(); it != modules.end(); ++it) {
        it->second.isRegistered = false;
        it->second.isResponding = false;
        it->second.state = ModuleState::OFF;
    }
}

bool ModuleManager::SetModuleState(uint8_t moduleId, ModuleState state) {
    std::map<uint8_t, ModuleInfo>::iterator it = modules.find(moduleId);
    if (it != modules.end()) {
        it->second.state = state;
        return true;
    }
    return false;
}

bool ModuleManager::SetAllModulesState(ModuleState state) {
    for (std::map<uint8_t, ModuleInfo>::iterator iter = modules.begin(); iter != modules.end(); ++iter) {
        std::pair<const uint8_t, ModuleInfo>& pair = *iter;
        pair.second.state = state;
    }
    return !modules.empty();
}

bool ModuleManager::IsolateModule(uint8_t moduleId) {
    return SetModuleState(moduleId, ModuleState::OFF);
}

bool ModuleManager::EnableBalancing(uint8_t moduleId, uint8_t cellMask) {
    (void)cellMask;  // Suppress unused parameter warning - TODO: implement balancing
    std::map<uint8_t, ModuleInfo>::iterator it = modules.find(moduleId);
    if (it != modules.end()) {
        // TODO: Store balancing state
        return true;
    }
    return false;
}

void ModuleManager::UpdateModuleStatus(uint8_t moduleId, const uint8_t* data) {
    std::map<uint8_t, ModuleInfo>::iterator it = modules.find(moduleId);
    if (it == modules.end()) {
        // Module not registered, add if discovery active
        if (discoveryActive) {
            // Extract unique ID from status message
            uint32_t uniqueId = (data[4] << 24) | (data[5] << 16) | (data[6] << 8) | data[7];
            RegisterModule(moduleId, uniqueId);
            it = modules.find(moduleId);
        } else {
            return;
        }
    }
    
    it->second.lastResponseTime = GetTickCount();
    it->second.lastMessageTime = GetTickCount();
    it->second.messageCount++;
    it->second.isResponding = true;
    it->second.statusPending = false;  // Clear pending flag when we receive status
    
    // Parse status data (format depends on protocol)
    it->second.state = static_cast<ModuleState>(data[0] & 0x07);
    
    totalMessages++;
}

void ModuleManager::UpdateCellVoltages(uint8_t moduleId, uint8_t startCell, const uint16_t* voltages, uint8_t count) {
    std::map<uint8_t, ModuleInfo>::iterator it = modules.find(moduleId);
    if (it == modules.end()) {
        return;
    }
    
    for (uint8_t i = 0; i < count && (startCell + i) < (uint8_t)it->second.cellVoltages.size(); i++) {
        it->second.cellVoltages[startCell + i] = voltages[i] * 0.001f; // Convert mV to V
    }
    
    // Update module voltage as sum of cells
    it->second.voltage = std::accumulate(it->second.cellVoltages.begin(), 
                                         it->second.cellVoltages.end(), 0.0f);
}

void ModuleManager::UpdateCellTemperatures(uint8_t moduleId, uint8_t startCell, const uint16_t* temps, uint8_t count) {
    std::map<uint8_t, ModuleInfo>::iterator it = modules.find(moduleId);
    if (it == modules.end()) {
        return;
    }
    
    for (uint8_t i = 0; i < count && (startCell + i) < (uint8_t)it->second.cellTemperatures.size(); i++) {
        it->second.cellTemperatures[startCell + i] = temps[i] * 0.1f - 273.15f; // Convert to Celsius
    }
    
    // Update average temperature
    float sum = std::accumulate(it->second.cellTemperatures.begin(), 
                                it->second.cellTemperatures.end(), 0.0f);
    it->second.temperature = sum / it->second.cellTemperatures.size();
}

void ModuleManager::UpdateModuleElectrical(uint8_t moduleId, float voltage, float current, float temp) {
    std::map<uint8_t, ModuleInfo>::iterator it = modules.find(moduleId);
    if (it == modules.end()) {
        return;
    }
    
    it->second.voltage = voltage;
    it->second.current = current;
    it->second.temperature = temp;
    it->second.lastMessageTime = GetTickCount();
    it->second.isResponding = true;
}

ModuleInfo* ModuleManager::GetModule(uint8_t moduleId) {
    std::map<uint8_t, ModuleInfo>::iterator it = modules.find(moduleId);
    if (it != modules.end()) {
        return &it->second;
    }
    return NULL;
}

ModuleInfo ModuleManager::GetModuleInfo(uint8_t moduleId) {
    std::map<uint8_t, ModuleInfo>::iterator it = modules.find(moduleId);
    if (it != modules.end()) {
        return it->second;
    }
    // Return empty module info if not found
    ModuleInfo empty;
    memset(&empty, 0, sizeof(empty));
    return empty;
}

std::vector<ModuleInfo*> ModuleManager::GetAllModules() {
    std::vector<ModuleInfo*> result;
    for (std::map<uint8_t, ModuleInfo>::iterator iter = modules.begin(); iter != modules.end(); ++iter) {
        std::pair<const uint8_t, ModuleInfo>& pair = *iter;
        // Only return modules with valid (non-zero) unique IDs
        if (pair.second.uniqueId != 0) {
            result.push_back(&pair.second);
        }
    }
    return result;
}

std::vector<uint8_t> ModuleManager::GetRegisteredModuleIds() {
    std::vector<uint8_t> result;
    for (std::map<uint8_t, ModuleInfo>::const_iterator iter = modules.begin(); iter != modules.end(); ++iter) {
        const std::pair<const uint8_t, ModuleInfo>& pair = *iter;
        if (pair.second.isRegistered) {
            result.push_back(pair.first);
        }
    }
    return result;
}

bool ModuleManager::IsModuleRegistered(uint8_t moduleId) {
    std::map<uint8_t, ModuleInfo>::iterator it = modules.find(moduleId);
    return (it != modules.end() && it->second.isRegistered);
}

bool ModuleManager::IsModuleResponding(uint8_t moduleId) {
    std::map<uint8_t, ModuleInfo>::iterator it = modules.find(moduleId);
    return (it != modules.end() && it->second.isResponding);
}

void ModuleManager::CheckTimeouts(DWORD currentTime, DWORD timeoutMs) {
    for (std::map<uint8_t, ModuleInfo>::iterator it = modules.begin(); it != modules.end(); ++it) {
        ModuleInfo& module = it->second;
        
        // Only check timeout for registered modules that we're waiting on
        if (module.isRegistered && module.waitingForStatusResponse) {
            // Check if we've been waiting for a response longer than timeoutMs
            if ((currentTime - module.statusRequestTime) > timeoutMs) {
                // Module has timed out - deregister it
                module.isRegistered = false;
                module.isResponding = false;
                module.waitingForStatusResponse = false;  // Clear the waiting flag
                module.statusPending = false;  // Clear the pending flag too
                module.state = ModuleState::OFF;
            }
        }
    }
}

void ModuleManager::SetStatusPending(uint8_t moduleId, bool pending) {
    std::map<uint8_t, ModuleInfo>::iterator it = modules.find(moduleId);
    if (it != modules.end()) {
        it->second.statusPending = pending;
        if (pending) {
            // Record when we started waiting for a response
            it->second.statusRequestTime = GetTickCount();
        }
    }
}

float ModuleManager::GetPackVoltage() {
    float total = 0.0f;
    for (std::map<uint8_t, ModuleInfo>::const_iterator iter = modules.begin(); iter != modules.end(); ++iter) {
        const std::pair<const uint8_t, ModuleInfo>& pair = *iter;
        if (pair.second.isRegistered && pair.second.state != ModuleState::OFF) {
            total += pair.second.voltage;
        }
    }
    return total;
}

float ModuleManager::GetPackCurrent() {
    // Assuming modules are in parallel, current is max of all modules
    float maxCurrent = 0.0f;
    for (std::map<uint8_t, ModuleInfo>::const_iterator iter = modules.begin(); iter != modules.end(); ++iter) {
        const std::pair<const uint8_t, ModuleInfo>& pair = *iter;
        if (pair.second.isRegistered && pair.second.state != ModuleState::OFF) {
            if (std::abs(pair.second.current) > std::abs(maxCurrent)) {
                maxCurrent = pair.second.current;
            }
        }
    }
    return maxCurrent;
}

float ModuleManager::GetPackSoc() {
    if (modules.empty()) return 0.0f;
    
    float totalSoc = 0.0f;
    int count = 0;
    for (std::map<uint8_t, ModuleInfo>::const_iterator iter = modules.begin(); iter != modules.end(); ++iter) {
        const std::pair<const uint8_t, ModuleInfo>& pair = *iter;
        if (pair.second.isRegistered) {
            totalSoc += pair.second.soc;
            count++;
        }
    }
    return count > 0 ? totalSoc / count : 0.0f;
}

float ModuleManager::GetMinCellVoltage() {
    float minVoltage = 999.0f;
    for (std::map<uint8_t, ModuleInfo>::const_iterator iter = modules.begin(); iter != modules.end(); ++iter) {
        const std::pair<const uint8_t, ModuleInfo>& pair = *iter;
        if (pair.second.isRegistered) {
            for (size_t i = 0; i < pair.second.cellVoltages.size(); i++) {
                float voltage = pair.second.cellVoltages[i];
                if (voltage > 0.1f && voltage < minVoltage) {
                    minVoltage = voltage;
                }
            }
        }
    }
    return minVoltage < 999.0f ? minVoltage : 0.0f;
}

float ModuleManager::GetMaxCellVoltage() {
    float maxVoltage = 0.0f;
    for (std::map<uint8_t, ModuleInfo>::const_iterator iter = modules.begin(); iter != modules.end(); ++iter) {
        const std::pair<const uint8_t, ModuleInfo>& pair = *iter;
        if (pair.second.isRegistered) {
            for (size_t i = 0; i < pair.second.cellVoltages.size(); i++) {
                float voltage = pair.second.cellVoltages[i];
                if (voltage > maxVoltage) {
                    maxVoltage = voltage;
                }
            }
        }
    }
    return maxVoltage;
}

float ModuleManager::GetAverageTemperature() {
    if (modules.empty()) return 25.0f;
    
    float totalTemp = 0.0f;
    int count = 0;
    for (std::map<uint8_t, ModuleInfo>::const_iterator iter = modules.begin(); iter != modules.end(); ++iter) {
        const std::pair<const uint8_t, ModuleInfo>& pair = *iter;
        if (pair.second.isRegistered) {
            totalTemp += pair.second.temperature;
            count++;
        }
    }
    return count > 0 ? totalTemp / count : 25.0f;
}

bool ModuleManager::CheckForFaults() {
    bool faultsFound = false;
    for (std::map<uint8_t, ModuleInfo>::iterator iter = modules.begin(); iter != modules.end(); ++iter) {
        std::pair<const uint8_t, ModuleInfo>& pair = *iter;
        DetectFaults(pair.second);
        // Check if module has any fault conditions set
        if (pair.second.errorCount > 0 || !pair.second.isResponding) {
            faultsFound = true;
        }
    }
    return faultsFound;
}

std::vector<std::string> ModuleManager::GetActiveFaults() {
    std::vector<std::string> faults;
    
    for (std::map<uint8_t, ModuleInfo>::const_iterator iter = modules.begin(); iter != modules.end(); ++iter) {
        const std::pair<const uint8_t, ModuleInfo>& pair = *iter;
        // Check for modules with fault conditions (not a separate FAULT state)
        if (pair.second.errorCount > 0) {
            char buffer[256];
            sprintf(buffer, "Module %d has errors (count: %d)", pair.first, pair.second.errorCount);
            faults.push_back(buffer);
        }
        
        // Check for specific fault conditions
        for (size_t i = 0; i < pair.second.cellVoltages.size(); i++) {
            float voltage = pair.second.cellVoltages[i];
            if (voltage < 2.5f && voltage > 0.1f) {
                char buffer[256];
                sprintf(buffer, "Module %d Cell %d undervoltage: %.2fV", pair.first, i, voltage);
                faults.push_back(buffer);
            }
            if (voltage > 4.2f) {
                char buffer[256];
                sprintf(buffer, "Module %d Cell %d overvoltage: %.2fV", pair.first, i, voltage);
                faults.push_back(buffer);
            }
        }
        
        if (pair.second.temperature > 60.0f) {
            char buffer[256];
            sprintf(buffer, "Module %d overtemperature: %.1f°C", pair.first, pair.second.temperature);
            faults.push_back(buffer);
        }
        
        if (!pair.second.isResponding) {
            char buffer[256];
            sprintf(buffer, "Module %d not responding", pair.first);
            faults.push_back(buffer);
        }
    }
    
    return faults;
}

void ModuleManager::ClearFaults() {
    for (std::map<uint8_t, ModuleInfo>::iterator iter = modules.begin(); iter != modules.end(); ++iter) {
        std::pair<const uint8_t, ModuleInfo>& pair = *iter;
        // Clear error counts and reset non-responding modules to OFF
        if (pair.second.errorCount > 0 || !pair.second.isResponding) {
            pair.second.errorCount = 0;
            if (!pair.second.isResponding) {
                pair.second.state = ModuleState::OFF;
            }
        }
    }
}

bool ModuleManager::DistributeWeb4Keys(uint8_t moduleId, const uint8_t* deviceKey, const uint8_t* lctKey) {
    std::map<uint8_t, ModuleInfo>::iterator it = modules.find(moduleId);
    if (it == modules.end()) {
        return false;
    }
    
    std::memcpy(it->second.web4DeviceKeyHalf, deviceKey, 64);
    std::memcpy(it->second.web4LctKeyHalf, lctKey, 64);
    it->second.hasWeb4Keys = true;
    
    return true;
}

bool ModuleManager::StoreWeb4ComponentId(uint8_t moduleId, const std::string& componentId) {
    std::map<uint8_t, ModuleInfo>::iterator it = modules.find(moduleId);
    if (it == modules.end()) {
        return false;
    }
    
    it->second.web4ComponentId = componentId;
    return true;
}

void ModuleManager::CheckTimeouts() {
    DWORD now = GetTickCount();
    
    for (std::map<uint8_t, ModuleInfo>::iterator iter = modules.begin(); iter != modules.end(); ++iter) {
        std::pair<const uint8_t, ModuleInfo>& pair = *iter;
        // Handle tick count wraparound (happens every ~49 days)
        DWORD elapsed = (now >= pair.second.lastMessageTime) ? 
                        (now - pair.second.lastMessageTime) : 
                        (0xFFFFFFFF - pair.second.lastMessageTime + now + 1);
            
        if (elapsed > moduleTimeoutMs) {
            pair.second.isResponding = false;
            pair.second.errorCount++;
            totalErrors++;
        }
    }
}

void ModuleManager::UpdateStatistics() {
    // Calculate uptime
    DWORD now = GetTickCount();
    DWORD uptimeMs = (now >= startTime) ? 
                     (now - startTime) : 
                     (0xFFFFFFFF - startTime + now + 1);
    // DWORD uptime = uptimeMs / 1000;  // Convert to seconds - TODO: use this value
    (void)uptimeMs;  // Suppress unused warning
    
    // Update other statistics as needed
}

bool ModuleManager::ValidateModuleId(uint8_t moduleId) {
    return moduleId > 0 && moduleId <= 32;
}

bool ModuleManager::ValidateCellData(uint8_t moduleId, uint8_t cellIndex) {
    std::map<uint8_t, ModuleInfo>::iterator it = modules.find(moduleId);
    if (it == modules.end()) {
        return false;
    }
    return cellIndex < (uint8_t)it->second.cellVoltages.size();
}

void ModuleManager::CalculatePackValues() {
    // This would be called periodically to update pack-level calculations
    // Already implemented in individual getter functions
}

void ModuleManager::DetectFaults(ModuleInfo& module) {
    // Check for various fault conditions
    
    // Undervoltage - increment error count instead of setting FAULT state
    for (size_t i = 0; i < module.cellVoltages.size(); i++) {
        float voltage = module.cellVoltages[i];
        if (voltage < 2.5f && voltage > 0.1f) {
            module.errorCount++;
            // Module stays in current state but with error flag
            return;
        }
    }
    
    // Overvoltage - increment error count instead of setting FAULT state
    for (size_t i = 0; i < module.cellVoltages.size(); i++) {
        float voltage = module.cellVoltages[i];
        if (voltage > 4.2f) {
            module.errorCount++;
            // Module stays in current state but with error flag
            return;
        }
    }
    
    // Overtemperature - increment error count instead of setting FAULT state
    if (module.temperature > 60.0f) {
        module.errorCount++;
        // Module stays in current state but with error flag
        return;
    }
    
    // Communication timeout - mark as not responding but don't change state
    if (!module.isResponding) {
        module.errorCount++;
        // Module keeps current state, isResponding flag already indicates the issue
        return;
    }
}

} // namespace PackEmulator
