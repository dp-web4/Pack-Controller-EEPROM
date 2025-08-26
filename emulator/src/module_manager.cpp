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
#include <cstring>

namespace PackEmulator {

ModuleManager::ModuleManager() 
    : discoveryActive(false)
    , minDiscoveryId(1)
    , moduleTimeoutMs(5000)
    , maxModules(32)
    , totalMessages(0)
    , totalErrors(0) {
    startTime = std::chrono::steady_clock::now();
}

ModuleManager::~ModuleManager() {
    StopDiscovery();
}

void ModuleManager::StartDiscovery() {
    discoveryActive = true;
    minDiscoveryId = 1;
    
    // Clear any existing unregistered modules
    for (auto it = modules.begin(); it != modules.end(); ) {
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
    
    if (modules.size() >= maxModules) {
        return false;
    }
    
    ModuleInfo module;
    module.moduleId = moduleId;
    module.uniqueId = uniqueId;
    module.state = ModuleState::OFF;
    module.isRegistered = true;
    module.isResponding = true;
    module.hasWeb4Keys = false;
    module.lastMessageTime = std::chrono::steady_clock::now();
    module.messageCount = 0;
    module.errorCount = 0;
    
    // Initialize electrical data
    module.voltage = 0.0f;
    module.current = 0.0f;
    module.temperature = 25.0f;
    module.soc = 0.0f;
    module.soh = 100.0f;
    
    // Initialize cell arrays (assume 14 cells per module as typical)
    module.cellVoltages.resize(14, 0.0f);
    module.cellTemperatures.resize(14, 25.0f);
    
    modules[moduleId] = module;
    return true;
}

bool ModuleManager::DeregisterModule(uint8_t moduleId) {
    auto it = modules.find(moduleId);
    if (it != modules.end()) {
        modules.erase(it);
        return true;
    }
    return false;
}

void ModuleManager::DeregisterAllModules() {
    modules.clear();
}

bool ModuleManager::SetModuleState(uint8_t moduleId, ModuleState state) {
    auto it = modules.find(moduleId);
    if (it != modules.end()) {
        it->second.state = state;
        return true;
    }
    return false;
}

bool ModuleManager::SetAllModulesState(ModuleState state) {
    for (auto& pair : modules) {
        pair.second.state = state;
    }
    return !modules.empty();
}

bool ModuleManager::IsolateModule(uint8_t moduleId) {
    return SetModuleState(moduleId, ModuleState::OFF);
}

bool ModuleManager::EnableBalancing(uint8_t moduleId, uint8_t cellMask) {
    auto it = modules.find(moduleId);
    if (it != modules.end()) {
        // TODO: Store balancing state
        return true;
    }
    return false;
}

void ModuleManager::UpdateModuleStatus(uint8_t moduleId, const uint8_t* data) {
    auto it = modules.find(moduleId);
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
    
    it->second.lastMessageTime = std::chrono::steady_clock::now();
    it->second.messageCount++;
    it->second.isResponding = true;
    
    // Parse status data (format depends on protocol)
    it->second.state = static_cast<ModuleState>(data[0] & 0x07);
    
    totalMessages++;
}

void ModuleManager::UpdateCellVoltages(uint8_t moduleId, uint8_t startCell, const uint16_t* voltages, uint8_t count) {
    auto it = modules.find(moduleId);
    if (it == modules.end()) {
        return;
    }
    
    for (uint8_t i = 0; i < count && (startCell + i) < it->second.cellVoltages.size(); i++) {
        it->second.cellVoltages[startCell + i] = voltages[i] * 0.001f; // Convert mV to V
    }
    
    // Update module voltage as sum of cells
    it->second.voltage = std::accumulate(it->second.cellVoltages.begin(), 
                                         it->second.cellVoltages.end(), 0.0f);
}

void ModuleManager::UpdateCellTemperatures(uint8_t moduleId, uint8_t startCell, const uint16_t* temps, uint8_t count) {
    auto it = modules.find(moduleId);
    if (it == modules.end()) {
        return;
    }
    
    for (uint8_t i = 0; i < count && (startCell + i) < it->second.cellTemperatures.size(); i++) {
        it->second.cellTemperatures[startCell + i] = temps[i] * 0.1f - 273.15f; // Convert to Celsius
    }
    
    // Update average temperature
    float sum = std::accumulate(it->second.cellTemperatures.begin(), 
                                it->second.cellTemperatures.end(), 0.0f);
    it->second.temperature = sum / it->second.cellTemperatures.size();
}

void ModuleManager::UpdateModuleElectrical(uint8_t moduleId, float voltage, float current, float temp) {
    auto it = modules.find(moduleId);
    if (it == modules.end()) {
        return;
    }
    
    it->second.voltage = voltage;
    it->second.current = current;
    it->second.temperature = temp;
    it->second.lastMessageTime = std::chrono::steady_clock::now();
    it->second.isResponding = true;
}

ModuleInfo* ModuleManager::GetModule(uint8_t moduleId) {
    auto it = modules.find(moduleId);
    if (it != modules.end()) {
        return &it->second;
    }
    return nullptr;
}

std::vector<ModuleInfo*> ModuleManager::GetAllModules() {
    std::vector<ModuleInfo*> result;
    for (auto& pair : modules) {
        result.push_back(&pair.second);
    }
    return result;
}

std::vector<uint8_t> ModuleManager::GetRegisteredModuleIds() {
    std::vector<uint8_t> result;
    for (const auto& pair : modules) {
        if (pair.second.isRegistered) {
            result.push_back(pair.first);
        }
    }
    return result;
}

bool ModuleManager::IsModuleRegistered(uint8_t moduleId) {
    auto it = modules.find(moduleId);
    return (it != modules.end() && it->second.isRegistered);
}

bool ModuleManager::IsModuleResponding(uint8_t moduleId) {
    auto it = modules.find(moduleId);
    return (it != modules.end() && it->second.isResponding);
}

float ModuleManager::GetPackVoltage() {
    float total = 0.0f;
    for (const auto& pair : modules) {
        if (pair.second.isRegistered && pair.second.state != ModuleState::OFF) {
            total += pair.second.voltage;
        }
    }
    return total;
}

float ModuleManager::GetPackCurrent() {
    // Assuming modules are in parallel, current is max of all modules
    float maxCurrent = 0.0f;
    for (const auto& pair : modules) {
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
    for (const auto& pair : modules) {
        if (pair.second.isRegistered) {
            totalSoc += pair.second.soc;
            count++;
        }
    }
    return count > 0 ? totalSoc / count : 0.0f;
}

float ModuleManager::GetMinCellVoltage() {
    float minVoltage = 999.0f;
    for (const auto& pair : modules) {
        if (pair.second.isRegistered) {
            for (float voltage : pair.second.cellVoltages) {
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
    for (const auto& pair : modules) {
        if (pair.second.isRegistered) {
            for (float voltage : pair.second.cellVoltages) {
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
    for (const auto& pair : modules) {
        if (pair.second.isRegistered) {
            totalTemp += pair.second.temperature;
            count++;
        }
    }
    return count > 0 ? totalTemp / count : 25.0f;
}

bool ModuleManager::CheckForFaults() {
    bool faultsFound = false;
    for (auto& pair : modules) {
        DetectFaults(pair.second);
        if (pair.second.state == ModuleState::FAULT) {
            faultsFound = true;
        }
    }
    return faultsFound;
}

std::vector<std::string> ModuleManager::GetActiveFaults() {
    std::vector<std::string> faults;
    
    for (const auto& pair : modules) {
        if (pair.second.state == ModuleState::FAULT) {
            faults.push_back("Module " + std::to_string(pair.first) + " in fault state");
        }
        
        // Check for specific fault conditions
        for (size_t i = 0; i < pair.second.cellVoltages.size(); i++) {
            float voltage = pair.second.cellVoltages[i];
            if (voltage < 2.5f && voltage > 0.1f) {
                faults.push_back("Module " + std::to_string(pair.first) + 
                               " Cell " + std::to_string(i) + " undervoltage: " + 
                               std::to_string(voltage) + "V");
            }
            if (voltage > 4.2f) {
                faults.push_back("Module " + std::to_string(pair.first) + 
                               " Cell " + std::to_string(i) + " overvoltage: " + 
                               std::to_string(voltage) + "V");
            }
        }
        
        if (pair.second.temperature > 60.0f) {
            faults.push_back("Module " + std::to_string(pair.first) + 
                           " overtemperature: " + std::to_string(pair.second.temperature) + "°C");
        }
        
        if (!pair.second.isResponding) {
            faults.push_back("Module " + std::to_string(pair.first) + " not responding");
        }
    }
    
    return faults;
}

void ModuleManager::ClearFaults() {
    for (auto& pair : modules) {
        if (pair.second.state == ModuleState::FAULT) {
            pair.second.state = ModuleState::OFF;
        }
    }
}

bool ModuleManager::DistributeWeb4Keys(uint8_t moduleId, const uint8_t* deviceKey, const uint8_t* lctKey) {
    auto it = modules.find(moduleId);
    if (it == modules.end()) {
        return false;
    }
    
    std::memcpy(it->second.web4DeviceKeyHalf, deviceKey, 64);
    std::memcpy(it->second.web4LctKeyHalf, lctKey, 64);
    it->second.hasWeb4Keys = true;
    
    return true;
}

bool ModuleManager::StoreWeb4ComponentId(uint8_t moduleId, const std::string& componentId) {
    auto it = modules.find(moduleId);
    if (it == modules.end()) {
        return false;
    }
    
    it->second.web4ComponentId = componentId;
    return true;
}

void ModuleManager::CheckTimeouts() {
    auto now = std::chrono::steady_clock::now();
    
    for (auto& pair : modules) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - pair.second.lastMessageTime).count();
            
        if (elapsed > moduleTimeoutMs) {
            pair.second.isResponding = false;
            pair.second.errorCount++;
            totalErrors++;
        }
    }
}

void ModuleManager::UpdateStatistics() {
    // Calculate uptime
    auto now = std::chrono::steady_clock::now();
    auto uptime = std::chrono::duration_cast<std::chrono::seconds>(now - startTime).count();
    
    // Update other statistics as needed
}

bool ModuleManager::ValidateModuleId(uint8_t moduleId) {
    return moduleId > 0 && moduleId <= 32;
}

bool ModuleManager::ValidateCellData(uint8_t moduleId, uint8_t cellIndex) {
    auto it = modules.find(moduleId);
    if (it == modules.end()) {
        return false;
    }
    return cellIndex < it->second.cellVoltages.size();
}

void ModuleManager::CalculatePackValues() {
    // This would be called periodically to update pack-level calculations
    // Already implemented in individual getter functions
}

void ModuleManager::DetectFaults(ModuleInfo& module) {
    // Check for various fault conditions
    
    // Undervoltage
    for (float voltage : module.cellVoltages) {
        if (voltage < 2.5f && voltage > 0.1f) {
            module.state = ModuleState::FAULT;
            return;
        }
    }
    
    // Overvoltage
    for (float voltage : module.cellVoltages) {
        if (voltage > 4.2f) {
            module.state = ModuleState::FAULT;
            return;
        }
    }
    
    // Overtemperature
    if (module.temperature > 60.0f) {
        module.state = ModuleState::FAULT;
        return;
    }
    
    // Communication timeout
    if (!module.isResponding) {
        module.state = ModuleState::FAULT;
        return;
    }
}

} // namespace PackEmulator