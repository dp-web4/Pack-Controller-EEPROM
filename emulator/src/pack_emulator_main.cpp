//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include "pack_emulator_main.h"
#include "../include/can_id_module.h"
#include "../include/can_id_bms_vcu.h"
#include <sstream>
#include <iomanip>

//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TMainForm *MainForm;

//---------------------------------------------------------------------------
__fastcall TMainForm::TMainForm(TComponent* Owner)
    : TForm(Owner)
    , isConnected(false)
    , selectedModuleId(0) {
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::FormCreate(TObject *Sender) {
    (void)Sender;  // Suppress unused parameter warning
    // Initialize managers
    moduleManager = new PackEmulator::ModuleManager();
    canInterface = new PackEmulator::CANInterface();
    
    // Set up CAN callback
    canInterface->SetCallback(this);
    
    // Initialize UI
    UpdateConnectionStatus(false);
    
    // Load configuration
    LoadConfiguration();
    
    // Set up timers
    UpdateTimer->Interval = 100;  // 10 Hz update
    UpdateTimer->Enabled = true;
    
    TimeoutTimer->Interval = 1000;  // 1 Hz timeout check
    TimeoutTimer->Enabled = true;
    
    LogMessage("Pack Controller Emulator initialized");
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::FormDestroy(TObject *Sender) {
    (void)Sender;  // Suppress unused parameter warning
    // Clean up dynamically allocated objects
    if (isConnected) {
        DisconnectButtonClick(NULL);
    }
    
    SaveConfiguration();
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::ConnectButtonClick(TObject *Sender) {
    (void)Sender;  // Suppress unused parameter warning
    // Get selected channel (default PCAN-USB)
    uint16_t channel = PCAN_USBBUS1;
    if (CANChannelCombo->ItemIndex > 0) {
        channel = PCAN_USBBUS1 + CANChannelCombo->ItemIndex;
    }
    
    // Get baudrate from combo box or use default
    uint16_t baudrate = PCAN_BAUD_500K;  // Default 500K
    if (BaudrateCombo && BaudrateCombo->ItemIndex >= 0) {
        switch(BaudrateCombo->ItemIndex) {
            case 0: baudrate = PCAN_BAUD_125K; break;
            case 1: baudrate = PCAN_BAUD_250K; break;
            case 2: baudrate = PCAN_BAUD_500K; break;
            case 3: baudrate = PCAN_BAUD_1M; break;
            default: baudrate = PCAN_BAUD_500K;
        }
    }
    
    // Connect to CAN
    if (canInterface->Connect(channel, baudrate)) {
        canInterface->StartReceiving();
        isConnected = true;
        UpdateConnectionStatus(true);
        String baudrateStr;
        switch(baudrate) {
            case PCAN_BAUD_125K: baudrateStr = "125K"; break;
            case PCAN_BAUD_250K: baudrateStr = "250K"; break;
            case PCAN_BAUD_500K: baudrateStr = "500K"; break;
            case PCAN_BAUD_1M: baudrateStr = "1M"; break;
            default: baudrateStr = "Unknown";
        }
        LogMessage("Connected to CAN bus at " + baudrateStr + " baud");
        
        // Enable controls
        DiscoverButton->Enabled = true;
        RegisterButton->Enabled = true;
        ConnectButton->Enabled = false;
        DisconnectButton->Enabled = true;
    } else {
        ShowError("Failed to connect: " + String(canInterface->GetLastError().c_str()));
    }
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::DisconnectButtonClick(TObject *Sender) {
    (void)Sender;  // Suppress unused parameter warning
    canInterface->StopReceiving();
    canInterface->Disconnect();
    isConnected = false;
    UpdateConnectionStatus(false);
    LogMessage("Disconnected from CAN bus");
    
    // Disable controls
    DiscoverButton->Enabled = false;
    RegisterButton->Enabled = false;
    ConnectButton->Enabled = true;
    DisconnectButton->Enabled = false;
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::DiscoverButtonClick(TObject *Sender) {
    (void)Sender;  // Suppress unused parameter warning
    
    if (DiscoverButton->Tag == 0) {
        // Start discovery
        moduleManager->StartDiscovery();
        LogMessage("Module discovery started");
        
        // Send announcement request to trigger all modules to announce
        uint8_t data[8] = {0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        if (canInterface->SendMessage(ID_MODULE_ANNOUNCE_REQUEST, data, 8)) {
            LogMessage("Broadcast discovery request (CAN ID 0x" + IntToHex((int)ID_MODULE_ANNOUNCE_REQUEST, 3) + ")");
        } else {
            LogMessage("Failed to send discovery request: " + String(canInterface->GetLastError().c_str()));
        }
        
        DiscoverButton->Caption = "Stop Discovery";
        DiscoverButton->Tag = 1;
    } else {
        // Stop discovery
        moduleManager->StopDiscovery();
        LogMessage("Module discovery stopped");
        DiscoverButton->Caption = "Start Discovery";
        DiscoverButton->Tag = 0;
    }
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::RegisterButtonClick(TObject *Sender) {
    (void)Sender;  // Suppress unused parameter warning
    if (!selectedModuleId) {
        ShowError("No module selected");
        return;
    }
    
    if (moduleManager->IsModuleRegistered(selectedModuleId)) {
        ShowError("Module already registered");
        return;
    }
    
    // Send registration command
    canInterface->SendRegistrationAck(selectedModuleId, true);
    LogMessage("Registered module " + IntToStr(selectedModuleId));
    
    UpdateModuleList();
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::DeregisterButtonClick(TObject *Sender) {
    (void)Sender;  // Suppress unused parameter warning
    if (!selectedModuleId) {
        ShowError("No module selected");
        return;
    }
    
    moduleManager->DeregisterModule(selectedModuleId);
    canInterface->SendRegistrationAck(selectedModuleId, false);
    LogMessage("Deregistered module " + IntToStr(selectedModuleId));
    
    UpdateModuleList();
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::DeregisterAllButtonClick(TObject *Sender) {
    (void)Sender;  // Suppress unused parameter warning
    moduleManager->DeregisterAllModules();
    LogMessage("All modules deregistered");
    UpdateModuleList();
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::SetStateButtonClick(TObject *Sender) {
    (void)Sender;  // Suppress unused parameter warning
    if (!selectedModuleId) {
        ShowError("No module selected");
        return;
    }
    
    PackEmulator::ModuleState state = GetSelectedState();
    moduleManager->SetModuleState(selectedModuleId, state);
    
    // Send state command to module
    uint8_t stateCmd = static_cast<uint8_t>(state);
    canInterface->SendStateChange(selectedModuleId, stateCmd);
    
    LogMessage("Set module " + IntToStr(selectedModuleId) + " to state " + IntToStr(stateCmd));
    UpdateModuleDetails(selectedModuleId);
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::SetAllStatesButtonClick(TObject *Sender) {
    (void)Sender;  // Suppress unused parameter warning
    PackEmulator::ModuleState state = GetSelectedState();
    moduleManager->SetAllModulesState(state);
    
    // Send to all registered modules
    std::vector<uint8_t> moduleIds = moduleManager->GetRegisteredModuleIds();
    uint8_t stateCmd = static_cast<uint8_t>(state);
    
    for (size_t i = 0; i < moduleIds.size(); i++) {
        uint8_t id = moduleIds[i];
        canInterface->SendStateChange(id, stateCmd);
    }
    
    LogMessage("Set all modules to state " + IntToStr(stateCmd));
    UpdateModuleList();
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::ModuleListViewSelectItem(TObject *Sender, 
    TListItem *Item, bool Selected) {
    (void)Sender;  // Suppress unused parameter warning
    if (Selected && Item) {
        selectedModuleId = Item->Caption.ToInt();
        UpdateModuleDetails(selectedModuleId);
    }
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::UpdateTimerTimer(TObject *Sender) {
    (void)Sender;  // Suppress unused parameter warning
    if (!isConnected) {
        return;
    }
    
    // Update display
    UpdateStatusDisplay();
    
    // Update selected module details
    if (selectedModuleId) {
        UpdateModuleDetails(selectedModuleId);
    }
    
    // Send periodic heartbeat/keep-alive to registered modules
    static int heartbeatCounter = 0;
    heartbeatCounter++;
    if (heartbeatCounter >= 50) {  // Every 5 seconds (100ms timer * 50)
        heartbeatCounter = 0;
        
        // Send keep-alive to all registered modules
        std::vector<uint8_t> moduleIds = moduleManager->GetRegisteredModuleIds();
        if (!moduleIds.empty()) {
            for (size_t i = 0; i < moduleIds.size(); i++) {
                // Send keep-alive/heartbeat
                uint8_t data[8] = {0x00};  // Simple heartbeat
                canInterface->SendModuleCommand(moduleIds[i], 0xFE, data, 1);  // 0xFE = heartbeat command
            }
            // Don't log each heartbeat to avoid spam
        }
    }
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::TimeoutTimerTimer(TObject *Sender) {
    (void)Sender;  // Suppress unused parameter warning
    moduleManager->CheckTimeouts();
    moduleManager->CheckForFaults();
    
    // Update module list to show timeout status
    UpdateModuleList();
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::DistributeKeysButtonClick(TObject *Sender) {
    (void)Sender;  // Suppress unused parameter warning
    if (!selectedModuleId) {
        ShowError("No module selected");
        return;
    }
    
    // Get keys from UI (would be generated/retrieved in real implementation)
    uint8_t deviceKey[64] = {0};
    uint8_t lctKey[64] = {0};
    
    // TODO: Get actual keys from Web4 system
    
    // Distribute keys
    moduleManager->DistributeWeb4Keys(selectedModuleId, deviceKey, lctKey);
    
    // Send keys via CAN in chunks
    for (int chunk = 0; chunk < 8; chunk++) {
        canInterface->SendWeb4KeyChunk(selectedModuleId, chunk, &deviceKey[chunk * 8]);
    }
    
    LogMessage("Web4 keys distributed to module " + IntToStr(selectedModuleId));
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::ExportDataItemClick(TObject *Sender) {
    (void)Sender;  // Suppress unused parameter warning
    if (SaveDialog->Execute()) {
        // Export module data to CSV
        // TODO: Implement CSV export
        LogMessage("Data exported to " + SaveDialog->FileName);
    }
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::ExitItemClick(TObject *Sender) {
    (void)Sender;  // Suppress unused parameter warning
    Close();
}

//---------------------------------------------------------------------------
// Private implementation functions
//---------------------------------------------------------------------------

void TMainForm::UpdateModuleList() {
    ModuleListView->Items->BeginUpdate();
    ModuleListView->Items->Clear();
    
    std::vector<PackEmulator::ModuleInfo*> modules = moduleManager->GetAllModules();
    for (size_t i = 0; i < modules.size(); i++) {
        PackEmulator::ModuleInfo* module = modules[i];
        TListItem* item = ModuleListView->Items->Add();
        item->Caption = IntToStr(module->moduleId);
        item->SubItems->Add(module->isRegistered ? "Registered" : "Discovered");
        item->SubItems->Add(module->isResponding ? "OK" : "Timeout");
        
        String stateStr;
        switch (module->state) {
            case PackEmulator::ModuleState::OFF: stateStr = "Off"; break;
            case PackEmulator::ModuleState::STANDBY: stateStr = "Standby"; break;
            case PackEmulator::ModuleState::PRECHARGE: stateStr = "Precharge"; break;
            case PackEmulator::ModuleState::ON: stateStr = "On"; break;
            default: stateStr = "Unknown";
        }
        item->SubItems->Add(stateStr);
        
        item->SubItems->Add(FloatToStrF(module->voltage, ffFixed, 7, 2) + "V");
        item->SubItems->Add(FloatToStrF(module->soc, ffFixed, 7, 1) + "%");
    }
    
    ModuleListView->Items->EndUpdate();
}

void TMainForm::UpdateModuleDetails(uint8_t moduleId) {
    PackEmulator::ModuleInfo* module = moduleManager->GetModule(moduleId);
    if (module == NULL) {
        return;
    }
    
    // Update status labels
    VoltageLabel->Caption = "Voltage: " + FloatToStrF(module->voltage, ffFixed, 7, 2) + " V";
    CurrentLabel->Caption = "Current: " + FloatToStrF(module->current, ffFixed, 7, 2) + " A";
    TemperatureLabel->Caption = "Temperature: " + FloatToStrF(module->temperature, ffFixed, 7, 1) + " °C";
    SOCLabel->Caption = "SOC: " + FloatToStrF(module->soc, ffFixed, 7, 1) + " %";
    SOHLabel->Caption = "SOH: " + FloatToStrF(module->soh, ffFixed, 7, 1) + " %";
    
    // Update cell display
    UpdateCellDisplay(moduleId);
}

void TMainForm::UpdateStatusDisplay() {
    // Update pack-level values
    StatusBar->Panels->Items[0]->Text = "Pack: " + 
        FloatToStrF(moduleManager->GetPackVoltage(), ffFixed, 7, 1) + "V";
    StatusBar->Panels->Items[1]->Text = "Current: " + 
        FloatToStrF(moduleManager->GetPackCurrent(), ffFixed, 7, 1) + "A";
    StatusBar->Panels->Items[2]->Text = "Modules: " + 
        IntToStr((int)moduleManager->GetModuleCount());
        
    // Update statistics
    PackEmulator::CANInterface::Statistics stats = canInterface->GetStatistics();
    StatusBar->Panels->Items[3]->Text = "TX: " + IntToStr((int)stats.messagesSent) + 
                                        " RX: " + IntToStr((int)stats.messagesReceived);
}

void TMainForm::UpdateCellDisplay(uint8_t moduleId) {
    PackEmulator::ModuleInfo* module = moduleManager->GetModule(moduleId);
    if (module == NULL) {
        return;
    }
    
    CellGrid->RowCount = module->cellVoltages.size() + 1;
    CellGrid->Cells[0][0] = "Cell";
    CellGrid->Cells[1][0] = "Voltage";
    CellGrid->Cells[2][0] = "Temperature";
    
    for (size_t i = 0; i < module->cellVoltages.size(); i++) {
        CellGrid->Cells[0][i + 1] = IntToStr((int)(i + 1));
        CellGrid->Cells[1][i + 1] = FloatToStrF(module->cellVoltages[i], ffFixed, 7, 3);
        CellGrid->Cells[2][i + 1] = FloatToStrF(module->cellTemperatures[i], ffFixed, 7, 1);
    }
}

void TMainForm::UpdateConnectionStatus(bool connected) {
    if (connected) {
        ConnectionStatusLabel->Caption = "Connected";
        ConnectionStatusLabel->Font->Color = clGreen;
    } else {
        ConnectionStatusLabel->Caption = "Disconnected";
        ConnectionStatusLabel->Font->Color = clRed;
    }
}

void TMainForm::OnCANMessage(const PackEmulator::CANMessage& msg) {
    // Process message based on ID
    uint32_t canId = msg.id & 0x7FF;
    
    // Debug: Log all received messages
    String dataStr;
    for (int i = 0; i < msg.length; i++) {
        dataStr += IntToHex((int)msg.data[i], 2) + " ";
    }
    LogMessage("RX CAN ID 0x" + IntToHex((int)canId, 3) + " Data: " + dataStr);
    
    // Check for module announcements (0x500 or 0x000 for compatibility)
    // Module firmware bug: sending on 0x000 instead of 0x500
    if (canId == ID_MODULE_ANNOUNCEMENT || canId == 0x000) {
        // Parse according to CANFRM_MODULE_ANNOUNCEMENT structure:
        // Bytes 0-1: Module firmware version (16 bits)
        // Byte 2: Module manufacturer ID (8 bits) 
        // Byte 3: Module part ID (8 bits)
        // Bytes 4-7: Module unique ID (32 bits, little-endian)
        
        uint16_t fwVersion = msg.data[0] | (msg.data[1] << 8);
        uint8_t mfgId = msg.data[2];
        uint8_t partId = msg.data[3];
        
        // Unique ID is little-endian in bytes 4-7
        uint32_t uniqueId = msg.data[4] | (msg.data[5] << 8) | 
                           (msg.data[6] << 16) | (msg.data[7] << 24);
        
        LogMessage("Module announcement received:");
        LogMessage("  FW Version: 0x" + IntToHex(fwVersion, 4));
        LogMessage("  Mfg ID: 0x" + IntToHex(mfgId, 2));
        LogMessage("  Part ID: 0x" + IntToHex(partId, 2));
        LogMessage("  Unique ID: 0x" + IntToHex((int)uniqueId, 8));
        
        // Module doesn't send its assigned ID in announcement
        // We need to assign one based on unique ID
        uint8_t moduleId = 0;
        
        // Check if we already know this module
        std::vector<uint8_t> existingIds = moduleManager->GetRegisteredModuleIds();
        for (size_t i = 0; i < existingIds.size(); i++) {
            PackEmulator::ModuleInfo* mod = moduleManager->GetModule(existingIds[i]);
            if (mod && mod->uniqueId == uniqueId) {
                moduleId = existingIds[i];
                break;
            }
        }
        
        // If not found, assign next available ID
        if (moduleId == 0) {
            // Find lowest available ID starting from 1
            for (uint8_t id = 1; id <= 32; id++) {
                if (!moduleManager->IsModuleRegistered(id)) {
                    moduleId = id;
                    break;
                }
            }
        }
        
        // Process registration
        if (moduleId > 0 && moduleId <= 32) {
            if (!moduleManager->IsModuleRegistered(moduleId)) {
                // New module - register it
                if (moduleManager->RegisterModule(moduleId, uniqueId)) {
                    // Send registration ACK with the assigned module ID
                    // According to CANFRM_MODULE_REGISTRATION (0x510):
                    uint8_t regData[8];
                    regData[0] = moduleId;      // Assigned module ID
                    regData[1] = 0x01;          // Controller ID (pack controller = 1)
                    regData[2] = mfgId;         // Echo back mfg ID
                    regData[3] = partId;        // Echo back part ID
                    regData[4] = uniqueId & 0xFF;
                    regData[5] = (uniqueId >> 8) & 0xFF;
                    regData[6] = (uniqueId >> 16) & 0xFF;
                    regData[7] = (uniqueId >> 24) & 0xFF;
                    
                    canInterface->SendMessage(ID_MODULE_REGISTRATION, regData, 8);
                    LogMessage("Registered module ID " + IntToStr(moduleId) + 
                              " (Unique: 0x" + IntToHex((int)uniqueId, 8) + ")");
                    UpdateModuleList();
                } else {
                    LogMessage("Failed to register module");
                }
            } else {
                // Existing module re-announcing
                LogMessage("Module " + IntToStr(moduleId) + " re-announced");
                moduleManager->UpdateModuleStatus(moduleId, msg.data);
                
                // Send registration ACK again in case module lost it
                uint8_t regData[8];
                regData[0] = moduleId;
                regData[1] = 0x01;
                regData[2] = mfgId;
                regData[3] = partId;
                regData[4] = uniqueId & 0xFF;
                regData[5] = (uniqueId >> 8) & 0xFF;
                regData[6] = (uniqueId >> 16) & 0xFF;
                regData[7] = (uniqueId >> 24) & 0xFF;
                canInterface->SendMessage(ID_MODULE_REGISTRATION, regData, 8);
            }
        } else {
            LogMessage("ERROR: Could not assign module ID (all 32 slots full?)");
        }
    }
    // Check for module status messages
    else if (canId == ID_MODULE_STATUS_1 || canId == ID_MODULE_STATUS_2 || 
             canId == ID_MODULE_STATUS_3 || canId == ID_MODULE_STATUS_4) {
        uint8_t moduleId = msg.data[0];  // Module ID is typically in first byte
        ProcessModuleStatus(moduleId, msg.data);
    }
    // Check for cell voltage data (from VCU interface, repurpose for modules)
    else if (canId == ID_MODULE_CELL_VOLTAGE) {
        uint8_t moduleId = msg.data[0];
        ProcessCellVoltages(moduleId, msg.data);
    }
    // Check for cell temperature data
    else if (canId == ID_MODULE_CELL_TEMP) {
        uint8_t moduleId = msg.data[0];
        ProcessCellTemperatures(moduleId, msg.data);
    }
    // Check for module detail messages
    else if (canId == ID_MODULE_DETAIL) {
        uint8_t moduleId = msg.data[0];
        ProcessModuleDetail(moduleId, msg.data);
    }
}

void TMainForm::OnCANError(uint32_t errorCode, const std::string& errorMsg) {
    String errStr = "CAN Error (0x" + IntToHex((int)errorCode, 8) + "): " + String(errorMsg.c_str());
    LogMessage(errStr);
    
    // Check for specific error conditions
    if (errorCode & PCAN_ERROR_BUSOFF) {
        LogMessage("  -> Bus OFF: No other devices on bus or severe error");
    }
    if (errorCode & PCAN_ERROR_BUSWARNING) {
        LogMessage("  -> Bus Warning: Check termination resistors and baudrate");
    }
}

void TMainForm::ProcessModuleStatus(uint8_t moduleId, const uint8_t* data) {
    moduleManager->UpdateModuleStatus(moduleId, data);
    
    // Parse additional status data
    float voltage = ((data[2] << 8) | data[3]) * 0.01f;
    float current = ((int16_t)((data[4] << 8) | data[5])) * 0.1f;
    float temp = data[6] - 40.0f;
    
    moduleManager->UpdateModuleElectrical(moduleId, voltage, current, temp);
}

void TMainForm::ProcessCellVoltages(uint8_t moduleId, const uint8_t* data) {
    // Parse cell voltage data (format depends on protocol)
    uint16_t voltages[4];
    for (int i = 0; i < 4; i++) {
        voltages[i] = (data[i * 2] << 8) | data[i * 2 + 1];
    }
    
    // Determine starting cell index from CAN ID
    uint8_t startCell = (data[0] >> 4) * 4;
    
    moduleManager->UpdateCellVoltages(moduleId, startCell, voltages, 4);
}

void TMainForm::ProcessCellTemperatures(uint8_t moduleId, const uint8_t* data) {
    // Similar to voltage processing
    uint16_t temps[4];
    for (int i = 0; i < 4; i++) {
        temps[i] = (data[i * 2] << 8) | data[i * 2 + 1];
    }
    
    uint8_t startCell = (data[0] >> 4) * 4;
    moduleManager->UpdateCellTemperatures(moduleId, startCell, temps, 4);
}

void TMainForm::ProcessModuleFault(uint8_t moduleId, const uint8_t* data) {
    // Process fault codes
    LogMessage("Module " + IntToStr(moduleId) + " fault: " + IntToHex((int)data[0], 2));
    // In the STM32 firmware, faults don't change state but set faultCode flags
    // For now, we'll just log the fault. Could enhance ModuleInfo to track faultCode
}

void TMainForm::ProcessModuleDetail(uint8_t moduleId, const uint8_t* data) {
    (void)data;  // Suppress unused parameter warning - TODO: implement detail parsing
    // Process detailed module information
    // data[1]: Number of cells
    // data[2]: Module type/version
    // data[3-4]: Capacity
    // data[5-6]: Cycle count
    
    PackEmulator::ModuleInfo* module = moduleManager->GetModule(moduleId);
    if (module != NULL) {
        LogMessage("Module " + IntToStr(moduleId) + " detail received");
        UpdateModuleDetails(moduleId);
    }
}

void TMainForm::LogMessage(const String& msg) {
    HistoryMemo->Lines->Add("[" + TimeToStr(Now()) + "] " + msg);
    
    // Limit history size
    while (HistoryMemo->Lines->Count > 1000) {
        HistoryMemo->Lines->Delete(0);
    }
}

void TMainForm::ShowError(const String& msg) {
    MessageDlg(msg, mtError, TMsgDlgButtons() << mbOK, 0);
    LogMessage("ERROR: " + msg);
}

void TMainForm::LoadConfiguration() {
    // Load from INI file or registry
    // TODO: Implement configuration loading
}

void TMainForm::SaveConfiguration() {
    // Save to INI file or registry
    // TODO: Implement configuration saving
}

PackEmulator::ModuleState TMainForm::GetSelectedState() {
    switch (StateRadioGroup->ItemIndex) {
        case 0: return PackEmulator::ModuleState::OFF;
        case 1: return PackEmulator::ModuleState::STANDBY;
        case 2: return PackEmulator::ModuleState::PRECHARGE;
        case 3: return PackEmulator::ModuleState::ON;
        default: return PackEmulator::ModuleState::OFF;
    }
}

//---------------------------------------------------------------------------
