//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include "pack_emulator_main.h"
#include "../include/can_id_module.h"
#include "../include/can_id_bms_vcu.h"
#include <sstream>
#include <iomanip>
#include <windows.h>

// Windows message for scrolling edit controls
#ifndef EM_LINESCROLL
#define EM_LINESCROLL 0x00B6
#endif

//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TMainForm *MainForm;

//---------------------------------------------------------------------------
__fastcall TMainForm::TMainForm(TComponent* Owner)
    : TForm(Owner)
    , isConnected(false)
    , selectedModuleId(0)
    , nextModuleToPoll(0)
    , lastPollTime(0)
    , selectedState(PackEmulator::ModuleState::OFF)
    , pollingCellDetails(false)
    , nextCellToRequest(0)
    , lastCellRequestTime(0) {
    // Initialize message flags
    memset(&messageFlags, 0, sizeof(messageFlags));
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::FormCreate(TObject *Sender) {
    (void)Sender;  // Suppress unused parameter warning
    // Initialize managers
    moduleManager = new PackEmulator::ModuleManager();
    canInterface = new PackEmulator::CANInterface();
    
    // Set up CAN callback
    canInterface->SetCallback(this);
    
    // Connect button events (if not connected in form designer)
    if (ClearHistoryButton) {
        ClearHistoryButton->OnClick = ClearHistoryButtonClick;
    }
    if (ExportHistoryButton) {
        ExportHistoryButton->OnClick = ExportHistoryButtonClick;
    }
    
    // Initialize UI
    UpdateConnectionStatus(false);
    
    // Load configuration
    LoadConfiguration();
    
    // Set up timers
    UpdateTimer->Interval = 100;  // 10 Hz update
    UpdateTimer->Enabled = true;
    
    TimeoutTimer->Interval = 1000;  // 1 Hz timeout check
    TimeoutTimer->Enabled = true;
    
    // Create and configure message poll timer
    MessagePollTimer = new TTimer(this);
    MessagePollTimer->Interval = 10;  // 10ms for fast message processing
    MessagePollTimer->OnTimer = MessagePollTimerTimer;
    MessagePollTimer->Enabled = false;  // Will be enabled when connected
    
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
        
        // Start timers
        DiscoveryTimer->Enabled = true;
        PollTimer->Enabled = true;
        MessagePollTimer->Enabled = true;  // Start message processing
        
        // Send initial discovery request
        SendModuleDiscoveryRequest();
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
    
    // Update heartbeat indicator
    HeartbeatLabel->Caption = "Heartbeat: -";
    HeartbeatLabel->Font->Color = clGray;
    
    // Stop timers
    DiscoveryTimer->Enabled = false;
    PollTimer->Enabled = false;
    MessagePollTimer->Enabled = false;  // Stop message processing
    
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
        // IMPORTANT: For extended frames, the 11-bit message ID goes in bits 28-18
        uint32_t announceExtId = ((uint32_t)ID_MODULE_ANNOUNCE_REQUEST << 18) | 0;  // 0x51D -> 0x14740000
        if (canInterface->SendMessage(announceExtId, data, 8, true)) {
            LogMessage("-> 0x" + IntToHex((int)ID_MODULE_ANNOUNCE_REQUEST, 3) + 
                      " [Discovery Request] Broadcasting to all modules");
        } else {
            LogMessage("✗ Failed to send discovery request: " + String(canInterface->GetLastError().c_str()));
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
void __fastcall TMainForm::SetOffButtonClick(TObject *Sender) {
    (void)Sender;  // Suppress unused parameter warning
    if (!isConnected) {
        ShowError("Not connected to CAN bus");
        return;
    }
    if (!selectedModuleId) {
        ShowError("No module selected");
        return;
    }
    
    PackEmulator::ModuleState state = PackEmulator::ModuleState::OFF;
    selectedState = state;  // Update for Set All button
    
    // Track the commanded state (but don't change actual state)
    PackEmulator::ModuleInfo* module = moduleManager->GetModule(selectedModuleId);
    if (module != NULL) {
        module->commandedState = state;
    }
    
    // Queue state command using message flags
    messageFlags.stateChangeModuleId = selectedModuleId;
    messageFlags.stateChangeNewState = static_cast<uint8_t>(state);
    messageFlags.stateChange = true;
    
    LogMessage("Queueing state change for module " + IntToStr(selectedModuleId) + " to OFF");
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::SetStandbyButtonClick(TObject *Sender) {
    (void)Sender;  // Suppress unused parameter warning
    if (!isConnected) {
        ShowError("Not connected to CAN bus");
        return;
    }
    if (!selectedModuleId) {
        ShowError("No module selected");
        return;
    }
    
    PackEmulator::ModuleState state = PackEmulator::ModuleState::STANDBY;
    selectedState = state;  // Update for Set All button
    
    // Track the commanded state (but don't change actual state)
    PackEmulator::ModuleInfo* module = moduleManager->GetModule(selectedModuleId);
    if (module != NULL) {
        module->commandedState = state;
    }
    
    // Queue state command using message flags
    messageFlags.stateChangeModuleId = selectedModuleId;
    messageFlags.stateChangeNewState = static_cast<uint8_t>(state);
    messageFlags.stateChange = true;
    
    LogMessage("Queueing state change for module " + IntToStr(selectedModuleId) + " to STANDBY");
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::SetPrechargeButtonClick(TObject *Sender) {
    (void)Sender;  // Suppress unused parameter warning
    if (!isConnected) {
        ShowError("Not connected to CAN bus");
        return;
    }
    if (!selectedModuleId) {
        ShowError("No module selected");
        return;
    }
    
    PackEmulator::ModuleState state = PackEmulator::ModuleState::PRECHARGE;
    selectedState = state;  // Update for Set All button
    
    // Track the commanded state (but don't change actual state)
    PackEmulator::ModuleInfo* module = moduleManager->GetModule(selectedModuleId);
    if (module != NULL) {
        module->commandedState = state;
    }
    
    // Queue state command using message flags
    messageFlags.stateChangeModuleId = selectedModuleId;
    messageFlags.stateChangeNewState = static_cast<uint8_t>(state);
    messageFlags.stateChange = true;
    
    LogMessage("Queueing state change for module " + IntToStr(selectedModuleId) + " to PRECHARGE");
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::SetOnButtonClick(TObject *Sender) {
    (void)Sender;  // Suppress unused parameter warning
    if (!isConnected) {
        ShowError("Not connected to CAN bus");
        return;
    }
    if (!selectedModuleId) {
        ShowError("No module selected");
        return;
    }
    
    PackEmulator::ModuleState state = PackEmulator::ModuleState::ON;
    selectedState = state;  // Update for Set All button
    
    // Track the commanded state (but don't change actual state)
    PackEmulator::ModuleInfo* module = moduleManager->GetModule(selectedModuleId);
    if (module != NULL) {
        module->commandedState = state;
    }
    
    // Queue state command using message flags
    messageFlags.stateChangeModuleId = selectedModuleId;
    messageFlags.stateChangeNewState = static_cast<uint8_t>(state);
    messageFlags.stateChange = true;
    
    LogMessage("Queueing state change for module " + IntToStr(selectedModuleId) + " to ON");
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::SetAllStatesButtonClick(TObject *Sender) {
    (void)Sender;  // Suppress unused parameter warning
    if (!isConnected) {
        ShowError("Not connected to CAN bus");
        return;
    }
    PackEmulator::ModuleState state = selectedState;  // Use the last selected state
    
    // Track the commanded state for all modules (but don't change actual state)
    std::vector<uint8_t> moduleIds = moduleManager->GetRegisteredModuleIds();
    uint8_t stateCmd = static_cast<uint8_t>(state);
    
    for (size_t i = 0; i < moduleIds.size(); i++) {
        uint8_t id = moduleIds[i];
        PackEmulator::ModuleInfo* module = moduleManager->GetModule(id);
        if (module != NULL) {
            module->commandedState = state;
        }
    }
    
    // Use broadcast (module ID 0) to send to all modules at once
    // This is more efficient than queuing individual commands
    messageFlags.stateChangeModuleId = 0;  // 0 = broadcast to all
    messageFlags.stateChangeNewState = stateCmd;
    messageFlags.stateChange = true;
    
    LogMessage("Queueing broadcast state change to all modules to state " + IntToStr(stateCmd));
    // Don't update display yet - wait for module responses
    // UpdateModuleList();
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::ModuleListViewSelectItem(TObject *Sender, 
    TListItem *Item, bool Selected) {
    (void)Sender;  // Suppress unused parameter warning
    if (Selected && Item) {
        // Extract module ID from "[ID]" format
        String caption = Item->Caption;
        caption = caption.SubString(2, caption.Length() - 2);  // Remove [ and ]
        selectedModuleId = caption.ToInt();
        LogMessage("Selected module " + IntToStr(selectedModuleId));
        UpdateModuleDetails(selectedModuleId);
        
        // If Cells tab is already selected, restart polling with new module
        if (DetailsPageControl->ActivePage == CellsTab && isConnected) {
            pollingCellDetails = true;
            nextCellToRequest = 0;
            CellPollTimer->Enabled = true;
            // Reset the static flag in CellPollTimerTimer
            LogMessage("Restarted cell polling for newly selected module " + IntToStr(selectedModuleId));
        }
    } else if (!Selected && Item) {
        // Handle deselection - extract module ID from "[ID]" format
        String caption = Item->Caption;
        caption = caption.SubString(2, caption.Length() - 2);  // Remove [ and ]
        int moduleId = caption.ToInt();
        if (selectedModuleId == moduleId) {
            selectedModuleId = 0;
            LogMessage("Module deselected");
        }
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
    
    // Update module list periodically to show message counts and status changes
    static int listUpdateCounter = 0;
    listUpdateCounter++;
    if (listUpdateCounter >= 10) {  // Every 1 second (100ms timer * 10)
        listUpdateCounter = 0;
        UpdateModuleList();
    }
    
    // Set flags for heartbeat and time sync
    static int heartbeatCounter = 0;
    static int timeSyncCounter = 0;
    
    heartbeatCounter++;
    timeSyncCounter++;
    
    // Set heartbeat flag every 500ms (100ms timer * 5)
    if (heartbeatCounter >= 5) {
        heartbeatCounter = 0;
        messageFlags.heartbeat = true;
    }
    
    // Set time sync flag every 5 seconds (100ms timer * 50)
    if (timeSyncCounter >= 50) {
        timeSyncCounter = 0;
        messageFlags.timeSync = true;
    }
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::TimeoutTimerTimer(TObject *Sender) {
    (void)Sender;  // Suppress unused parameter warning
    
    // Store count before timeout check
    size_t beforeCount = moduleManager->GetRegisteredModuleIds().size();
    
    moduleManager->CheckTimeouts();
    moduleManager->CheckForFaults();
    
    // Check if any modules timed out
    size_t afterCount = moduleManager->GetRegisteredModuleIds().size();
    if (beforeCount > 0 && afterCount == 0) {
        LogMessage("All modules timed out - stopping broadcasts");
    }
    
    // Update module list to show timeout status
    UpdateModuleList();
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::ClearHistoryButtonClick(TObject *Sender) {
    (void)Sender;  // Suppress unused parameter warning
    HistoryMemo->Lines->Clear();
    LogMessage("History cleared");
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::ExportHistoryButtonClick(TObject *Sender) {
    (void)Sender;  // Suppress unused parameter warning
    if (SaveDialog->Execute()) {
        try {
            HistoryMemo->Lines->SaveToFile(SaveDialog->FileName);
            LogMessage("History exported to " + SaveDialog->FileName);
        } catch (...) {
            ShowError("Failed to export history");
        }
    }
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
    // Temporarily disable the selection event handler to prevent re-triggering
    // when we programmatically select an item
    TLVSelectItemEvent savedHandler = ModuleListView->OnSelectItem;
    ModuleListView->OnSelectItem = NULL;
    
    ModuleListView->Items->BeginUpdate();
    ModuleListView->Items->Clear();
    
    std::vector<PackEmulator::ModuleInfo*> modules = moduleManager->GetAllModules();
    for (size_t i = 0; i < modules.size(); i++) {
        PackEmulator::ModuleInfo* module = modules[i];
        TListItem* item = ModuleListView->Items->Add();
        // Format module ID with brackets to make it look button-like
        item->Caption = "[" + IntToStr(module->moduleId) + "]";
        
        // Add unique ID column
        String uniqueIdStr = "0x" + IntToHex((int)module->uniqueId, 8);
        item->SubItems->Add(uniqueIdStr);
        
        // Use compact status indicators
        item->SubItems->Add(module->isRegistered ? "REG" : "---");
        item->SubItems->Add(module->isResponding ? "OK" : "T/O");
        
        String stateStr;
        TColor stateColor = clBlack;
        switch (module->state) {
            case PackEmulator::ModuleState::OFF: 
                stateStr = "Off"; 
                stateColor = clGray;
                break;
            case PackEmulator::ModuleState::STANDBY: 
                stateStr = "Standby"; 
                stateColor = clBlue;
                break;
            case PackEmulator::ModuleState::PRECHARGE: 
                stateStr = "Precharge"; 
                stateColor = clOlive;
                break;
            case PackEmulator::ModuleState::ON: 
                stateStr = "On"; 
                stateColor = clGreen;
                break;
            default: 
                stateStr = "Unknown";
                stateColor = clRed;
        }
        item->SubItems->Add(stateStr);
        
        // Store color info in Data pointer for custom drawing
        item->Data = (void*)stateColor;
        
        // Add voltage with color coding for abnormal values
        float voltage = module->voltage;
        String voltageStr = FloatToStrF(voltage, ffFixed, 7, 2) + "V";
        item->SubItems->Add(voltageStr);
        
        // Add SOC with color coding
        float soc = module->soc;
        String socStr = FloatToStrF(soc, ffFixed, 7, 1) + "%";
        item->SubItems->Add(socStr);
        
        // Add cell count (min/max/expected)
        if (module->cellCount > 0 || module->cellCountMin > 0 || module->cellCountMax > 0) {
            String cellStr = IntToStr(module->cellCountMin) + "/" + 
                           IntToStr(module->cellCountMax) + "/" +
                           IntToStr(module->cellCount);
            item->SubItems->Add(cellStr);
        } else {
            item->SubItems->Add("-/-/-");
        }
        
        // Add message count to debug if we're receiving data
        item->SubItems->Add(IntToStr((int)module->messageCount));
        
        // Color the entire row based on module status
        if (!module->isResponding) {
            item->ImageIndex = -1;  // Could use for icon if we had an ImageList
        }
        
        // Select the item if it matches the currently selected module
        if (module->moduleId == selectedModuleId) {
            item->Selected = true;
        }
    }
    
    ModuleListView->Items->EndUpdate();
    
    // Restore the selection event handler
    ModuleListView->OnSelectItem = savedHandler;
}

void TMainForm::UpdateModuleDetails(uint8_t moduleId) {
    PackEmulator::ModuleInfo* module = moduleManager->GetModule(moduleId);
    if (module == NULL) {
        return;
    }
    
    // Update window title with selected module info
    Caption = "Pack Controller Emulator - Module " + IntToStr(moduleId) + 
              " (0x" + IntToHex((int)module->uniqueId, 8) + ")";
    
    // Update status labels
    VoltageLabel->Caption = "Voltage: " + FloatToStrF(module->voltage, ffFixed, 7, 2) + " V";
    CurrentLabel->Caption = "Current: " + FloatToStrF(module->current, ffFixed, 7, 2) + " A";
    TemperatureLabel->Caption = "Temperature: " + FloatToStrF(module->temperature, ffFixed, 7, 1) + " °C";
    SOCLabel->Caption = "SOC: " + FloatToStrF(module->soc, ffFixed, 7, 1) + " %";
    SOHLabel->Caption = "SOH: " + FloatToStrF(module->soh, ffFixed, 7, 1) + " %";
    
    // Update StatusGrid with additional module information
    StatusGrid->RowCount = 13;  // Increase rows for more data
    StatusGrid->Cells[0][0] = "Property";
    StatusGrid->Cells[1][0] = "Value";
    
    StatusGrid->Cells[0][1] = "Module ID";
    StatusGrid->Cells[1][1] = IntToStr(module->moduleId);
    
    StatusGrid->Cells[0][2] = "State";
    String stateStr;
    switch(module->state) {
        case PackEmulator::ModuleState::OFF: stateStr = "OFF"; break;
        case PackEmulator::ModuleState::STANDBY: stateStr = "STANDBY"; break;
        case PackEmulator::ModuleState::PRECHARGE: stateStr = "PRECHARGE"; break;
        case PackEmulator::ModuleState::ON: stateStr = "ON"; break;
        default: stateStr = "UNKNOWN"; break;
    }
    // Show commanded state if different from actual
    if (module->commandedState != module->state) {
        String cmdStr;
        switch(module->commandedState) {
            case PackEmulator::ModuleState::OFF: cmdStr = "OFF"; break;
            case PackEmulator::ModuleState::STANDBY: cmdStr = "STANDBY"; break;
            case PackEmulator::ModuleState::PRECHARGE: cmdStr = "PRECHARGE"; break;
            case PackEmulator::ModuleState::ON: cmdStr = "ON"; break;
            default: cmdStr = "UNKNOWN"; break;
        }
        StatusGrid->Cells[1][2] = stateStr + " (Cmd: " + cmdStr + ")";
    } else {
        StatusGrid->Cells[1][2] = stateStr;
    }
    
    StatusGrid->Cells[0][3] = "Min Cell V";
    StatusGrid->Cells[1][3] = FloatToStrF(module->minCellVoltage, ffFixed, 7, 3) + " V";
    
    StatusGrid->Cells[0][4] = "Max Cell V";
    StatusGrid->Cells[1][4] = FloatToStrF(module->maxCellVoltage, ffFixed, 7, 3) + " V";
    
    StatusGrid->Cells[0][5] = "Avg Cell V";
    StatusGrid->Cells[1][5] = FloatToStrF(module->avgCellVoltage, ffFixed, 7, 3) + " V";
    
    StatusGrid->Cells[0][6] = "Total Cell V";
    StatusGrid->Cells[1][6] = FloatToStrF(module->totalCellVoltage, ffFixed, 7, 2) + " V";
    
    StatusGrid->Cells[0][7] = "Min Temp";
    StatusGrid->Cells[1][7] = FloatToStrF(module->minCellTemp, ffFixed, 7, 1) + " °C";
    
    StatusGrid->Cells[0][8] = "Max Temp";
    StatusGrid->Cells[1][8] = FloatToStrF(module->maxCellTemp, ffFixed, 7, 1) + " °C";
    
    StatusGrid->Cells[0][9] = "Avg Temp";
    StatusGrid->Cells[1][9] = FloatToStrF(module->avgCellTemp, ffFixed, 7, 1) + " °C";
    
    StatusGrid->Cells[0][10] = "Max Charge I";
    StatusGrid->Cells[1][10] = FloatToStrF(module->maxChargeCurrent, ffFixed, 7, 1) + " A";
    
    StatusGrid->Cells[0][11] = "Max Discharge I";
    StatusGrid->Cells[1][11] = FloatToStrF(module->maxDischargeCurrent, ffFixed, 7, 1) + " A";
    
    StatusGrid->Cells[0][12] = "Cell Count";
    if (module->cellCountMin > 0 || module->cellCountMax > 0) {
        StatusGrid->Cells[1][12] = "Exp:" + IntToStr(module->cellCount) + 
                                   " Min:" + IntToStr(module->cellCountMin) +
                                   " Max:" + IntToStr(module->cellCountMax);
    } else {
        StatusGrid->Cells[1][12] = "Exp:" + IntToStr(module->cellCount) + " (No comm data)";
    }
    
    // Update cell display
    UpdateCellDisplay(moduleId);
}

void TMainForm::UpdateStatusDisplay() {
    // Update pack-level values with better formatting
    float packVoltage = moduleManager->GetPackVoltage();
    float packCurrent = moduleManager->GetPackCurrent();
    size_t moduleCount = moduleManager->GetModuleCount();
    size_t registeredCount = moduleManager->GetRegisteredModuleIds().size();
    
    StatusBar->Panels->Items[0]->Text = "Pack: " + 
        FloatToStrF(packVoltage, ffFixed, 7, 1) + "V";
    
    // Show current with direction indicator
    String currentStr = "Current: ";
    if (packCurrent > 0.1f) {
        currentStr += "-> " + FloatToStrF(packCurrent, ffFixed, 7, 1) + "A";  // Discharge
    } else if (packCurrent < -0.1f) {
        currentStr += "<- " + FloatToStrF(-packCurrent, ffFixed, 7, 1) + "A";  // Charge
    } else {
        currentStr += FloatToStrF(packCurrent, ffFixed, 7, 1) + "A";
    }
    StatusBar->Panels->Items[1]->Text = currentStr;
    
    // Show registered vs discovered modules
    StatusBar->Panels->Items[2]->Text = "Modules: " + 
        IntToStr((int)registeredCount) + "/" + IntToStr((int)moduleCount);
        
    // Update statistics with rate calculation (now using separate panels)
    PackEmulator::CANInterface::Statistics stats = canInterface->GetStatistics();
    static uint32_t lastTxCount = 0;
    static uint32_t lastRxCount = 0;
    static DWORD lastUpdateTime = 0;
    
    DWORD currentTime = GetTickCount();
    if (lastUpdateTime != 0 && currentTime > lastUpdateTime) {
        float deltaTime = (currentTime - lastUpdateTime) / 1000.0f;
        if (deltaTime > 0) {
            uint32_t txDelta = stats.messagesSent - lastTxCount;
            uint32_t rxDelta = stats.messagesReceived - lastRxCount;
            float txRate = txDelta / deltaTime;
            float rxRate = rxDelta / deltaTime;
            
            // Panel 3: TX stats (fixed width formatting)
            StatusBar->Panels->Items[3]->Text = 
                "TX: " + IntToStr((int)stats.messagesSent) + 
                " (" + FloatToStrF(txRate, ffFixed, 4, 1) + "/s)";
            
            // Panel 4: RX stats (separate panel, no jumping)
            StatusBar->Panels->Items[4]->Text = 
                "RX: " + IntToStr((int)stats.messagesReceived) + 
                " (" + FloatToStrF(rxRate, ffFixed, 4, 1) + "/s)";
        }
    } else {
        // Initial display
        StatusBar->Panels->Items[3]->Text = "TX: " + IntToStr((int)stats.messagesSent) + " (0.0/s)";
        StatusBar->Panels->Items[4]->Text = "RX: " + IntToStr((int)stats.messagesReceived) + " (0.0/s)";
    }
    
    lastTxCount = stats.messagesSent;
    lastRxCount = stats.messagesReceived;
    lastUpdateTime = currentTime;
}

void TMainForm::UpdateCellDisplay(uint8_t moduleId) {
    PackEmulator::ModuleInfo* module = moduleManager->GetModule(moduleId);
    if (module == NULL) {
        return;
    }
    
    // Use expected cell count if available, otherwise use actual array size
    int cellCount = module->cellCount > 0 ? module->cellCount : module->cellVoltages.size();
    
    // Ensure arrays are sized correctly
    if (module->cellVoltages.size() < cellCount) {
        module->cellVoltages.resize(cellCount, 0.0f);
        module->cellTemperatures.resize(cellCount, 0.0f);
    }
    
    CellGrid->RowCount = cellCount + 1;
    CellGrid->Cells[0][0] = "Cell";
    CellGrid->Cells[1][0] = "Voltage (V)";
    CellGrid->Cells[2][0] = "Temp (°C)";
    
    for (int i = 0; i < cellCount; i++) {
        CellGrid->Cells[0][i + 1] = IntToStr(i + 1);
        
        // Show 0.000 for missing cells (not communicating)
        if (module->cellVoltages[i] == 0.0f) {
            CellGrid->Cells[1][i + 1] = "0.000";
            CellGrid->Cells[2][i + 1] = "0.0";
        } else {
            CellGrid->Cells[1][i + 1] = FloatToStrF(module->cellVoltages[i], ffFixed, 7, 3);
            CellGrid->Cells[2][i + 1] = FloatToStrF(module->cellTemperatures[i], ffFixed, 7, 1);
        }
    }
}

void TMainForm::UpdateConnectionStatus(bool connected) {
    if (connected) {
        ConnectionStatusLabel->Caption = "● Connected";
        ConnectionStatusLabel->Font->Color = clGreen;
        ConnectionStatusLabel->Font->Style = TFontStyles() << fsBold;
        
        // Update window title if no module selected
        if (selectedModuleId == 0) {
            Caption = "Pack Controller Emulator - Connected";
        }
    } else {
        ConnectionStatusLabel->Caption = "○ Disconnected";
        ConnectionStatusLabel->Font->Color = clRed;
        ConnectionStatusLabel->Font->Style = TFontStyles();
        
        // Update window title
        Caption = "Pack Controller Emulator - Disconnected";
        selectedModuleId = 0;  // Clear selection on disconnect
    }
}

void TMainForm::OnCANMessage(const PackEmulator::CANMessage& msg) {
    // Process message based on ID
    // For extended frames, the standard 11-bit ID is in bits 28-18
    // The module ID is in bits 17-0
    uint32_t canId;
    uint8_t moduleIdFromExtended = 0;
    
    if (msg.isExtended) {
        // Extended frame: extract standard ID from bits 28-18
        canId = (msg.id >> 18) & 0x7FF;
        // Module ID is in lower bits
        moduleIdFromExtended = msg.id & 0xFF;
    } else {
        // Standard frame: use the ID directly
        canId = msg.id & 0x7FF;
    }
    
    // Format data string with better spacing
    String dataStr;
    for (int i = 0; i < msg.length; i++) {
        if (i > 0 && i % 4 == 0) dataStr += "| ";  // Group bytes for readability
        dataStr += IntToHex((int)msg.data[i], 2) + " ";
    }
    
    // Add message description based on CAN ID
    String description;
    switch (canId) {
        case ID_MODULE_ANNOUNCEMENT:
        case 0x000:  // Module firmware bug
            description = " [Module Announce]";
            break;
        case ID_MODULE_STATUS_1:
            description = " [Status 1]";
            break;
        case ID_MODULE_STATUS_2:
            description = " [Status 2]";
            break;
        case ID_MODULE_STATUS_3:
            description = " [Status 3]";
            break;
        case ID_MODULE_DETAIL:
            description = " [Module Detail]";
            break;
        case ID_MODULE_CELL_COMM_STATUS1:
            description = " [Cell Comm Status]";
            break;
        case ID_MODULE_CELL_VOLTAGE:
            description = " [Cell Voltage]";
            break;
        case ID_MODULE_CELL_TEMP:
            description = " [Cell Temp]";
            break;
        default:
            description = "";
    }
    
    // Log with module ID for extended frames
    if (msg.isExtended && moduleIdFromExtended > 0) {
        LogMessage("<- 0x" + IntToHex((int)canId, 3) + " (M" + IntToStr(moduleIdFromExtended) + ")" + 
                   description + " [" + IntToStr(msg.length) + "] " + dataStr);
    } else {
        LogMessage("<- 0x" + IntToHex((int)canId, 3) + description + " [" + 
                   IntToStr(msg.length) + "] " + dataStr);
    }
    
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
                    
                    // IMPORTANT: For extended frames, the 11-bit message ID goes in bits 28-18
                // For registration, we include the module ID in the lower bits
                uint32_t regExtId = ((uint32_t)ID_MODULE_REGISTRATION << 18) | moduleId;  // 0x510 -> 0x14400000 + moduleId
                if (canInterface->SendMessage(regExtId, regData, 8, true)) {
                        LogMessage("-> 0x" + IntToHex((int)ID_MODULE_REGISTRATION, 3) + 
                                  " [Registration ACK] Assigned ID " + IntToStr(moduleId));
                    }
                    LogMessage("✓ Registered module ID " + IntToStr(moduleId) + 
                              " (Unique: 0x" + IntToHex((int)uniqueId, 8) + ")");
                    
                    // Debug: Show what we're comparing for heartbeat
                    LogMessage("  Module will filter: RegID=" + IntToStr(moduleId) + 
                              " vs Heartbeat byte[0]=" + IntToStr(3) + " (ON state)");
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
                // IMPORTANT: For extended frames, the 11-bit message ID goes in bits 28-18
                // For registration, we include the module ID in the lower bits
                uint32_t regExtId = ((uint32_t)ID_MODULE_REGISTRATION << 18) | moduleId;  // 0x510 -> 0x14400000 + moduleId
                if (canInterface->SendMessage(regExtId, regData, 8, true)) {
                    LogMessage("Re-sent registration ACK on CAN ID 0x" + 
                              IntToHex((int)ID_MODULE_REGISTRATION, 3));
                }
            }
        } else {
            LogMessage("ERROR: Could not assign module ID (all 32 slots full?)");
        }
    }
    // Check for module status messages
    else if (canId == ID_MODULE_STATUS_1) {
        // Use the module ID we already extracted from extended frame
        uint8_t moduleId = moduleIdFromExtended;
        LogMessage("<- 0x" + IntToHex((int)canId, 3) + " STATUS_1 from module " + IntToStr(moduleId));
        
        // Clear the pending flag since we got a response
        moduleManager->SetStatusPending(moduleId, false);
        ProcessModuleStatus1(moduleId, msg.data);
    }
    else if (canId == ID_MODULE_STATUS_2) {
        // Use the module ID we already extracted from extended frame
        uint8_t moduleId = moduleIdFromExtended;
        // Log only occasionally to avoid spam
        static int status2Count = 0;
        if (++status2Count % 10 == 0) {
            LogMessage("<- 0x" + IntToHex((int)canId, 3) + " STATUS_2 from module " + IntToStr(moduleId));
        }
        ProcessModuleStatus2(moduleId, msg.data);
    }
    else if (canId == ID_MODULE_STATUS_3) {
        // Use the module ID we already extracted from extended frame
        uint8_t moduleId = moduleIdFromExtended;
        // Log only occasionally to avoid spam
        static int status3Count = 0;
        if (++status3Count % 10 == 0) {
            LogMessage("<- 0x" + IntToHex((int)canId, 3) + " STATUS_3 from module " + IntToStr(moduleId));
        }
        ProcessModuleStatus3(moduleId, msg.data);
    }
    else if (canId == ID_MODULE_HARDWARE) {
        // Hardware capability message from module
        uint8_t moduleId = moduleIdFromExtended;
        LogMessage("<- 0x" + IntToHex((int)canId, 3) + " HARDWARE from module " + IntToStr(moduleId));
        ProcessModuleHardware(moduleId, msg.data);
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
        // Module ID comes from extended CAN ID, not from data
        uint8_t cellId = msg.data[0];
        LogMessage("<- 0x505 MODULE_DETAIL from module " + IntToStr(moduleIdFromExtended) + 
                  " for cell " + IntToStr(cellId));
        ProcessModuleDetail(moduleIdFromExtended, msg.data);
    }
    // Check for cell comm status messages
    else if (canId == ID_MODULE_CELL_COMM_STATUS1) {
        ProcessModuleCellCommStatus(moduleIdFromExtended, msg.data);
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

void TMainForm::ProcessModuleStatus1(uint8_t moduleId, const uint8_t* data) {
    // First update the module status (updates message count and timestamp)
    moduleManager->UpdateModuleStatus(moduleId, data);
    
    // Clear the waiting flag - we got a response
    PackEmulator::ModuleInfo* module = moduleManager->GetModule(moduleId);
    if (module != NULL) {
        module->waitingForStatusResponse = false;
    }
    
    // Parse MODULE_STATUS_1 according to can_frm_mod.h:
    // Byte 0: ModuleState (bits 0-3) and ModuleStatus (bits 4-7)
    // Byte 1: ModuleSOC (0.5% per bit)
    // Byte 2: ModuleSOH (0.5% per bit)
    // Byte 3: CellCount (direct value, no conversion needed)
    // Bytes 4-5: moduleMmc = ModuleMeasuredCurrent (16 bits, signed, 0.1A per bit)
    // Bytes 6-7: moduleMmv = ModuleMeasuredVoltage (16 bits, MODULE_VOLTAGE_FACTOR = 0.015V per bit)
    
    uint8_t stateAndStatus = data[0];
    uint8_t moduleState = stateAndStatus & 0x0F;  // Lower 4 bits
    uint8_t moduleStatus = (stateAndStatus >> 4) & 0x0F;  // Upper 4 bits
    uint8_t cellCount = data[3];  // Direct byte value, no increment
    
    // Current is in bytes 4-5 (moduleMmc field) - LITTLE ENDIAN
    // MODULE_CURRENT_BASE = -655.36A, MODULE_CURRENT_FACTOR = 0.02A
    // actual_current = MODULE_CURRENT_BASE + (raw * MODULE_CURRENT_FACTOR)
    uint16_t currentRaw = data[4] | (data[5] << 8);
    float current = -655.36f + (currentRaw * 0.02f);
    
    // Voltage is in bytes 6-7 (moduleMmv field) - LITTLE ENDIAN
    uint16_t voltageRaw = data[6] | (data[7] << 8);
    float voltage = voltageRaw * 0.015f;  // MODULE_VOLTAGE_FACTOR = 0.015V per bit
    
    float soc = data[1] * 0.5f;
    float soh = data[2] * 0.5f;
    
    // Log the actual data we received
    LogMessage("  State=" + IntToStr(moduleState) + " Status=" + IntToStr(moduleStatus) +
               " V=" + FloatToStrF(voltage, ffFixed, 7, 2) + "V" +
               " I=" + FloatToStrF(current, ffFixed, 7, 1) + "A" +
               " SOC=" + FloatToStrF(soc, ffFixed, 7, 1) + "%" +
               " Cells=" + IntToStr(cellCount));
    
    // Update module with parsed data
    // Reuse the module pointer we already have
    if (module != NULL) {
        // Update state
        switch(moduleState) {
            case 0: module->state = PackEmulator::ModuleState::OFF; break;
            case 1: module->state = PackEmulator::ModuleState::STANDBY; break;
            case 2: module->state = PackEmulator::ModuleState::PRECHARGE; break;
            case 3: module->state = PackEmulator::ModuleState::ON; break;
            default: module->state = PackEmulator::ModuleState::UNKNOWN; break;
        }
        
        module->soc = soc;
        module->soh = soh;
        module->voltage = voltage;
        module->current = current;
        module->cellCount = cellCount;  // Store expected cell count from STATUS_1
        module->isResponding = true;
        module->messageCount++;
        module->lastMessageTime = GetTickCount();  // Update timestamp to prevent timeout
        
        // Initialize cell arrays if cell count changed
        if (cellCount > 0 && module->cellVoltages.size() != cellCount) {
            module->cellVoltages.resize(cellCount, 0.0f);  // 0V for missing cells
            module->cellTemperatures.resize(cellCount, 0.0f);  // 0°C for missing cells
        }
        
        // Update the module list display to show new data
        UpdateModuleList();
    }
    
    // Temperature isn't in STATUS_1, it comes from STATUS_3
    // So we don't update temperature here
}

void TMainForm::ProcessCellVoltages(uint8_t moduleId, const uint8_t* data) {
    // Parse cell voltage data (format depends on protocol)
    uint16_t voltages[4];
    for (int i = 0; i < 4; i++) {
        // LITTLE ENDIAN byte order
        voltages[i] = data[i * 2] | (data[i * 2 + 1] << 8);
    }
    
    // Determine starting cell index from CAN ID
    uint8_t startCell = (data[0] >> 4) * 4;
    
    moduleManager->UpdateCellVoltages(moduleId, startCell, voltages, 4);
}

void TMainForm::ProcessCellTemperatures(uint8_t moduleId, const uint8_t* data) {
    // Similar to voltage processing
    uint16_t temps[4];
    for (int i = 0; i < 4; i++) {
        // LITTLE ENDIAN byte order
        temps[i] = data[i * 2] | (data[i * 2 + 1] << 8);
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
    // Parse MODULE_DETAIL (0x505) according to ModuleCPU code:
    // Byte 0: Cell ID
    // Byte 1: Expected cell count  
    // Bytes 2-3: Cell temperature (16 bits, 0.01°C per bit, -55.35°C offset)
    // Bytes 4-5: Cell voltage (16 bits, 0.001V per bit)
    // Byte 6: Cell SOC (0.5% per bit)
    // Byte 7: Cell SOH (0.5% per bit)
    
    PackEmulator::ModuleInfo* module = moduleManager->GetModule(moduleId);
    if (module != NULL) {
        // Clear the waiting flag - we got a response
        module->waitingForCellResponse = false;
        
        uint8_t cellId = data[0];
        uint8_t expectedCount = data[1];
        
        // Initialize arrays to expected size if needed (with default values for missing cells)
        if (module->cellVoltages.size() < expectedCount) {
            module->cellVoltages.resize(expectedCount, 0.0f);
            module->cellTemperatures.resize(expectedCount, 0.0f);
        }
        
        // Check if cell ID is valid
        if (cellId >= expectedCount) {
            LogMessage("Module " + IntToStr(moduleId) + " Cell " + IntToStr(cellId) + 
                      " out of range (expected " + IntToStr(expectedCount) + " cells)");
            return;
        }
        
        // Parse temperature (little-endian, with -55.35°C offset)
        uint16_t tempRaw = data[2] | (data[3] << 8);
        float cellTemp = (tempRaw * 0.01f) - 55.35f;
        
        // Parse voltage (little-endian, 0.001V per bit)
        uint16_t voltRaw = data[4] | (data[5] << 8);
        float cellVolt = voltRaw * 0.001f;
        
        // Parse SOC and SOH
        float cellSOC = data[6] * 0.5f;
        float cellSOH = data[7] * 0.5f;
        
        // Store the cell data
        module->cellVoltages[cellId] = cellVolt;
        module->cellTemperatures[cellId] = cellTemp;
        module->lastMessageTime = GetTickCount();  // Update timestamp to prevent timeout
        
        LogMessage("Module " + IntToStr(moduleId) + " Cell " + IntToStr(cellId) + 
                  ": " + FloatToStrF(cellVolt, ffFixed, 7, 3) + "V, " +
                  FloatToStrF(cellTemp, ffFixed, 7, 1) + "°C");
        
        // Update only the specific cell row in the display if this module is selected
        // This avoids the display glitch where values alternate between actual and zero
        if (moduleId == selectedModuleId && DetailsPageControl->ActivePage == CellsTab) {
            // Update just this cell's row in the grid (row = cellId + 1 due to header)
            int gridRow = cellId + 1;
            if (gridRow < CellGrid->RowCount) {
                if (cellVolt == 0.0f) {
                    CellGrid->Cells[1][gridRow] = "0.000";
                    CellGrid->Cells[2][gridRow] = "0.0";
                } else {
                    CellGrid->Cells[1][gridRow] = FloatToStrF(cellVolt, ffFixed, 7, 3);
                    CellGrid->Cells[2][gridRow] = FloatToStrF(cellTemp, ffFixed, 7, 1);
                }
            }
        }
    }
}

void TMainForm::ProcessModuleStatus2(uint8_t moduleId, const uint8_t* data) {
    // Clear the waiting flag - we got a response
    PackEmulator::ModuleInfo* module = moduleManager->GetModule(moduleId);
    if (module != NULL) {
        module->waitingForStatusResponse = false;
    }
    
    // Parse MODULE_STATUS_2 according to can_frm_mod.h:
    // Bytes 0-1: cellLoVolt (16 bits, 0.001V per bit)
    // Bytes 2-3: cellHiVolt (16 bits, 0.001V per bit)
    // Bytes 4-5: cellAvgVolt (16 bits, 0.001V per bit)
    // Bytes 6-7: cellTotalV (16 bits, 0.015V per bit per eeprom_data.h)
    
    // Reuse the module pointer we already have
    if (module != NULL) {
        // LITTLE ENDIAN byte order
        uint16_t loVolt = data[0] | (data[1] << 8);
        uint16_t hiVolt = data[2] | (data[3] << 8);
        uint16_t avgVolt = data[4] | (data[5] << 8);
        uint16_t totalVolt = data[6] | (data[7] << 8);
        
        // Store min/max/avg cell voltages using correct scaling factors
        module->minCellVoltage = loVolt * 0.001f;   // CELL_VOLTAGE_FACTOR
        module->maxCellVoltage = hiVolt * 0.001f;   // CELL_VOLTAGE_FACTOR
        module->avgCellVoltage = avgVolt * 0.001f;  // CELL_VOLTAGE_FACTOR
        module->totalCellVoltage = totalVolt * 0.015f; // CELL_TOTAL_VOLTAGE_FACTOR
        module->lastMessageTime = GetTickCount();  // Update timestamp to prevent timeout
        
        // Log only occasionally to avoid spam
        static int status2Count = 0;
        if (++status2Count % 10 == 0) {
            LogMessage("Module " + IntToStr(moduleId) + " STATUS_2: Min=" + 
                      FloatToStrF(module->minCellVoltage, ffFixed, 7, 3) + "V, Max=" +
                      FloatToStrF(module->maxCellVoltage, ffFixed, 7, 3) + "V");
        }
    }
}

void TMainForm::ProcessModuleStatus3(uint8_t moduleId, const uint8_t* data) {
    // Clear the waiting flag - we got a response
    PackEmulator::ModuleInfo* module = moduleManager->GetModule(moduleId);
    if (module != NULL) {
        module->waitingForStatusResponse = false;
    }
    
    // Parse MODULE_STATUS_3 according to can_frm_mod.h:
    // Bytes 0-1: cellLoTemp (16 bits, TEMPERATURE_FACTOR = 0.01°C per bit, -55.35°C offset)
    // Bytes 2-3: cellHiTemp (16 bits, TEMPERATURE_FACTOR = 0.01°C per bit, -55.35°C offset)
    // Bytes 4-5: cellAvgTemp (16 bits, TEMPERATURE_FACTOR = 0.01°C per bit, -55.35°C offset)
    // Bytes 6-7: UNUSED
    // Note: Temperature encoding: actual_celsius = (raw * 0.01) - 55.35
    
    // Reuse the module pointer we already have
    if (module != NULL) {
        // LITTLE ENDIAN byte order
        uint16_t loTemp = data[0] | (data[1] << 8);
        uint16_t hiTemp = data[2] | (data[3] << 8);
        uint16_t avgTemp = data[4] | (data[5] << 8);
        
        // Convert using TEMPERATURE_FACTOR (0.01°C per bit) with -55.35°C offset
        // Temperature encoding: actual_celsius = (raw * 0.01) - 55.35
        module->minCellTemp = (loTemp * 0.01f) - 55.35f;
        module->maxCellTemp = (hiTemp * 0.01f) - 55.35f;
        module->avgCellTemp = (avgTemp * 0.01f) - 55.35f;
        
        // Update the main temperature with average
        module->temperature = module->avgCellTemp;
        module->lastMessageTime = GetTickCount();  // Update timestamp to prevent timeout
        
        // Log only occasionally
        static int status3Count = 0;
        if (++status3Count % 10 == 0) {
            LogMessage("Module " + IntToStr(moduleId) + " STATUS_3: Temp Min=" + 
                      FloatToStrF(module->minCellTemp, ffFixed, 7, 1) + "°C, Max=" +
                      FloatToStrF(module->maxCellTemp, ffFixed, 7, 1) + "°C");
        }
    }
}

void TMainForm::ProcessModuleCellCommStatus(uint8_t moduleId, const uint8_t* data) {
    // Parse MODULE_CELL_COMM_STATUS1 according to ModuleCPU code:
    // Byte 0: cellCountMin (fewest cells received)
    // Byte 1: cellCountMax (most cells received)
    // Bytes 2-3: cellI2CErrors (16 bits, little-endian)
    // Byte 4: MCRXFramingErrors
    // Byte 5: FirstCellWithI2CError (0xFF = none)
    // Bytes 6-7: UNUSED
    
    PackEmulator::ModuleInfo* module = moduleManager->GetModule(moduleId);
    if (module != NULL) {
        module->cellCountMin = data[0];
        module->cellCountMax = data[1];
        module->cellI2CErrors = data[2] | (data[3] << 8);
        
        uint8_t mcRxErrors = data[4];
        uint8_t firstErrorCell = data[5];
        
        LogMessage("Module " + IntToStr(moduleId) + " CELL_COMM: Min=" + IntToStr(module->cellCountMin) +
                  " Max=" + IntToStr(module->cellCountMax) + " cells, Expected=" + IntToStr(module->cellCount) +
                  ", I2C Errors=" + IntToStr(module->cellI2CErrors));
        
        module->lastMessageTime = GetTickCount();  // Update timestamp to prevent timeout
        
        // Only update UI if this is the selected module
        if (moduleId == selectedModuleId) {
            UpdateModuleDetails(moduleId);
        }
    }
}

void TMainForm::ProcessModuleHardware(uint8_t moduleId, const uint8_t* data) {
    // Parse MODULE_HARDWARE according to can_frm_mod.h:
    // Bytes 0-1: maxChargeA (16 bits, 0.1A per bit)
    // Bytes 2-3: maxDischargeA (16 bits, 0.1A per bit)
    // Bytes 4-5: maxChargeEndV (16 bits, 0.01V per bit)
    // Bytes 6-7: hwVersion (16 bits)
    
    PackEmulator::ModuleInfo* module = moduleManager->GetModule(moduleId);
    if (module != NULL) {
        // LITTLE ENDIAN byte order
        uint16_t maxCharge = data[0] | (data[1] << 8);
        uint16_t maxDischarge = data[2] | (data[3] << 8);
        uint16_t maxVoltage = data[4] | (data[5] << 8);
        uint16_t hwVersion = data[6] | (data[7] << 8);
        
        // Currents use MODULE_CURRENT_BASE and MODULE_CURRENT_FACTOR
        module->maxChargeCurrent = -655.36f + (maxCharge * 0.02f);
        module->maxDischargeCurrent = -655.36f + (maxDischarge * 0.02f);
        module->maxChargeVoltage = maxVoltage * 0.01f;
        module->hardwareVersion = hwVersion;
        
        LogMessage("Module " + IntToStr(moduleId) + " HARDWARE: MaxChg=" + 
                  FloatToStrF(module->maxChargeCurrent, ffFixed, 7, 1) + "A, MaxDis=" +
                  FloatToStrF(module->maxDischargeCurrent, ffFixed, 7, 1) + "A, MaxV=" +
                  FloatToStrF(module->maxChargeVoltage, ffFixed, 7, 2) + "V, HW=0x" +
                  IntToHex(hwVersion, 4));
        
        module->lastMessageTime = GetTickCount();  // Update timestamp to prevent timeout
    }
}

void TMainForm::LogMessage(const String& msg) {
    HistoryMemo->Lines->Add("[" + TimeToStr(Now()) + "] " + msg);
    
    // Limit history size
    while (HistoryMemo->Lines->Count > 1000) {
        HistoryMemo->Lines->Delete(0);
    }
    
    // Auto-scroll to bottom to show most recent message
    // SendMessage scrolls the memo to the end
    SendMessage(HistoryMemo->Handle, EM_LINESCROLL, 0, HistoryMemo->Lines->Count);
    
    // Alternative method: Set selection to end
    HistoryMemo->SelStart = HistoryMemo->Text.Length();
    HistoryMemo->SelLength = 0;
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::DetailsPageControlChange(TObject *Sender) {
    (void)Sender;  // Suppress unused parameter warning
    
    // Check if Cells tab is selected
    if (DetailsPageControl->ActivePage == CellsTab) {
        LogMessage("Cells tab selected. Module ID: " + IntToStr(selectedModuleId) + 
                  ", Connected: " + String(isConnected ? "Yes" : "No"));
        
        // Start polling cell details if we have a selected module
        if (selectedModuleId > 0 && isConnected) {
            pollingCellDetails = true;
            nextCellToRequest = 0;
            CellPollTimer->Enabled = true;
            LogMessage("Started polling cell details for module " + IntToStr(selectedModuleId));
        } else {
            LogMessage("Cannot start polling: Need to select a module and be connected");
        }
    } else {
        // Stop polling cell details when leaving Cells tab
        if (pollingCellDetails) {
            pollingCellDetails = false;
            CellPollTimer->Enabled = false;
            LogMessage("Stopped polling cell details");
        }
    }
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::CellPollTimerTimer(TObject *Sender) {
    (void)Sender;  // Suppress unused parameter warning
    
    // Check if we should be polling
    if (!pollingCellDetails || !isConnected || selectedModuleId == 0) {
        CellPollTimer->Enabled = false;
        return;
    }
    
    // Get module info to know how many cells to request
    PackEmulator::ModuleInfo* module = moduleManager->GetModule(selectedModuleId);
    if (module == NULL) {
        return;
    }
    
    // Use expected cell count if available, otherwise use what we've seen
    uint8_t cellCount = module->cellCount;
    if (cellCount == 0) {
        cellCount = module->cellCountMax;
    }
    if (cellCount == 0) {
        return;  // No cells to poll
    }
    
    // Set cell detail request flag with module and cell IDs
    messageFlags.cellDetail = true;
    messageFlags.cellModuleId = selectedModuleId;
    messageFlags.cellId = nextCellToRequest;
    
    // DON'T INCREMENT HERE - SendCellDetailRequest will do it after successful send
    // This prevents skipping cells when requests are delayed
    
    lastCellRequestTime = GetTickCount();
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


void TMainForm::SendModuleDiscoveryRequest() {
    if (!isConnected) return;
    
    // Send module announcement request (0x51D)
    // This is a broadcast to all modules asking them to announce themselves
    uint8_t data[8] = {0};
    data[0] = 0x00;  // Request code (0x00 = request announcement)
    
    // For extended frames, the 11-bit message ID goes in bits 28-18
    uint32_t extendedId = ((uint32_t)ID_MODULE_ANNOUNCE_REQUEST << 18) | 0;
    
    if (canInterface->SendMessage(extendedId, data, 1, true)) {
        LogMessage("Sent module discovery request (0x51D)");
    }
}

void TMainForm::SendModuleStatusRequest(uint8_t moduleId) {
    if (!isConnected) return;
    
    // Send status request to specific module (0x512)
    uint8_t data[8] = {0};
    data[0] = moduleId;  // Module ID to request status from
    
    // For extended frames targeting a specific module
    uint32_t extendedId = ((uint32_t)ID_MODULE_STATUS_REQUEST << 18) | moduleId;
    
    if (canInterface->SendMessage(extendedId, data, 1, true)) {
        // Log status requests to debug polling
        LogMessage("→ 0x512 [Status Request] to module " + IntToStr(moduleId));
    }
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::DiscoveryTimerTimer(TObject *Sender) {
    (void)Sender;
    
    // Set discovery flag every 5 seconds
    messageFlags.discovery = true;
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::PollTimerTimer(TObject *Sender) {
    (void)Sender;
    
    if (!isConnected) return;
    
    // Don't send status requests while polling cell details
    if (pollingCellDetails) {
        return;
    }
    
    // Get registered modules
    std::vector<uint8_t> moduleIds = moduleManager->GetRegisteredModuleIds();
    if (moduleIds.empty()) return;
    
    // Simple round-robin polling - poll one module per timer tick
    if (nextModuleToPoll >= moduleIds.size()) {
        nextModuleToPoll = 0;
    }
    
    // Set status request flag with module ID
    messageFlags.statusRequest = true;
    messageFlags.statusModuleId = moduleIds[nextModuleToPoll];
    
    // Move to next module for next poll
    nextModuleToPoll++;
}

//---------------------------------------------------------------------------
// Message Queue Processing Functions
//---------------------------------------------------------------------------

void __fastcall TMainForm::MessagePollTimerTimer(TObject *Sender) {
    (void)Sender;
    
    // Process all pending messages in priority order
    ProcessMessageQueue();
}

void TMainForm::ProcessMessageQueue() {
    if (!isConnected) {
        return;
    }
    
    // Process messages in priority order
    // Priority 1: State changes (SAFETY-CRITICAL - highest priority)
    if (messageFlags.stateChange) {
        LogMessage("Processing queued state change message");
        messageFlags.stateChange = false;
        SendStateChangeMessage();
        return; // Process one message per call for better timing
    }
    
    // Priority 2: Heartbeat (important for connection)
    if (messageFlags.heartbeat) {
        messageFlags.heartbeat = false;
        SendHeartbeatMessage();
        return;
    }
    
    // Priority 3: Cell Detail requests
    if (messageFlags.cellDetail) {
        messageFlags.cellDetail = false;
        SendCellDetailRequest();
        return;
    }
    
    // Priority 4: Status requests
    if (messageFlags.statusRequest) {
        messageFlags.statusRequest = false;
        SendStatusRequest();
        return;
    }
    
    // Priority 5: Registration ACKs
    if (messageFlags.registration) {
        messageFlags.registration = false;
        SendRegistrationAck();
        return;
    }
    
    // Priority 6: Time sync
    if (messageFlags.timeSync) {
        messageFlags.timeSync = false;
        SendTimeSync();
        return;
    }
    
    // Priority 7: Discovery (lowest priority)
    if (messageFlags.discovery) {
        messageFlags.discovery = false;
        SendDiscoveryRequest();
        return;
    }
}

void TMainForm::SendHeartbeatMessage() {
    // Calculate the maximum state allowed based on all modules' commanded states
    uint8_t maxState = 0;  // Start with OFF
    
    std::vector<uint8_t> moduleIds = moduleManager->GetRegisteredModuleIds();
    for (size_t i = 0; i < moduleIds.size(); i++) {
        PackEmulator::ModuleInfo* module = moduleManager->GetModule(moduleIds[i]);
        if (module != NULL) {
            uint8_t cmdState = static_cast<uint8_t>(module->commandedState);
            if (cmdState > maxState) {
                maxState = cmdState;
            }
        }
    }
    
    // If no modules registered, default to OFF
    // The heartbeat should reflect the actual commanded state, not force a minimum
    
    // Send MODULE_MAX_STATE (0x517) - Heartbeat with highest allowed state
    uint8_t data[1] = {0};
    data[0] = maxState;
    
    uint32_t heartbeatExtId = ((uint32_t)ID_MODULE_MAX_STATE << 18) | 0;
    if (canInterface->SendMessage(heartbeatExtId, data, 1, true)) {
        // Update heartbeat indicator
        String stateName;
        switch(maxState) {
            case 0: stateName = "OFF"; break;
            case 1: stateName = "STANDBY"; break;
            case 2: stateName = "PRECHARGE"; break;
            case 3: stateName = "ON"; break;
            default: stateName = "?"; break;
        }
        HeartbeatLabel->Caption = "Heartbeat: " + stateName;
        HeartbeatLabel->Font->Color = clGreen;
        
        static int logCounter = 0;
        if (++logCounter % 10 == 0) {  // Log every 10th heartbeat
            LogMessage("-> 0x" + IntToHex((int)ID_MODULE_MAX_STATE, 3) + 
                      " [Heartbeat] Max state: " + stateName);
        }
    }
}

void TMainForm::SendStateChangeMessage() {
    uint8_t moduleId = messageFlags.stateChangeModuleId;
    uint8_t newState = messageFlags.stateChangeNewState;
    
    String stateName;
    switch(newState) {
        case 0: stateName = "OFF"; break;
        case 1: stateName = "STANDBY"; break;
        case 2: stateName = "PRECHARGE"; break;
        case 3: stateName = "ON"; break;
        default: stateName = "UNKNOWN"; break;
    }
    
    LogMessage("Sending state change: Module " + IntToStr(moduleId) + " to " + stateName);
    
    // Send the state change command
    if (canInterface->SendStateChange(moduleId, newState)) {
        LogMessage("-> 0x514 [State Change] Module " + IntToStr(moduleId) + 
                  " to " + stateName + " - SUCCESS");
    } else {
        LogMessage("ERROR: Failed to send state change to Module " + IntToStr(moduleId));
    }
}

void TMainForm::SendCellDetailRequest() {
    uint8_t moduleId = messageFlags.cellModuleId;
    uint8_t cellId = messageFlags.cellId;
    
    // Check if module is still waiting for a previous cell response
    PackEmulator::ModuleInfo* module = moduleManager->GetModule(moduleId);
    if (module != NULL && module->waitingForCellResponse) {
        // Check for timeout (200ms - shorter for cell details)
        DWORD currentTime = GetTickCount();
        if (currentTime - module->cellRequestTime < 200) {
            // Still waiting, re-queue the same cell for retry
            messageFlags.cellDetail = true;  // Re-queue this request
            return;  // Don't increment cell number
        }
        // Timeout occurred, will send new request for same cell
        LogMessage("Module " + IntToStr(moduleId) + " cell " + IntToStr(cellId) + 
                  " response timeout (200ms), resending same cell");
    }
    
    // Send the detail request
    bool success = canInterface->SendDetailRequest(moduleId, cellId);
    
    if (success) {
        // Set waiting flag and timestamp
        if (module != NULL) {
            module->waitingForCellResponse = true;
            module->cellRequestTime = GetTickCount();
        }
        
        // Always log cell requests to verify sequencing
        LogMessage("→ 0x515 [Cell Detail Request] Module " + 
                  IntToStr(moduleId) + " Cell " + IntToStr(cellId) + " (sent)");
        
        // NOW increment to next cell since we successfully sent this one
        if (module != NULL) {
            uint8_t cellCount = module->cellCount;
            if (cellCount == 0) {
                cellCount = module->cellCountMax;
            }
            if (cellCount > 0) {
                nextCellToRequest++;
                if (nextCellToRequest >= cellCount) {
                    nextCellToRequest = 0;  // Wrap around to first cell
                }
            }
        }
    } else {
        static int errorLogCounter = 0;
        if (errorLogCounter++ < 10) {
            LogMessage("Failed to send cell detail request to Module " + 
                      IntToStr(moduleId) + " Cell " + IntToStr(cellId));
        }
        // Don't increment on failure - retry same cell next time
        messageFlags.cellDetail = true;  // Re-queue this request
    }
}

void TMainForm::SendStatusRequest() {
    uint8_t moduleId = messageFlags.statusModuleId;
    
    // Check if module is still waiting for a previous status response
    PackEmulator::ModuleInfo* module = moduleManager->GetModule(moduleId);
    if (module != NULL && module->waitingForStatusResponse) {
        // Check for timeout (500ms)
        DWORD currentTime = GetTickCount();
        if (currentTime - module->statusRequestTime < 500) {
            // Still waiting, don't send another request yet
            return;  // Keep the flag set to retry later
        }
        // Timeout occurred, will send new request
        static int timeoutLogCounter = 0;
        if (++timeoutLogCounter % 10 == 0) {
            LogMessage("Module " + IntToStr(moduleId) + " status response timeout, retrying");
        }
    }
    
    // Send MODULE_STATUS_REQUEST (0x512) to specific module
    uint8_t data[1] = {0};
    data[0] = 0x01;  // Request all status messages
    
    uint32_t extendedId = ((uint32_t)ID_MODULE_STATUS_REQUEST << 18) | moduleId;
    
    if (canInterface->SendMessage(extendedId, data, 1, true)) {
        // Set waiting flag and timestamp
        if (module != NULL) {
            module->waitingForStatusResponse = true;
            module->statusRequestTime = GetTickCount();
        }
        
        // Log occasionally to avoid spam
        static int statusLogCounter = 0;
        if (++statusLogCounter % 50 == 0) {
            LogMessage("→ 0x512 [Status Request] to module " + IntToStr(moduleId));
        }
    }
}

void TMainForm::SendRegistrationAck() {
    uint8_t moduleId = messageFlags.registrationModuleId;
    uint32_t uniqueId = messageFlags.registrationUniqueId;
    
    // Send MODULE_REGISTRATION (0x510) ACK
    uint8_t regData[8] = {0};
    regData[0] = 0x01;  // ACK
    regData[1] = moduleId;  // Assigned module ID
    
    // Copy unique ID (little endian)
    regData[4] = (uint8_t)(uniqueId & 0xFF);
    regData[5] = (uint8_t)((uniqueId >> 8) & 0xFF);
    regData[6] = (uint8_t)((uniqueId >> 16) & 0xFF);
    regData[7] = (uint8_t)((uniqueId >> 24) & 0xFF);
    
    uint32_t regExtId = ((uint32_t)ID_MODULE_REGISTRATION << 18) | moduleId;
    if (canInterface->SendMessage(regExtId, regData, 8, true)) {
        LogMessage("→ 0x510 [Registration ACK] Module " + IntToStr(moduleId) +
                  " ID: 0x" + IntToHex((int)uniqueId, 8));
    }
}

void TMainForm::SendTimeSync() {
    // Send MODULE_SET_TIME (0x516)
    uint8_t data[5] = {0};
    
    // Get current time
    SYSTEMTIME st;
    GetSystemTime(&st);
    
    // Pack time: YYMMDDHHSS (BCD format)
    data[0] = ((st.wYear - 2000) / 10) << 4 | ((st.wYear - 2000) % 10);
    data[1] = (st.wMonth / 10) << 4 | (st.wMonth % 10);
    data[2] = (st.wDay / 10) << 4 | (st.wDay % 10);
    data[3] = (st.wHour / 10) << 4 | (st.wHour % 10);
    data[4] = (st.wMinute / 10) << 4 | (st.wMinute % 10);
    
    uint32_t timeSyncExtId = ((uint32_t)ID_MODULE_SET_TIME << 18) | 0;
    if (canInterface->SendMessage(timeSyncExtId, data, 5, true)) {
        static int timeLogCounter = 0;
        if (++timeLogCounter % 10 == 0) {  // Log every 10th sync
            LogMessage("→ 0x516 [Time Sync] " + 
                      IntToStr(st.wYear) + "-" + 
                      IntToStr(st.wMonth) + "-" + 
                      IntToStr(st.wDay) + " " +
                      IntToStr(st.wHour) + ":" + 
                      IntToStr(st.wMinute));
        }
    }
}

void TMainForm::SendDiscoveryRequest() {
    // Send MODULE_ANNOUNCE_REQUEST (0x51D)
    uint8_t data[8] = {0};
    data[0] = 0x01;  // Request announcement from all modules
    
    uint32_t announceExtId = ((uint32_t)ID_MODULE_ANNOUNCE_REQUEST << 18) | 0;
    if (canInterface->SendMessage(announceExtId, data, 8, true)) {
        LogMessage("→ 0x51D [Discovery Request] Broadcasting to all modules");
    }
}

//---------------------------------------------------------------------------
