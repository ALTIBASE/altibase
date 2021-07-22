object diskTblMgrForm: TdiskTblMgrForm
  Left = 0
  Top = 0
  BorderIcons = [biSystemMenu, biMinimize]
  Caption = 'diskTblMgr'
  ClientHeight = 446
  ClientWidth = 435
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  OldCreateOrder = False
  PixelsPerInch = 96
  TextHeight = 13
  object Panel1: TPanel
    Left = 0
    Top = 0
    Width = 435
    Height = 41
    Align = alTop
    TabOrder = 0
    ExplicitLeft = 384
    ExplicitTop = 128
    ExplicitWidth = 185
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
    object TBS_NAME: TEdit
      Left = 171
      Top = 14
      Width = 121
      Height = 21
      ImeName = 'Microsoft IME 2003'
      TabOrder = 2
      Text = 'TBS_NAME'
      Visible = False
    end
  end
  object DBGrid1: TDBGrid
    Left = 0
    Top = 41
    Width = 435
    Height = 360
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
    TabOrder = 1
    TitleFont.Charset = DEFAULT_CHARSET
    TitleFont.Color = clWhite
    TitleFont.Height = -11
    TitleFont.Name = 'Tahoma'
    TitleFont.Style = []
  end
  object DSN: TPanel
    Left = 0
    Top = 405
    Width = 435
    Height = 41
    Align = alBottom
    BevelInner = bvLowered
    TabOrder = 2
    ExplicitLeft = -416
    ExplicitWidth = 821
  end
  object ADOConnection1: TADOConnection
    CommandTimeout = 3
    ConnectionTimeout = 3
    LoginPrompt = False
    Left = 232
    Top = 8
  end
  object Timer1: TTimer
    Enabled = False
    OnTimer = Timer1Timer
    Left = 264
    Top = 8
  end
  object ADOQuery1: TADOQuery
    Connection = ADOConnection1
    Parameters = <>
    Left = 304
    Top = 8
  end
  object DataSource2: TDataSource
    DataSet = ADOQuery1
    Left = 344
    Top = 8
  end
end
