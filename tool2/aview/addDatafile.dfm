object Form9: TForm9
  Left = 0
  Top = 0
  BorderIcons = [biSystemMenu, biMinimize]
  BorderStyle = bsSingle
  Caption = 'CreateTablespace'
  ClientHeight = 301
  ClientWidth = 616
  Color = clBtnFace
  Font.Charset = ANSI_CHARSET
  Font.Color = clWindowText
  Font.Height = -13
  Font.Name = #44404#47548#52404
  Font.Style = []
  OldCreateOrder = False
  PixelsPerInch = 96
  TextHeight = 13
  object Label1: TLabel
    Left = 269
    Top = 118
    Width = 35
    Height = 13
    Caption = 'Mbyte'
  end
  object Label2: TLabel
    Left = 268
    Top = 148
    Width = 35
    Height = 13
    Caption = 'Mbyte'
  end
  object Label3: TLabel
    Left = 268
    Top = 178
    Width = 35
    Height = 13
    Caption = 'Kbyte'
  end
  object Panel1: TPanel
    Left = 32
    Top = 16
    Width = 105
    Height = 25
    Caption = 'SpaceName'
    TabOrder = 0
  end
  object Panel2: TPanel
    Left = 32
    Top = 47
    Width = 105
    Height = 25
    Caption = 'FileName'
    TabOrder = 1
  end
  object Panel3: TPanel
    Left = 32
    Top = 110
    Width = 105
    Height = 25
    Caption = 'initSize'
    TabOrder = 2
  end
  object Panel4: TPanel
    Left = 32
    Top = 141
    Width = 105
    Height = 25
    Caption = 'maxSize'
    TabOrder = 3
  end
  object Panel5: TPanel
    Left = 32
    Top = 172
    Width = 105
    Height = 25
    Caption = 'nextSize'
    TabOrder = 4
  end
  object Panel6: TPanel
    Left = 32
    Top = 203
    Width = 105
    Height = 25
    Caption = 'autoExtend'
    TabOrder = 5
  end
  object SPACENAME: TEdit
    Left = 143
    Top = 19
    Width = 154
    Height = 19
    Ctl3D = False
    ImeName = 'Microsoft IME 2003'
    ParentCtl3D = False
    TabOrder = 6
  end
  object DATAFILE: TEdit
    Left = 143
    Top = 50
    Width = 418
    Height = 19
    Ctl3D = False
    ImeName = 'Microsoft IME 2003'
    ParentCtl3D = False
    TabOrder = 7
  end
  object INITSIZE: TEdit
    Left = 143
    Top = 114
    Width = 121
    Height = 19
    Ctl3D = False
    ImeName = 'Microsoft IME 2003'
    ParentCtl3D = False
    TabOrder = 8
    Text = '1000'
  end
  object MAXSIZE: TEdit
    Left = 143
    Top = 143
    Width = 121
    Height = 19
    Ctl3D = False
    ImeName = 'Microsoft IME 2003'
    ParentCtl3D = False
    TabOrder = 9
    Text = '2000'
  end
  object EXTENDSIZE: TEdit
    Left = 143
    Top = 174
    Width = 121
    Height = 19
    Ctl3D = False
    ImeName = 'Microsoft IME 2003'
    ParentCtl3D = False
    TabOrder = 10
    Text = '10000'
  end
  object AUTOEXTEND: TCheckBox
    Left = 144
    Top = 208
    Width = 137
    Height = 17
    Caption = 'autoextend on'
    TabOrder = 11
  end
  object Button1: TButton
    Left = 268
    Top = 256
    Width = 75
    Height = 25
    Caption = 'Create'
    TabOrder = 12
    OnClick = Button1Click
  end
  object Button2: TButton
    Left = 567
    Top = 47
    Width = 33
    Height = 25
    Caption = '+'
    TabOrder = 13
    OnClick = Button2Click
  end
  object ComboBox1: TComboBox
    Left = 144
    Top = 72
    Width = 417
    Height = 21
    Ctl3D = False
    ImeName = 'Microsoft IME 2003'
    ItemHeight = 13
    ParentCtl3D = False
    TabOrder = 14
    OnDblClick = ComboBox1DblClick
  end
end
