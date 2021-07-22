object Form1: TForm1
  Left = 0
  Top = 0
  BorderIcons = [biSystemMenu, biMinimize]
  BorderStyle = bsSingle
  Caption = 'DSN Manager'
  ClientHeight = 403
  ClientWidth = 533
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
  object DSNLIST: TListBox
    Left = 0
    Top = 0
    Width = 169
    Height = 403
    Align = alLeft
    ImeName = 'Microsoft IME 2003'
    ItemHeight = 13
    TabOrder = 0
    OnClick = DSNLISTClick
  end
  object Panel1: TPanel
    Left = 184
    Top = 24
    Width = 97
    Height = 25
    Caption = 'DSN Name'
    TabOrder = 1
  end
  object Panel2: TPanel
    Left = 184
    Top = 63
    Width = 97
    Height = 25
    Caption = 'Server'
    TabOrder = 2
  end
  object Panel3: TPanel
    Left = 184
    Top = 102
    Width = 97
    Height = 25
    Caption = 'Port'
    TabOrder = 3
  end
  object Panel4: TPanel
    Left = 184
    Top = 141
    Width = 97
    Height = 25
    Caption = 'User'
    TabOrder = 4
  end
  object Panel5: TPanel
    Left = 184
    Top = 180
    Width = 97
    Height = 25
    Caption = 'PassWord'
    TabOrder = 5
  end
  object Panel6: TPanel
    Left = 184
    Top = 219
    Width = 97
    Height = 25
    Caption = 'NLS USE'
    TabOrder = 6
  end
  object Panel7: TPanel
    Left = 184
    Top = 258
    Width = 97
    Height = 25
    Caption = 'ODBC DLL'
    TabOrder = 7
  end
  object NLS: TComboBox
    Left = 282
    Top = 222
    Width = 145
    Height = 21
    Ctl3D = False
    ImeName = 'Microsoft IME 2003'
    ItemHeight = 13
    ParentCtl3D = False
    TabOrder = 8
    Text = 'US7ASCII'
    Items.Strings = (
      'US7ASCII'
      'KO16KSC5601'
      'UTF8')
  end
  object PASSWD: TEdit
    Left = 282
    Top = 184
    Width = 121
    Height = 19
    Ctl3D = False
    ImeName = 'Microsoft IME 2003'
    MaxLength = 40
    ParentCtl3D = False
    PasswordChar = '*'
    TabOrder = 9
  end
  object USER: TEdit
    Left = 282
    Top = 144
    Width = 121
    Height = 19
    Ctl3D = False
    ImeName = 'Microsoft IME 2003'
    MaxLength = 40
    ParentCtl3D = False
    TabOrder = 10
    Text = 'SYS'
  end
  object PORT: TEdit
    Left = 282
    Top = 105
    Width = 121
    Height = 19
    Ctl3D = False
    ImeName = 'Microsoft IME 2003'
    MaxLength = 5
    ParentCtl3D = False
    TabOrder = 11
    Text = '20300'
  end
  object SERVER: TEdit
    Left = 282
    Top = 66
    Width = 121
    Height = 19
    Ctl3D = False
    ImeName = 'Microsoft IME 2003'
    MaxLength = 16
    ParentCtl3D = False
    TabOrder = 12
    Text = '127.0.0.1'
  end
  object DSN: TEdit
    Left = 282
    Top = 27
    Width = 121
    Height = 19
    Ctl3D = False
    ImeName = 'Microsoft IME 2003'
    MaxLength = 40
    ParentCtl3D = False
    TabOrder = 13
    Text = 'ALTIBASE'
  end
  object Button1: TButton
    Left = 222
    Top = 352
    Width = 75
    Height = 25
    Caption = 'Save'
    TabOrder = 14
    OnClick = Button1Click
  end
  object Button2: TButton
    Left = 312
    Top = 352
    Width = 75
    Height = 25
    Caption = 'Delete'
    TabOrder = 15
    OnClick = Button2Click
  end
  object Panel10: TPanel
    Left = 184
    Top = 296
    Width = 97
    Height = 25
    Caption = 'DataBase'
    TabOrder = 16
  end
  object DataBase: TEdit
    Left = 282
    Top = 299
    Width = 121
    Height = 19
    Ctl3D = False
    ImeName = 'Microsoft IME 2003'
    MaxLength = 40
    ParentCtl3D = False
    TabOrder = 17
    Text = 'mydb'
  end
  object DLL: TEdit
    Left = 282
    Top = 261
    Width = 243
    Height = 21
    ImeName = 'Microsoft IME 2003'
    TabOrder = 18
    Text = 'c:\Windows\system32\a4_CM451.dll'
  end
end
