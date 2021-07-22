object Form2: TForm2
  Left = 0
  Top = 0
  BorderIcons = [biSystemMenu, biMinimize]
  BorderStyle = bsSingle
  Caption = 'AddUser'
  ClientHeight = 201
  ClientWidth = 438
  Color = clBtnFace
  Font.Charset = ANSI_CHARSET
  Font.Color = clWindowText
  Font.Height = -13
  Font.Name = #44404#47548#52404
  Font.Style = []
  OldCreateOrder = False
  PixelsPerInch = 96
  TextHeight = 13
  object Panel1: TPanel
    Left = 40
    Top = 16
    Width = 153
    Height = 33
    Caption = 'USER ID'
    TabOrder = 0
  end
  object Panel2: TPanel
    Left = 40
    Top = 55
    Width = 153
    Height = 33
    Caption = 'PASSWORD'
    TabOrder = 1
  end
  object Panel3: TPanel
    Left = 40
    Top = 94
    Width = 153
    Height = 33
    Caption = 'Default TableSpace'
    TabOrder = 2
  end
  object UID: TEdit
    Left = 199
    Top = 23
    Width = 178
    Height = 19
    Ctl3D = False
    ImeName = 'Microsoft IME 2003'
    MaxLength = 30
    ParentCtl3D = False
    TabOrder = 3
  end
  object PWD: TEdit
    Left = 200
    Top = 64
    Width = 177
    Height = 19
    Ctl3D = False
    ImeName = 'Microsoft IME 2003'
    MaxLength = 30
    ParentCtl3D = False
    PasswordChar = '*'
    TabOrder = 4
  end
  object TBSNAME: TEdit
    Left = 199
    Top = 101
    Width = 178
    Height = 19
    Ctl3D = False
    ImeName = 'Microsoft IME 2003'
    MaxLength = 30
    ParentCtl3D = False
    TabOrder = 5
  end
  object Button1: TButton
    Left = 176
    Top = 152
    Width = 89
    Height = 25
    Caption = 'CreateUser'
    TabOrder = 6
    OnClick = Button1Click
  end
end
