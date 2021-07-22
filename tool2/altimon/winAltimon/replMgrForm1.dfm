object replMgr: TreplMgr
  Left = 0
  Top = 0
  Caption = 'ReplMgr'
  ClientHeight = 595
  ClientWidth = 706
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
  object Panel1: TPanel
    Left = 0
    Top = 0
    Width = 706
    Height = 41
    Align = alTop
    BevelInner = bvLowered
    Ctl3D = False
    ParentCtl3D = False
    TabOrder = 0
    object Button1: TButton
      Left = 90
      Top = 10
      Width = 75
      Height = 25
      Caption = 'Go'
      TabOrder = 0
      OnClick = Button1Click
    end
    object CheckBox1: TCheckBox
      Left = 11
      Top = 12
      Width = 73
      Height = 17
      Caption = 'AutoView'
      TabOrder = 1
      OnClick = CheckBox1Click
    end
    object Button2: TButton
      Left = 171
      Top = 10
      Width = 75
      Height = 25
      Caption = 'Threshold'
      TabOrder = 2
      OnClick = Button2Click
    end
  end
  object DBGrid1: TDBGrid
    Left = 0
    Top = 41
    Width = 706
    Height = 160
    Align = alTop
    Color = clWhite
    Ctl3D = False
    DataSource = DataSource1
    FixedColor = 8421440
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -11
    Font.Name = 'Tahoma'
    Font.Style = []
    ImeName = 'Microsoft IME 2003'
    Options = [dgTitles, dgIndicator, dgColumnResize, dgColLines, dgRowLines, dgTabs, dgRowSelect, dgConfirmDelete, dgCancelOnExit]
    ParentCtl3D = False
    ParentFont = False
    TabOrder = 1
    TitleFont.Charset = DEFAULT_CHARSET
    TitleFont.Color = clWhite
    TitleFont.Height = -11
    TitleFont.Name = 'Tahoma'
    TitleFont.Style = []
  end
  object DSN: TPanel
    Left = 0
    Top = 554
    Width = 706
    Height = 41
    Align = alBottom
    BevelInner = bvLowered
    TabOrder = 2
  end
  object DBGrid2: TDBGrid
    Left = 0
    Top = 242
    Width = 706
    Height = 111
    Align = alTop
    Color = clWhite
    Ctl3D = False
    DataSource = DataSource2
    FixedColor = 8421440
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -11
    Font.Name = 'Tahoma'
    Font.Style = []
    ImeName = 'Microsoft IME 2003'
    Options = [dgTitles, dgIndicator, dgColumnResize, dgColLines, dgRowLines, dgTabs, dgRowSelect, dgConfirmDelete, dgCancelOnExit]
    ParentCtl3D = False
    ParentFont = False
    TabOrder = 3
    TitleFont.Charset = DEFAULT_CHARSET
    TitleFont.Color = clWhite
    TitleFont.Height = -11
    TitleFont.Name = 'Tahoma'
    TitleFont.Style = []
  end
  object DBGrid3: TDBGrid
    Left = 0
    Top = 394
    Width = 706
    Height = 112
    Align = alTop
    Color = clWhite
    Ctl3D = False
    DataSource = DataSource3
    FixedColor = 8421440
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -11
    Font.Name = 'Tahoma'
    Font.Style = []
    ImeName = 'Microsoft IME 2003'
    Options = [dgTitles, dgIndicator, dgColumnResize, dgColLines, dgRowLines, dgTabs, dgRowSelect, dgConfirmDelete, dgCancelOnExit]
    ParentCtl3D = False
    ParentFont = False
    TabOrder = 4
    TitleFont.Charset = DEFAULT_CHARSET
    TitleFont.Color = clWhite
    TitleFont.Height = -11
    TitleFont.Name = 'Tahoma'
    TitleFont.Style = []
  end
  object Panel2: TPanel
    Left = 0
    Top = 201
    Width = 706
    Height = 41
    Align = alTop
    Caption = 'Sender Status'
    TabOrder = 5
  end
  object Panel3: TPanel
    Left = 0
    Top = 353
    Width = 706
    Height = 41
    Align = alTop
    Caption = 'Receiver Status'
    TabOrder = 6
  end
  object Panel4: TPanel
    Left = 200
    Top = 232
    Width = 369
    Height = 81
    BevelInner = bvLowered
    BevelKind = bkTile
    TabOrder = 7
    Visible = False
    object Label3: TLabel
      Left = 15
      Top = 16
      Width = 88
      Height = 13
      Caption = 'Replication Gap > '
    end
    object Edit1: TEdit
      Left = 106
      Top = 13
      Width = 121
      Height = 21
      ImeName = 'Microsoft IME 2003'
      TabOrder = 0
      Text = '0'
    end
    object Button3: TButton
      Left = 244
      Top = 11
      Width = 75
      Height = 25
      Caption = 'Start'
      TabOrder = 1
      OnClick = Button3Click
    end
    object CheckBox2: TCheckBox
      Left = 328
      Top = 16
      Width = 97
      Height = 17
      Caption = 'CheckBox2'
      TabOrder = 2
      Visible = False
    end
    object Button4: TButton
      Left = 244
      Top = 42
      Width = 75
      Height = 25
      Caption = 'Close'
      TabOrder = 3
      OnClick = Button4Click
    end
  end
  object ADOConnection1: TADOConnection
    CommandTimeout = 3
    ConnectionTimeout = 3
    LoginPrompt = False
    Left = 624
    Top = 8
  end
  object Timer1: TTimer
    Enabled = False
    OnTimer = Timer1Timer
    Left = 656
    Top = 8
  end
  object ADOQuery1: TADOQuery
    Connection = ADOConnection1
    Parameters = <>
    Left = 664
    Top = 48
  end
  object DataSource1: TDataSource
    DataSet = ADOQuery1
    Left = 632
    Top = 48
  end
  object DataSource2: TDataSource
    DataSet = ADOQuery2
    Left = 640
    Top = 206
  end
  object ADOQuery2: TADOQuery
    Connection = ADOConnection1
    Parameters = <>
    Left = 672
    Top = 206
  end
  object DataSource3: TDataSource
    DataSet = ADOQuery3
    Left = 632
    Top = 398
  end
  object ADOQuery3: TADOQuery
    Connection = ADOConnection1
    Parameters = <>
    Left = 664
    Top = 398
  end
end
