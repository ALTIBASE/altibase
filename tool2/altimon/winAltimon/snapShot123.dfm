object SnapShotForm: TSnapShotForm
  Left = 0
  Top = 0
  BorderIcons = [biSystemMenu, biMinimize]
  Caption = 'SnapShot'
  ClientHeight = 384
  ClientWidth = 694
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  OldCreateOrder = False
  OnClose = FormClose
  PixelsPerInch = 96
  TextHeight = 13
  object DSN: TPanel
    Left = 0
    Top = 343
    Width = 694
    Height = 41
    Align = alBottom
    BevelInner = bvLowered
    TabOrder = 0
    object Panel7: TPanel
      Left = 576
      Top = 2
      Width = 116
      Height = 37
      Align = alRight
      BevelInner = bvSpace
      TabOrder = 0
    end
  end
  object GroupBox1: TGroupBox
    Left = 0
    Top = 41
    Width = 694
    Height = 56
    Align = alTop
    Caption = '  Session Summary  '
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -11
    Font.Name = 'Tahoma'
    Font.Style = [fsBold]
    ParentFont = False
    TabOrder = 1
    OnDblClick = GroupBox1DblClick
    object Panel1: TPanel
      Left = 2
      Top = 15
      Width = 690
      Height = 39
      Align = alClient
      BevelInner = bvLowered
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clWindowText
      Font.Height = -11
      Font.Name = 'Tahoma'
      Font.Style = []
      ParentFont = False
      TabOrder = 0
      object Label1: TLabel
        Left = 16
        Top = 14
        Width = 65
        Height = 13
        Caption = 'SessionCount'
      end
      object Label2: TLabel
        Left = 168
        Top = 13
        Width = 75
        Height = 13
        Caption = 'longRunSession'
      end
      object Label3: TLabel
        Left = 336
        Top = 13
        Width = 76
        Height = 13
        Caption = 'lockWaitSession'
      end
      object SSCOUNT: TEdit
        Left = 87
        Top = 11
        Width = 50
        Height = 19
        Ctl3D = False
        ImeName = 'Microsoft IME 2003'
        ParentCtl3D = False
        ParentShowHint = False
        ShowHint = True
        TabOrder = 0
      end
      object WCOUNT: TEdit
        Left = 249
        Top = 11
        Width = 50
        Height = 19
        Ctl3D = False
        ImeName = 'Microsoft IME 2003'
        ParentCtl3D = False
        ReadOnly = True
        TabOrder = 1
      end
      object LCOUNT: TEdit
        Left = 418
        Top = 11
        Width = 48
        Height = 19
        Ctl3D = False
        ImeName = 'Microsoft IME 2003'
        ParentCtl3D = False
        ReadOnly = True
        TabOrder = 2
      end
    end
  end
  object Panel2: TPanel
    Left = 0
    Top = 0
    Width = 694
    Height = 41
    Align = alTop
    TabOrder = 2
    object CheckBox1: TCheckBox
      Left = 10
      Top = 13
      Width = 73
      Height = 17
      Caption = 'AutoView'
      TabOrder = 0
      OnClick = CheckBox1Click
    end
    object Button1: TButton
      Left = 89
      Top = 9
      Width = 75
      Height = 25
      Caption = 'go'
      TabOrder = 1
      OnClick = Button1Click
    end
    object DBGrid1: TDBGrid
      Left = 354
      Top = 9
      Width = 320
      Height = 17
      DataSource = DataSource1
      ImeName = 'Microsoft IME 2003'
      Options = [dgEditing, dgAlwaysShowEditor, dgTitles, dgIndicator, dgColumnResize, dgColLines, dgRowLines, dgTabs, dgConfirmDelete, dgCancelOnExit]
      TabOrder = 2
      TitleFont.Charset = DEFAULT_CHARSET
      TitleFont.Color = clWindowText
      TitleFont.Height = -11
      TitleFont.Name = 'Tahoma'
      TitleFont.Style = []
      Visible = False
    end
    object Button2: TButton
      Left = 168
      Top = 9
      Width = 75
      Height = 25
      Caption = 'history'
      TabOrder = 3
      OnClick = Button2Click
    end
  end
  object GroupBox2: TGroupBox
    Left = 0
    Top = 97
    Width = 694
    Height = 56
    Align = alTop
    Caption = '  Memory Summary  '
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -11
    Font.Name = 'Tahoma'
    Font.Style = [fsBold]
    ParentFont = False
    TabOrder = 3
    OnDblClick = GroupBox2DblClick
    object Panel3: TPanel
      Left = 2
      Top = 15
      Width = 690
      Height = 39
      Align = alClient
      BevelInner = bvLowered
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clWindowText
      Font.Height = -11
      Font.Name = 'Tahoma'
      Font.Style = []
      ParentFont = False
      TabOrder = 0
      object Label4: TLabel
        Left = 16
        Top = 14
        Width = 69
        Height = 13
        Caption = 'Total MaxAlloc'
      end
      object Label5: TLabel
        Left = 236
        Top = 13
        Width = 55
        Height = 13
        Caption = 'CurrentUse'
      end
      object TOTMEM: TEdit
        Left = 91
        Top = 11
        Width = 115
        Height = 19
        Ctl3D = False
        ImeName = 'Microsoft IME 2003'
        ParentCtl3D = False
        ParentShowHint = False
        ShowHint = True
        TabOrder = 0
      end
      object TOTUSE: TEdit
        Left = 297
        Top = 11
        Width = 128
        Height = 19
        Ctl3D = False
        ImeName = 'Microsoft IME 2003'
        ParentCtl3D = False
        ReadOnly = True
        TabOrder = 1
      end
    end
  end
  object GroupBox3: TGroupBox
    Left = 0
    Top = 153
    Width = 694
    Height = 56
    Align = alTop
    Caption = '  Memory Table Summary  '
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -11
    Font.Name = 'Tahoma'
    Font.Style = [fsBold]
    ParentFont = False
    TabOrder = 4
    OnDblClick = GroupBox3DblClick
    object Panel4: TPanel
      Left = 2
      Top = 15
      Width = 690
      Height = 39
      Align = alClient
      BevelInner = bvLowered
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clWindowText
      Font.Height = -11
      Font.Name = 'Tahoma'
      Font.Style = []
      ParentFont = False
      TabOrder = 0
      object Label6: TLabel
        Left = 16
        Top = 14
        Width = 63
        Height = 13
        Caption = 'MemAllocSize'
      end
      object Label7: TLabel
        Left = 228
        Top = 13
        Width = 63
        Height = 13
        Caption = 'MemFreeSize'
      end
      object Label8: TLabel
        Left = 460
        Top = 13
        Width = 46
        Height = 13
        Caption = 'IndexUse'
      end
      object MEMALLOC: TEdit
        Left = 91
        Top = 11
        Width = 115
        Height = 19
        Ctl3D = False
        ImeName = 'Microsoft IME 2003'
        ParentCtl3D = False
        ParentShowHint = False
        ShowHint = True
        TabOrder = 0
      end
      object MEMFREE: TEdit
        Left = 297
        Top = 11
        Width = 128
        Height = 19
        Ctl3D = False
        ImeName = 'Microsoft IME 2003'
        ParentCtl3D = False
        ReadOnly = True
        TabOrder = 1
      end
      object INDEXUSE: TEdit
        Left = 512
        Top = 11
        Width = 128
        Height = 19
        Ctl3D = False
        ImeName = 'Microsoft IME 2003'
        ParentCtl3D = False
        ReadOnly = True
        TabOrder = 2
      end
    end
  end
  object GroupBox4: TGroupBox
    Left = 0
    Top = 209
    Width = 694
    Height = 56
    Align = alTop
    Caption = '  CheckPoint Summary  '
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -11
    Font.Name = 'Tahoma'
    Font.Style = [fsBold]
    ParentFont = False
    TabOrder = 5
    object Panel5: TPanel
      Left = 2
      Top = 15
      Width = 690
      Height = 39
      Align = alClient
      BevelInner = bvLowered
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clWindowText
      Font.Height = -11
      Font.Name = 'Tahoma'
      Font.Style = []
      ParentFont = False
      TabOrder = 0
      object Label9: TLabel
        Left = 17
        Top = 14
        Width = 64
        Height = 13
        Caption = 'OldestLogFile'
      end
      object Label10: TLabel
        Left = 221
        Top = 13
        Width = 70
        Height = 13
        Caption = 'CurrentLogFile'
      end
      object OLDLOG: TEdit
        Left = 92
        Top = 11
        Width = 70
        Height = 19
        Ctl3D = False
        ImeName = 'Microsoft IME 2003'
        ParentCtl3D = False
        ParentShowHint = False
        ShowHint = True
        TabOrder = 0
      end
      object CURLOG: TEdit
        Left = 297
        Top = 11
        Width = 72
        Height = 19
        Ctl3D = False
        ImeName = 'Microsoft IME 2003'
        ParentCtl3D = False
        ReadOnly = True
        TabOrder = 1
      end
    end
  end
  object GroupBox5: TGroupBox
    Left = 0
    Top = 265
    Width = 694
    Height = 56
    Align = alTop
    Caption = ' Replication Summary  '
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -11
    Font.Name = 'Tahoma'
    Font.Style = [fsBold]
    ParentFont = False
    TabOrder = 6
    OnDblClick = GroupBox5DblClick
    object Panel6: TPanel
      Left = 2
      Top = 15
      Width = 690
      Height = 39
      Align = alClient
      BevelInner = bvLowered
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clWindowText
      Font.Height = -11
      Font.Name = 'Tahoma'
      Font.Style = []
      ParentFont = False
      TabOrder = 0
      object Label11: TLabel
        Left = 17
        Top = 14
        Width = 68
        Height = 13
        Caption = 'ReplicationList'
      end
      object ComboBox1: TComboBox
        Left = 89
        Top = 11
        Width = 566
        Height = 21
        Ctl3D = False
        ImeName = 'Microsoft IME 2003'
        ItemHeight = 13
        ParentCtl3D = False
        TabOrder = 0
      end
    end
  end
  object Timer1: TTimer
    Enabled = False
    OnTimer = Timer1Timer
    Left = 448
    Top = 8
  end
  object ADOQuery1: TADOQuery
    Connection = ADOConnection1
    Parameters = <>
    Left = 336
    Top = 8
  end
  object DataSource1: TDataSource
    DataSet = ADOQuery1
    Left = 368
    Top = 8
  end
  object ADOConnection1: TADOConnection
    CommandTimeout = 3
    ConnectionTimeout = 3
    LoginPrompt = False
    Left = 408
    Top = 8
  end
end
