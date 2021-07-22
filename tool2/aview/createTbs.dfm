object Form9: TForm9
  Left = 0
  Top = 0
  BorderIcons = [biSystemMenu, biMinimize]
  BorderStyle = bsSingle
  Caption = 'CreateTablespace'
  ClientHeight = 333
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
    Top = 158
    Width = 35
    Height = 13
    Caption = 'Mbyte'
  end
  object Label2: TLabel
    Left = 268
    Top = 188
    Width = 35
    Height = 13
    Caption = 'Mbyte'
  end
  object Label3: TLabel
    Left = 268
    Top = 218
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
    Top = 150
    Width = 105
    Height = 25
    Caption = 'initSize'
    TabOrder = 2
  end
  object Panel4: TPanel
    Left = 32
    Top = 181
    Width = 105
    Height = 25
    Caption = 'maxSize'
    TabOrder = 3
  end
  object Panel5: TPanel
    Left = 32
    Top = 212
    Width = 105
    Height = 25
    Caption = 'nextSize'
    TabOrder = 4
  end
  object Panel6: TPanel
    Left = 32
    Top = 243
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
    Top = 154
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
    Top = 183
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
    Top = 214
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
    Top = 248
    Width = 137
    Height = 17
    Caption = 'autoextend on'
    TabOrder = 11
  end
  object Button1: TButton
    Left = 268
    Top = 284
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
  object ListBox1: TListBox
    Left = 32
    Top = 80
    Width = 568
    Height = 65
    Ctl3D = False
    ImeName = 'Microsoft IME 2003'
    ItemHeight = 13
    ParentCtl3D = False
    TabOrder = 14
    OnDblClick = ListBox1DblClick
  end
end
