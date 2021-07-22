object sessionMgrForm: TsessionMgrForm
  Left = 0
  Top = 0
  Caption = 'sessionMgrForm'
  ClientHeight = 619
  ClientWidth = 821
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  OldCreateOrder = False
  OnClose = FormClose
  OnResize = FormResize
  OnShow = FormShow
  PixelsPerInch = 96
  TextHeight = 13
  object DBGrid1: TDBGrid
    Left = 0
    Top = 90
    Width = 821
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
    TabOrder = 0
    TitleFont.Charset = DEFAULT_CHARSET
    TitleFont.Color = clWhite
    TitleFont.Height = -11
    TitleFont.Name = 'Tahoma'
    TitleFont.Style = []
    OnDblClick = DBGrid1DblClick
  end
  object RadioGroup1: TRadioGroup
    Left = 0
    Top = 0
    Width = 821
    Height = 49
    Align = alTop
    Caption = ' Queryt Flag  '
    Columns = 4
    Ctl3D = False
    Items.Strings = (
      'All session'
      'long-Run Session'
      'Timeout Session'
      'lockWait Session')
    ParentCtl3D = False
    TabOrder = 1
  end
  object Panel1: TPanel
    Left = 0
    Top = 49
    Width = 821
    Height = 41
    Align = alTop
    BevelInner = bvLowered
    Ctl3D = False
    ParentCtl3D = False
    TabOrder = 2
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
  object DBGrid2: TDBGrid
    Left = 0
    Top = 376
    Width = 821
    Height = 202
    Align = alBottom
    Ctl3D = False
    DataSource = DataSource1
    ImeName = 'Microsoft IME 2003'
    Options = [dgTitles, dgIndicator, dgColumnResize, dgColLines, dgRowLines, dgTabs, dgRowSelect, dgConfirmDelete, dgCancelOnExit]
    ParentCtl3D = False
    TabOrder = 3
    TitleFont.Charset = DEFAULT_CHARSET
    TitleFont.Color = clWindowText
    TitleFont.Height = -11
    TitleFont.Name = 'Tahoma'
    TitleFont.Style = []
    OnDblClick = DBGrid2DblClick
  end
  object DSN: TPanel
    Left = 0
    Top = 578
    Width = 821
    Height = 41
    Align = alBottom
    BevelInner = bvLowered
    TabOrder = 4
  end
  object Memo1: TMemo
    Left = 168
    Top = 273
    Width = 537
    Height = 305
    Hint = 'Double Click , close QueryWindow'
    BevelKind = bkSoft
    Ctl3D = False
    ImeName = 'Microsoft IME 2003'
    Lines.Strings = (
      'Memo1')
    ParentCtl3D = False
    ParentShowHint = False
    ScrollBars = ssBoth
    ShowHint = True
    TabOrder = 5
    Visible = False
    OnDblClick = Memo1DblClick
  end
  object ADOConnection1: TADOConnection
    CommandTimeout = 3
    ConnectionTimeout = 3
    LoginPrompt = False
    Left = 720
    Top = 16
  end
  object ADOQuery1: TADOQuery
    Connection = ADOConnection1
    Parameters = <>
    Left = 656
    Top = 288
  end
  object DataSource2: TDataSource
    DataSet = ADOQuery1
    Left = 696
    Top = 288
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
  object Timer1: TTimer
    Enabled = False
    OnTimer = Timer1Timer
    Left = 752
    Top = 16
  end
  object ADOQuery3: TADOQuery
    Connection = ADOConnection1
    Parameters = <>
    Left = 616
    Top = 472
  end
end
