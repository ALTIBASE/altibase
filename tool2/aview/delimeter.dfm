object Form3: TForm3
  Left = 0
  Top = 0
  BorderIcons = [biSystemMenu, biMinimize]
  BorderStyle = bsSingle
  Caption = 'Form3'
  ClientHeight = 164
  ClientWidth = 328
  Color = clBtnFace
  Font.Charset = ANSI_CHARSET
  Font.Color = clWindowText
  Font.Height = -13
  Font.Name = #44404#47548#52404
  Font.Style = []
  OldCreateOrder = False
  OnShow = FormShow
  PixelsPerInch = 96
  TextHeight = 13
  object Panel1: TPanel
    Left = 8
    Top = 32
    Width = 121
    Height = 33
    Caption = 'Column delimeter'
    TabOrder = 0
  end
  object Panel2: TPanel
    Left = 8
    Top = 71
    Width = 121
    Height = 33
    Caption = 'Row delimeter'
    TabOrder = 1
  end
  object COLS: TEdit
    Left = 135
    Top = 39
    Width = 162
    Height = 19
    Ctl3D = False
    ImeName = 'Microsoft IME 2003'
    MaxLength = 20
    ParentCtl3D = False
    TabOrder = 2
  end
  object ROWS: TEdit
    Left = 135
    Top = 79
    Width = 162
    Height = 19
    Hint = 'Row Delimeter add "\n" you set'
    Ctl3D = False
    ImeName = 'Microsoft IME 2003'
    MaxLength = 20
    ParentCtl3D = False
    ParentShowHint = False
    ShowHint = True
    TabOrder = 3
  end
  object Button1: TButton
    Left = 136
    Top = 120
    Width = 75
    Height = 25
    Caption = 'Set'
    TabOrder = 4
    OnClick = Button1Click
  end
end
