object tblForm: TtblForm
  Left = 0
  Top = 0
  Caption = 'tableSpaceMgr'
  ClientHeight = 463
  ClientWidth = 677
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
  object DBGrid1: TDBGrid
    Left = 0
    Top = 41
    Width = 677
    Height = 120
    Align = alTop
    Ctl3D = False
    DataSource = DataSource1
    FixedColor = clTeal
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -11
    Font.Name = 'Tahoma'
    Font.Style = []
    ImeName = 'Microsoft IME 2003'
    Options = [dgTitles, dgIndicator, dgColumnResize, dgColLines, dgRowLines, dgTabs, dgRowSelect, dgConfirmDelete, dgCancelOnExit]
    ParentCtl3D = False
    ParentFont = False
    TabOrder = 0
    TitleFont.Charset = DEFAULT_CHARSET
    TitleFont.Color = clWhite
    TitleFont.Height = -11
    TitleFont.Name = 'Tahoma'
    TitleFont.Style = []
  end
  object DBGrid2: TDBGrid
    Left = 0
    Top = 161
    Width = 677
    Height = 261
    Align = alClient
    Ctl3D = False
    DataSource = DataSource2
    FixedColor = clTeal
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
    OnDblClick = DBGrid2DblClick
  end
  object Panel1: TPanel
    Left = 0
    Top = 0
    Width = 677
    Height = 41
    Align = alTop
    BevelInner = bvLowered
    Ctl3D = False
    ParentCtl3D = False
    TabOrder = 2
    object CheckBox1: TCheckBox
      Left = 11
      Top = 12
      Width = 73
      Height = 17
      Caption = 'AutoView'
      TabOrder = 0
      OnClick = CheckBox1Click
    end
    object Button1: TButton
      Left = 90
      Top = 10
      Width = 75
      Height = 25
      Caption = 'Go'
      TabOrder = 1
      OnClick = Button1Click
    end
  end
  object DSN: TPanel
    Left = 0
    Top = 422
    Width = 677
    Height = 41
    Align = alBottom
    BevelInner = bvLowered
    Ctl3D = False
    ParentCtl3D = False
    TabOrder = 3
  end
  object ADOConnection1: TADOConnection
    CommandTimeout = 3
    ConnectionTimeout = 3
    LoginPrompt = False
    Left = 608
    Top = 16
  end
  object Timer1: TTimer
    Enabled = False
    OnTimer = Timer1Timer
    Left = 640
    Top = 16
  end
  object ADOQuery1: TADOQuery
    Connection = ADOConnection1
    Parameters = <>
    Left = 552
    Top = 120
  end
  object ADOQuery2: TADOQuery
    Connection = ADOConnection1
    Parameters = <>
    Left = 560
    Top = 272
  end
  object DataSource1: TDataSource
    DataSet = ADOQuery1
    Left = 584
    Top = 120
  end
  object DataSource2: TDataSource
    DataSet = ADOQuery2
    Left = 592
    Top = 272
  end
end
