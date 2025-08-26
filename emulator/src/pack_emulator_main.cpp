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
    // Initialize managers
    moduleManager = std::make_unique<PackEmulator::ModuleManager>();
    canInterface = std::make_unique<PackEmulator::CANInterface>();
    
    // Set up CAN callbacks
    canInterface->SetMessageCallback(
        [this](const PackEmulator::CANMessage& msg) { OnCANMessage(msg); });
    canInterface->SetErrorCallback(
        [this](uint32_t code, const std::string& error) { OnCANError(code, error); });
    
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
    if (isConnected) {
        DisconnectButtonClick(nullptr);
    }
    
    SaveConfiguration();
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::ConnectButtonClick(TObject *Sender) {
    // Get selected channel (default PCAN-USB)
    uint16_t channel = PCAN_USBBUS1;
    if (CANChannelCombo->ItemIndex > 0) {
        channel = PCAN_USBBUS1 + CANChannelCombo->ItemIndex;
    }
    
    // Get baudrate
    uint16_t baudrate = PCAN_BAUD_500K;  // Default 500K for battery modules
    
    // Connect to CAN
    if (canInterface->Connect(channel, baudrate)) {
        canInterface->StartReceiving();
        isConnected = true;
        UpdateConnectionStatus(true);
        LogMessage("Connected to CAN bus");
        
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
    moduleManager->StartDiscovery();
    LogMessage("Module discovery started");
    
    // Send announcement request
    uint8_t data[8] = {0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    canInterface->SendMessage(ID_PACK_ANNOUNCE_REQUEST, data, 8);
    
    DiscoverButton->Caption = "Stop Discovery";
    DiscoverButton->Tag = 1;  // Use tag to track state
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::RegisterButtonClick(TObject *Sender) {
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
    moduleManager->DeregisterAllModules();
    LogMessage("All modules deregistered");
    UpdateModuleList();
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::SetStateButtonClick(TObject *Sender) {
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
    PackEmulator::ModuleState state = GetSelectedState();
    moduleManager->SetAllModulesState(state);
    
    // Send to all registered modules
    auto moduleIds = moduleManager->GetRegisteredModuleIds();
    uint8_t stateCmd = static_cast<uint8_t>(state);
    
    for (uint8_t id : moduleIds) {
        canInterface->SendStateChange(id, stateCmd);
    }
    
    LogMessage("Set all modules to state " + IntToStr(stateCmd));
    UpdateModuleList();
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::ModuleListViewSelectItem(TObject *Sender, 
    TListItem *Item, bool Selected) {
    if (Selected && Item) {
        selectedModuleId = Item->Caption.ToInt();
        UpdateModuleDetails(selectedModuleId);
    }
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::UpdateTimerTimer(TObject *Sender) {
    if (!isConnected) {
        return;
    }
    
    // Update display
    UpdateStatusDisplay();
    
    // Update selected module details
    if (selectedModuleId) {
        UpdateModuleDetails(selectedModuleId);
    }
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::TimeoutTimerTimer(TObject *Sender) {
    moduleManager->CheckTimeouts();
    moduleManager->CheckForFaults();
    
    // Update module list to show timeout status
    UpdateModuleList();
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::DistributeKeysButtonClick(TObject *Sender) {
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
    if (SaveDialog->Execute()) {
        // Export module data to CSV
        // TODO: Implement CSV export
        LogMessage("Data exported to " + SaveDialog->FileName);
    }
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::ExitItemClick(TObject *Sender) {
    Close();
}

//---------------------------------------------------------------------------
// Private implementation functions
//---------------------------------------------------------------------------

void TMainForm::UpdateModuleList() {
    ModuleListView->Items->BeginUpdate();
    ModuleListView->Items->Clear();
    
    auto modules = moduleManager->GetAllModules();
    for (auto* module : modules) {
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
            case PackEmulator::ModuleState::FAULT: stateStr = "FAULT"; break;
            default: stateStr = "Unknown";
        }
        item->SubItems->Add(stateStr);
        
        item->SubItems->Add(FloatToStrF(module->voltage, ffFixed, 7, 2) + "V");
        item->SubItems->Add(FloatToStrF(module->soc, ffFixed, 7, 1) + "%");
    }
    
    ModuleListView->Items->EndUpdate();
}

void TMainForm::UpdateModuleDetails(uint8_t moduleId) {
    auto* module = moduleManager->GetModule(moduleId);
    if (!module) {
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
        IntToStr(moduleManager->GetModuleCount());
        
    // Update statistics
    auto stats = canInterface->GetStatistics();
    StatusBar->Panels->Items[3]->Text = "TX: " + IntToStr(stats.messagesSent) + 
                                        " RX: " + IntToStr(stats.messagesReceived);
}

void TMainForm::UpdateCellDisplay(uint8_t moduleId) {
    auto* module = moduleManager->GetModule(moduleId);
    if (!module) {
        return;
    }
    
    CellGrid->RowCount = module->cellVoltages.size() + 1;
    CellGrid->Cells[0][0] = "Cell";
    CellGrid->Cells[1][0] = "Voltage";
    CellGrid->Cells[2][0] = "Temperature";
    
    for (size_t i = 0; i < module->cellVoltages.size(); i++) {
        CellGrid->Cells[0][i + 1] = IntToStr(i + 1);
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
    uint32_t baseId = msg.id & 0x7FF;
    
    if (PackEmulator::IsModuleMessage(msg.id)) {
        uint8_t moduleId = PackEmulator::GetModuleIdFromCAN(msg.id);
        
        // Determine message type
        if (baseId >= ID_MODULE_STATUS_BASE && baseId < ID_MODULE_STATUS_BASE + 0x20) {
            ProcessModuleStatus(moduleId, msg.data);
        } else if (baseId >= ID_MODULE_CELL_VOLTAGE_BASE && baseId < ID_MODULE_CELL_VOLTAGE_BASE + 0x20) {
            ProcessCellVoltages(moduleId, msg.data);
        } else if (baseId >= ID_MODULE_CELL_TEMP_BASE && baseId < ID_MODULE_CELL_TEMP_BASE + 0x20) {
            ProcessCellTemperatures(moduleId, msg.data);
        } else if (baseId >= ID_MODULE_FAULT_BASE && baseId < ID_MODULE_FAULT_BASE + 0x20) {
            ProcessModuleFault(moduleId, msg.data);
        }
    }
}

void TMainForm::OnCANError(uint32_t errorCode, const std::string& errorMsg) {
    LogMessage("CAN Error: " + String(errorMsg.c_str()));
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
    moduleManager->SetModuleState(moduleId, PackEmulator::ModuleState::FAULT);
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