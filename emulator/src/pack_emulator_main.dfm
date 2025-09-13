object MainForm: TMainForm
  Left = 0
  Top = 0
  Caption = 'Pack Controller Emulator'
  ClientHeight = 600
  ClientWidth = 1100
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  OldCreateOrder = False
  Position = poScreenCenter
  OnCreate = FormCreate
  OnDestroy = FormDestroy
  PixelsPerInch = 96
  TextHeight = 13
  object TopPanel: TPanel
    Left = 0
    Top = 0
    Width = 1100
    Height = 45
    Align = alTop
    TabOrder = 0
    object ConnectionGroup: TGroupBox
      Left = 8
      Top = 4
      Width = 600
      Height = 37
      Caption = ' CAN Connection '
      TabOrder = 0
      object ConnectionStatusLabel: TLabel
        Left = 380
        Top = 15
        Width = 80
        Height = 13
        Caption = 'Disconnected'
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clRed
        Font.Height = -11
        Font.Name = 'Tahoma'
        Font.Style = []
        ParentFont = False
      end
      object HeartbeatLabel: TLabel
        Left = 475
        Top = 15
        Width = 115
        Height = 13
        Caption = 'Heartbeat: -'
        AutoSize = True
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clGray
        Font.Height = -11
        Font.Name = 'Tahoma'
        Font.Style = []
        ParentFont = False
      end
      object CANChannelCombo: TComboBox
        Left = 16
        Top = 12
        Width = 110
        Height = 21
        Style = csDropDownList
        ItemIndex = 0
        TabOrder = 0
        Text = 'PCAN-USB 1'
        Items.Strings = (
          'PCAN-USB 1'
          'PCAN-USB 2'
          'PCAN-USB 3'
          'PCAN-USB 4')
      end
      object BaudrateCombo: TComboBox
        Left = 130
        Top = 12
        Width = 90
        Height = 21
        Style = csDropDownList
        ItemIndex = 2
        TabOrder = 1
        Text = '500 kBit/s'
        Items.Strings = (
          '125 kBit/s'
          '250 kBit/s'
          '500 kBit/s'
          '1 MBit/s')
      end
      object ConnectButton: TButton
        Left = 225
        Top = 11
        Width = 65
        Height = 23
        Caption = 'Connect'
        TabOrder = 2
        OnClick = ConnectButtonClick
      end
      object DisconnectButton: TButton
        Left = 295
        Top = 11
        Width = 75
        Height = 23
        Caption = 'Disconnect'
        Enabled = False
        TabOrder = 3
        OnClick = DisconnectButtonClick
      end
    end
  end
  object LeftPanel: TPanel
    Left = 0
    Top = 45
    Width = 550
    Height = 536
    Align = alLeft
    TabOrder = 1
    object ModulesGroup: TGroupBox
      Left = 1
      Top = 1
      Width = 548
      Height = 490
      Align = alClient
      Caption = ' Battery Modules '
      TabOrder = 0
      object ModuleListView: TListView
        Left = 2
        Top = 15
        Width = 544
        Height = 432
        Align = alClient
        Columns = <
          item
            AutoSize = True
            Caption = 'ID'
            MinWidth = 30
          end
          item
            AutoSize = True
            Caption = 'Unique ID'
            MinWidth = 80
          end
          item
            AutoSize = True
            Caption = 'Status'
            MinWidth = 40
          end
          item
            AutoSize = True
            Caption = 'Comm'
            MinWidth = 35
          end
          item
            AutoSize = True
            Caption = 'State'
            MinWidth = 60
          end
          item
            AutoSize = True
            Caption = 'Voltage'
            MinWidth = 60
          end
          item
            AutoSize = True
            Caption = 'SOC'
            MinWidth = 45
          end
          item
            AutoSize = True
            Caption = 'Cells (Min/Max/Exp)'
            MinWidth = 90
          end
          item
            AutoSize = True
            Caption = 'Messages'
            MinWidth = 60
          end>
        GridLines = True
        HideSelection = False
        MultiSelect = False
        ReadOnly = True
        RowSelect = True
        TabOrder = 0
        ViewStyle = vsReport
        OnSelectItem = ModuleListViewSelectItem
      end
      object ButtonPanel: TPanel
        Left = 2
        Top = 447
        Width = 544
        Height = 41
        Align = alBottom
        BevelOuter = bvNone
        TabOrder = 1
        object DiscoverButton: TButton
          Left = 8
          Top = 8
          Width = 65
          Height = 25
          Caption = 'Discover'
          Enabled = False
          TabOrder = 0
          OnClick = DiscoverButtonClick
        end
        object RegisterButton: TButton
          Left = 79
          Top = 8
          Width = 65
          Height = 25
          Caption = 'Register'
          Enabled = False
          TabOrder = 1
          OnClick = RegisterButtonClick
        end
        object DeregisterButton: TButton
          Left = 150
          Top = 8
          Width = 65
          Height = 25
          Caption = 'Deregister'
          TabOrder = 2
          OnClick = DeregisterButtonClick
        end
        object DeregisterAllButton: TButton
          Left = 221
          Top = 8
          Width = 65
          Height = 25
          Caption = 'Clear All'
          TabOrder = 3
          OnClick = DeregisterAllButtonClick
        end
      end
    end
  end
  object CenterPanel: TPanel
    Left = 550
    Top = 45
    Width = 550
    Height = 536
    Align = alClient
    TabOrder = 2
    object ControlGroup: TGroupBox
      Left = 1
      Top = 1
      Width = 701
      Height = 65
      Align = alTop
      Caption = ' Module Control '
      TabOrder = 0
      object SetOffButton: TButton
        Left = 16
        Top = 24
        Width = 65
        Height = 25
        Caption = 'Set Off'
        TabOrder = 0
        OnClick = SetOffButtonClick
      end
      object SetStandbyButton: TButton
        Left = 87
        Top = 24
        Width = 65
        Height = 25
        Caption = 'Set Standby'
        TabOrder = 1
        OnClick = SetStandbyButtonClick
      end
      object SetPrechargeButton: TButton
        Left = 158
        Top = 24
        Width = 75
        Height = 25
        Caption = 'Set Precharge'
        TabOrder = 2
        OnClick = SetPrechargeButtonClick
      end
      object SetOnButton: TButton
        Left = 239
        Top = 24
        Width = 65
        Height = 25
        Caption = 'Set On'
        TabOrder = 3
        OnClick = SetOnButtonClick
      end
      object SetAllStatesButton: TButton
        Left = 320
        Top = 24
        Width = 97
        Height = 25
        Caption = 'Set All States'
        TabOrder = 4
        OnClick = SetAllStatesButtonClick
      end
      object EnableBalancingCheck: TCheckBox
        Left = 423
        Top = 28
        Width = 97
        Height = 17
        Caption = 'Enable Balancing'
        TabOrder = 3
      end
      object BalancingMaskEdit: TEdit
        Left = 526
        Top = 26
        Width = 66
        Height = 21
        TabOrder = 5
        Text = '0x00'
      end
    end
    object DetailsPageControl: TPageControl
      Left = 1
      Top = 97
      Width = 701
      Height = 394
      ActivePage = StatusTab
      Align = alClient
      TabOrder = 1
      OnChange = DetailsPageControlChange
      object StatusTab: TTabSheet
        Caption = 'Status'
        object VoltageLabel: TLabel
          Left = 16
          Top = 16
          Width = 87
          Height = 19
          Caption = 'Voltage: 0.0 V'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -16
          Font.Name = 'Tahoma'
          Font.Style = []
          ParentFont = False
        end
        object CurrentLabel: TLabel
          Left = 16
          Top = 48
          Width = 87
          Height = 19
          Caption = 'Current: 0.0 A'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -16
          Font.Name = 'Tahoma'
          Font.Style = []
          ParentFont = False
        end
        object TemperatureLabel: TLabel
          Left = 16
          Top = 80
          Width = 123
          Height = 19
          Caption = 'Temperature: 25.0 C'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -16
          Font.Name = 'Tahoma'
          Font.Style = []
          ParentFont = False
        end
        object SOCLabel: TLabel
          Left = 16
          Top = 112
          Width = 72
          Height = 19
          Caption = 'SOC: 0.0 %'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -16
          Font.Name = 'Tahoma'
          Font.Style = []
          ParentFont = False
        end
        object SOHLabel: TLabel
          Left = 16
          Top = 144
          Width = 82
          Height = 19
          Caption = 'SOH: 100.0 %'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -16
          Font.Name = 'Tahoma'
          Font.Style = []
          ParentFont = False
        end
        object StatusGrid: TStringGrid
          Left = 240
          Top = 16
          Width = 441
          Height = 337
          ColCount = 3
          DefaultColWidth = 140
          FixedCols = 0
          RowCount = 10
          TabOrder = 0
        end
      end
      object CellsTab: TTabSheet
        Caption = 'Cells'
        ImageIndex = 1
        object CellGrid: TStringGrid
          Left = 3
          Top = 3
          Width = 358
          Height = 360
          ColCount = 3
          DefaultColWidth = 115
          FixedCols = 0
          RowCount = 15
          TabOrder = 0
        end
        object AutoRangeCheck: TCheckBox
          Left = 376
          Top = 16
          Width = 97
          Height = 17
          Caption = 'Auto Range'
          Checked = True
          State = cbChecked
          TabOrder = 1
        end
      end
      object HistoryTab: TTabSheet
        Caption = 'History'
        ImageIndex = 2
        object HistoryMemo: TMemo
          Left = 0
          Top = 0
          Width = 693
          Height = 335
          Align = alClient
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'Courier New'
          Font.Style = []
          ParentFont = False
          ReadOnly = True
          ScrollBars = ssVertical
          TabOrder = 0
        end
        object HistoryButtonPanel: TPanel
          Left = 0
          Top = 335
          Width = 693
          Height = 35
          Align = alBottom
          BevelOuter = bvNone
          TabOrder = 1
          object ClearHistoryButton: TButton
            Left = 8
            Top = 5
            Width = 75
            Height = 25
            Caption = 'Clear'
            TabOrder = 0
          end
          object ExportHistoryButton: TButton
            Left = 89
            Top = 5
            Width = 75
            Height = 25
            Caption = 'Export'
            TabOrder = 1
          end
        end
      end
      object Web4Tab: TTabSheet
        Caption = 'Web4'
        ImageIndex = 3
        object Web4Group: TGroupBox
          Left = 16
          Top = 16
          Width = 665
          Height = 337
          Caption = ' Web4 Key Management '
          TabOrder = 0
          object DeviceKeyEdit: TEdit
            Left = 16
            Top = 32
            Width = 633
            Height = 21
            TabOrder = 0
          end
          object LCTKeyEdit: TEdit
            Left = 16
            Top = 72
            Width = 633
            Height = 21
            TabOrder = 1
          end
          object ComponentIdEdit: TEdit
            Left = 16
            Top = 112
            Width = 633
            Height = 21
            TabOrder = 2
          end
          object DistributeKeysButton: TButton
            Left = 16
            Top = 152
            Width = 113
            Height = 25
            Caption = 'Distribute Keys'
            TabOrder = 3
            OnClick = DistributeKeysButtonClick
          end
          object EncryptionEnabledCheck: TCheckBox
            Left = 144
            Top = 156
            Width = 121
            Height = 17
            Caption = 'Encryption Enabled'
            TabOrder = 4
          end
        end
      end
    end
  end
  object BottomPanel: TPanel
    Left = 0
    Top = 581
    Width = 1100
    Height = 19
    Align = alBottom
    TabOrder = 3
    object StatusBar: TStatusBar
      Left = 1
      Top = 1
      Width = 998
      Height = 17
      Align = alClient
      Panels = <
        item
          Text = 'Pack: 0.0V'
          Width = 150
        end
        item
          Text = 'Current: 0.0A'
          Width = 150
        end
        item
          Text = 'Modules: 0'
          Width = 100
        end
        item
          Text = 'TX: 0 (0.0/s)'
          Width = 150
        end
        item
          Text = 'RX: 0 (0.0/s)'
          Width = 150
        end>
    end
  end
  object UpdateTimer: TTimer
    Enabled = False
    Interval = 100
    OnTimer = UpdateTimerTimer
    Left = 920
    Top = 104
  end
  object TimeoutTimer: TTimer
    Enabled = False
    OnTimer = TimeoutTimerTimer
    Left = 920
    Top = 152
  end
  object DiscoveryTimer: TTimer
    Enabled = False
    Interval = 5000
    OnTimer = DiscoveryTimerTimer
    Left = 920
    Top = 200
  end
  object PollTimer: TTimer
    Enabled = False
    Interval = 100
    OnTimer = PollTimerTimer
    Left = 920
    Top = 248
  end
  object CellPollTimer: TTimer
    Enabled = False
    Interval = 50
    OnTimer = CellPollTimerTimer
    Left = 920
    Top = 296
  end
  object MainMenu: TMainMenu
    Left = 920
    Top = 344
    object FileMenu: TMenuItem
      Caption = '&File'
      object SaveConfigItem: TMenuItem
        Caption = 'Save Configuration'
      end
      object LoadConfigItem: TMenuItem
        Caption = 'Load Configuration'
      end
      object N1: TMenuItem
        Caption = '-'
      end
      object ExportDataItem: TMenuItem
        Caption = 'Export Data...'
        OnClick = ExportDataItemClick
      end
      object N2: TMenuItem
        Caption = '-'
      end
      object ExitItem: TMenuItem
        Caption = 'E&xit'
        OnClick = ExitItemClick
      end
    end
    object ToolsMenu: TMenuItem
      Caption = '&Tools'
      object LoggingItem: TMenuItem
        Caption = 'Enable Logging'
      end
      object SimulationItem: TMenuItem
        Caption = 'Simulation Mode'
      end
      object DiagnosticsItem: TMenuItem
        Caption = 'Diagnostics'
      end
    end
    object HelpMenu: TMenuItem
      Caption = '&Help'
      object AboutItem: TMenuItem
        Caption = 'About'
      end
    end
  end
  object SaveDialog: TSaveDialog
    DefaultExt = 'csv'
    Filter = 'CSV Files (*.csv)|*.csv|All Files (*.*)|*.*'
    Left = 920
    Top = 344
  end
  object OpenDialog: TOpenDialog
    DefaultExt = 'ini'
    Filter = 'Config Files (*.ini)|*.ini|All Files (*.*)|*.*'
    Left = 920
    Top = 392
  end
end