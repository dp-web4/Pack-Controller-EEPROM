//---------------------------------------------------------------------------
#ifndef PackEmulatorMainH
#define PackEmulatorMainH
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.Forms.hpp>
#include <Vcl.ExtCtrls.hpp>
#include <Vcl.ComCtrls.hpp>
#include <Vcl.Grids.hpp>
#include <Vcl.Menus.hpp>
#include <Vcl.Dialogs.hpp>
// TeeChart components - uncomment if available
// #include <VCLTee.Chart.hpp>
// #include <VCLTee.TeEngine.hpp>
// #include <VCLTee.TeeProcs.hpp>
// #include <VCLTee.Series.hpp>

#include "module_manager.h"
#include "can_interface.h"

//---------------------------------------------------------------------------
class TMainForm : public TForm, public PackEmulator::CANCallbackInterface
{
public:  // CANCallbackInterface implementation
    virtual void OnMessage(const PackEmulator::CANMessage& msg) { OnCANMessage(msg); }
    virtual void OnError(uint32_t errorCode, const std::string& errorMsg) { OnCANError(errorCode, errorMsg); }
    
__published:	// IDE-managed Components
    // Main panels
    TPanel *TopPanel;
    TPanel *LeftPanel;
    TPanel *CenterPanel;
    TPanel *BottomPanel;
    
    // Connection controls
    TGroupBox *ConnectionGroup;
    TComboBox *CANChannelCombo;
    TComboBox *BaudrateCombo;
    TButton *ConnectButton;
    TButton *DisconnectButton;
    TLabel *ConnectionStatusLabel;
    TLabel *HeartbeatLabel;
    
    // Module list
    TGroupBox *ModulesGroup;
    TListView *ModuleListView;
    TButton *DiscoverButton;
    TButton *RegisterButton;
    TButton *DeregisterButton;
    TButton *DeregisterAllButton;
    
    // Module control
    TGroupBox *ControlGroup;
    TRadioGroup *StateRadioGroup;
    TButton *SetStateButton;
    TButton *SetAllStatesButton;
    TCheckBox *EnableBalancingCheck;
    TEdit *BalancingMaskEdit;
    
    // Module details
    TPageControl *DetailsPageControl;
    TTabSheet *StatusTab;
    TTabSheet *CellsTab;
    TTabSheet *HistoryTab;
    TTabSheet *Web4Tab;
    
    // Status tab controls
    TStringGrid *StatusGrid;
    TLabel *VoltageLabel;
    TLabel *CurrentLabel;
    TLabel *TemperatureLabel;
    TLabel *SOCLabel;
    TLabel *SOHLabel;
    
    // Cells tab controls
    TStringGrid *CellGrid;
    // TChart *CellVoltageChart;  // Commented out - requires TeeChart component
    TCheckBox *AutoRangeCheck;
    
    // History tab controls
    TMemo *HistoryMemo;
    TButton *ClearHistoryButton;
    TButton *ExportHistoryButton;
    
    // Web4 tab controls
    TGroupBox *Web4Group;
    TEdit *DeviceKeyEdit;
    TEdit *LCTKeyEdit;
    TEdit *ComponentIdEdit;
    TButton *DistributeKeysButton;
    TCheckBox *EncryptionEnabledCheck;
    
    // Status bar
    TStatusBar *StatusBar;
    
    // Timers
    TTimer *UpdateTimer;
    TTimer *TimeoutTimer;
    TTimer *DiscoveryTimer;
    TTimer *PollTimer;
    
    // Menus
    TMainMenu *MainMenu;
    TMenuItem *FileMenu;
    TMenuItem *ToolsMenu;
    TMenuItem *HelpMenu;
    
    // File menu items
    TMenuItem *SaveConfigItem;
    TMenuItem *LoadConfigItem;
    TMenuItem *ExportDataItem;
    TMenuItem *ExitItem;
    
    // Tools menu items
    TMenuItem *LoggingItem;
    TMenuItem *SimulationItem;
    TMenuItem *DiagnosticsItem;
    
    // Dialogs
    TSaveDialog *SaveDialog;
    TOpenDialog *OpenDialog;
    
    // Event handlers
    void __fastcall ConnectButtonClick(TObject *Sender);
    void __fastcall DisconnectButtonClick(TObject *Sender);
    void __fastcall DiscoverButtonClick(TObject *Sender);
    void __fastcall RegisterButtonClick(TObject *Sender);
    void __fastcall DeregisterButtonClick(TObject *Sender);
    void __fastcall DeregisterAllButtonClick(TObject *Sender);
    void __fastcall SetStateButtonClick(TObject *Sender);
    void __fastcall SetAllStatesButtonClick(TObject *Sender);
    void __fastcall ModuleListViewSelectItem(TObject *Sender, TListItem *Item, bool Selected);
    void __fastcall UpdateTimerTimer(TObject *Sender);
    void __fastcall TimeoutTimerTimer(TObject *Sender);
    void __fastcall DiscoveryTimerTimer(TObject *Sender);
    void __fastcall PollTimerTimer(TObject *Sender);
    void __fastcall DistributeKeysButtonClick(TObject *Sender);
    void __fastcall ClearHistoryButtonClick(TObject *Sender);
    void __fastcall ExportHistoryButtonClick(TObject *Sender);
    void __fastcall ExportDataItemClick(TObject *Sender);
    void __fastcall ExitItemClick(TObject *Sender);
    void __fastcall FormCreate(TObject *Sender);
    void __fastcall FormDestroy(TObject *Sender);
    
private:	// User declarations
    PackEmulator::ModuleManager* moduleManager;
    PackEmulator::CANInterface* canInterface;
    
    bool isConnected;
    uint8_t selectedModuleId;
    uint8_t nextModuleToPoll;
    DWORD lastPollTime;
    
    // UI update functions
    void UpdateModuleList();
    void UpdateModuleDetails(uint8_t moduleId);
    void UpdateStatusDisplay();
    void UpdateCellDisplay(uint8_t moduleId);
    void UpdateConnectionStatus(bool connected);
    
    // CAN callbacks
    void OnCANMessage(const PackEmulator::CANMessage& msg);
    void OnCANError(uint32_t errorCode, const std::string& errorMsg);
    
    // Message processing
    void ProcessModuleStatus(uint8_t moduleId, const uint8_t* data);
    void ProcessCellVoltages(uint8_t moduleId, const uint8_t* data);
    void ProcessCellTemperatures(uint8_t moduleId, const uint8_t* data);
    void ProcessModuleFault(uint8_t moduleId, const uint8_t* data);
    void ProcessModuleDetail(uint8_t moduleId, const uint8_t* data);
    
    // Helper functions
    void LogMessage(const String& msg);
    void ShowError(const String& msg);
    void LoadConfiguration();
    void SaveConfiguration();
    PackEmulator::ModuleState GetSelectedState();
    void SendModuleDiscoveryRequest();
    void SendModuleStatusRequest(uint8_t moduleId);
    
public:		// User declarations
    __fastcall TMainForm(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TMainForm *MainForm;
//---------------------------------------------------------------------------
#endif