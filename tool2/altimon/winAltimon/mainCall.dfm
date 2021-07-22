object mainCallForm: TmainCallForm
  Left = 0
  Top = 0
  BorderIcons = [biSystemMenu, biMinimize]
  Caption = 'mainCallForm'
  ClientHeight = 391
  ClientWidth = 814
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  OldCreateOrder = False
  OnShow = FormShow
  PixelsPerInch = 96
  TextHeight = 13
  object Panel3: TPanel
    Left = 0
    Top = 0
    Width = 814
    Height = 33
    Align = alTop
    BevelInner = bvLowered
    Ctl3D = False
    ParentCtl3D = False
    TabOrder = 0
    object Label1: TLabel
      Left = 456
      Top = 12
      Width = 24
      Height = 13
      Caption = '(sec)'
    end
    object DSNLIST: TComboBox
      Left = 113
      Top = 8
      Width = 145
      Height = 21
      ImeName = 'Microsoft IME 2003'
      ItemHeight = 13
      TabOrder = 0
    end
    object Panel1: TPanel
      Left = 8
      Top = 8
      Width = 99
      Height = 21
      BevelInner = bvLowered
      Caption = 'ODBC DSN'
      TabOrder = 1
    end
    object Panel2: TPanel
      Left = 304
      Top = 7
      Width = 99
      Height = 21
      BevelInner = bvLowered
      Caption = 'AutoView Sleep'
      TabOrder = 2
    end
    object SleepTime: TEdit
      Left = 409
      Top = 7
      Width = 40
      Height = 19
      Ctl3D = False
      ImeName = 'Microsoft IME 2003'
      ParentCtl3D = False
      TabOrder = 3
      Text = '5'
    end
  end
  object Panel4: TPanel
    Left = 0
    Top = 33
    Width = 814
    Height = 56
    Align = alTop
    BevelInner = bvLowered
    Ctl3D = False
    ParentCtl3D = False
    TabOrder = 1
    object Button1: TButton
      Left = 8
      Top = 15
      Width = 107
      Height = 25
      Hint = 'View Session information'
      Caption = 'SessionMgr'
      ParentShowHint = False
      ShowHint = True
      TabOrder = 0
      OnClick = Button1Click
    end
    object Button2: TButton
      Left = 121
      Top = 15
      Width = 107
      Height = 25
      Hint = 'view lock Statement'
      Caption = 'lockMgr'
      ParentShowHint = False
      ShowHint = True
      TabOrder = 1
      OnClick = Button2Click
    end
    object Button3: TButton
      Left = 234
      Top = 15
      Width = 107
      Height = 25
      Hint = 'View Tablespace usage'
      Caption = 'TableSpaceMgr'
      ParentShowHint = False
      ShowHint = True
      TabOrder = 2
      OnClick = Button3Click
    end
    object Button4: TButton
      Left = 347
      Top = 15
      Width = 107
      Height = 25
      Hint = 'view memory usage of modules'
      Caption = 'MemStat'
      ParentShowHint = False
      ShowHint = True
      TabOrder = 3
      OnClick = Button4Click
    end
    object Button5: TButton
      Left = 460
      Top = 15
      Width = 107
      Height = 25
      Hint = 'view memory usage of tables'
      Caption = 'MemTableMgr'
      ParentShowHint = False
      ShowHint = True
      TabOrder = 4
      OnClick = Button5Click
    end
    object Button6: TButton
      Left = 573
      Top = 15
      Width = 107
      Height = 25
      Hint = 'view status of replication'
      Caption = 'replicationMgr'
      ParentShowHint = False
      ShowHint = True
      TabOrder = 5
      OnClick = Button6Click
    end
    object Button7: TButton
      Left = 686
      Top = 15
      Width = 107
      Height = 25
      Hint = 'view summary '
      Caption = 'SnapShot'
      ParentShowHint = False
      ShowHint = True
      TabOrder = 6
      OnClick = Button7Click
    end
  end
  object EVENT: TStringGrid
    Left = 0
    Top = 89
    Width = 814
    Height = 302
    Align = alClient
    Ctl3D = False
    FixedColor = 12615808
    FixedCols = 0
    Options = [goFixedVertLine, goFixedHorzLine, goVertLine, goHorzLine, goRangeSelect, goRowSizing, goColSizing, goRowMoving, goColMoving, goRowSelect]
    ParentCtl3D = False
    TabOrder = 2
    ExplicitHeight = 280
    ColWidths = (
      144
      80
      78
      480
      3)
  end
end
