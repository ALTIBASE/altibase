object lockMgrForm: TlockMgrForm
  Left = 0
  Top = 0
  Caption = 'lockMgr'
  ClientHeight = 542
  ClientWidth = 849
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  OldCreateOrder = False
  OnClose = FormClose
  OnResize = FormResize
  PixelsPerInch = 96
  TextHeight = 13
  object DSN: TPanel
    Left = 0
    Top = 501
    Width = 849
    Height = 41
    Align = alBottom
    BevelInner = bvLowered
    TabOrder = 0
  end
  object DBGrid2: TDBGrid
    Left = 0
    Top = 299
    Width = 849
    Height = 202
    Align = alBottom
    Ctl3D = False
    DataSource = DataSource1
    ImeName = 'Microsoft IME 2003'
    Options = [dgTitles, dgIndicator, dgColumnResize, dgColLines, dgRowLines, dgTabs, dgRowSelect, dgConfirmDelete, dgCancelOnExit]
    ParentCtl3D = False
    TabOrder = 1
    TitleFont.Charset = DEFAULT_CHARSET
    TitleFont.Color = clWindowText
    TitleFont.Height = -11
    TitleFont.Name = 'Tahoma'
    TitleFont.Style = []
    OnDblClick = DBGrid2DblClick
  end
  object DBGrid1: TDBGrid
    Left = 0
    Top = 41
    Width = 849
    Height = 231
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
    TabOrder = 2
    TitleFont.Charset = DEFAULT_CHARSET
    TitleFont.Color = clWhite
    TitleFont.Height = -11
    TitleFont.Name = 'Tahoma'
    TitleFont.Style = []
    OnDblClick = DBGrid1DblClick
  end
  object Panel1: TPanel
    Left = 0
    Top = 0
    Width = 849
    Height = 41
    Align = alTop
    BevelInner = bvLowered
    Ctl3D = False
    ParentCtl3D = False
    TabOrder = 3
    ExplicitTop = 90
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
  end
  object DBGrid3: TDBGrid
    Left = 480
    Top = 381
    Width = 320
    Height = 120
    DataSource = DataSource3
    ImeName = 'Microsoft IME 2003'
    Options = [dgEditing, dgAlwaysShowEditor, dgTitles, dgIndicator, dgColumnResize, dgColLines, dgRowLines, dgTabs, dgConfirmDelete, dgCancelOnExit]
    TabOrder = 4
    TitleFont.Charset = DEFAULT_CHARSET
    TitleFont.Color = clWindowText
    TitleFont.Height = -11
    TitleFont.Name = 'Tahoma'
    TitleFont.Style = []
    Visible = False
  end
  object ADOConnection1: TADOConnection
    CommandTimeout = 3
    ConnectionTimeout = 3
    LoginPrompt = False
    Left = 720
    Top = 16
  end
  object Timer1: TTimer
    Enabled = False
    OnTimer = Timer1Timer
    Left = 752
    Top = 16
  end
  object ADOQuery1: TADOQuery
    Connection = ADOConnection1
    Parameters = <>
    Left = 664
    Top = 216
  end
  object DataSource2: TDataSource
    DataSet = ADOQuery1
    Left = 704
    Top = 216
  end
  object ADOQuery2: TADOQuery
    Connection = ADOConnection1
    Parameters = <>
    Left = 616
    Top = 432
  end
  object DataSource1: TDataSource
    DataSet = ADOQuery2
    Left = 648
    Top = 432
  end
  object ADOQuery3: TADOQuery
    Connection = ADOConnection1
    Parameters = <>
    Left = 728
    Top = 464
  end
  object DataSource3: TDataSource
    DataSet = ADOQuery3
    Left = 760
    Top = 464
  end
end
