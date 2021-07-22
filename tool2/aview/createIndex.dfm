object Form7: TForm7
  Left = 0
  Top = 0
  BorderIcons = [biSystemMenu, biMinimize]
  BorderStyle = bsSingle
  Caption = 'CreateIndex'
  ClientHeight = 499
  ClientWidth = 550
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
    Left = 0
    Top = 0
    Width = 550
    Height = 92
    Align = alTop
    BevelInner = bvLowered
    TabOrder = 0
    object Label1: TLabel
      Left = 8
      Top = 37
      Width = 63
      Height = 13
      Caption = 'IndexName'
    end
    object Label2: TLabel
      Left = 8
      Top = 15
      Width = 63
      Height = 13
      Caption = 'TableName'
    end
    object Button3: TButton
      Left = 441
      Top = 61
      Width = 97
      Height = 25
      Caption = 'CreateIndex'
      TabOrder = 0
      OnClick = Button3Click
    end
    object IndexName: TEdit
      Left = 77
      Top = 37
      Width = 121
      Height = 19
      Ctl3D = False
      ImeName = 'Microsoft IME 2003'
      MaxLength = 40
      ParentCtl3D = False
      TabOrder = 1
    end
    object CheckBox1: TCheckBox
      Left = 101
      Top = 62
      Width = 97
      Height = 17
      Caption = 'UniqueIndex'
      Ctl3D = False
      ParentCtl3D = False
      TabOrder = 2
    end
    object TARGET: TEdit
      Left = 77
      Top = 12
      Width = 121
      Height = 19
      Ctl3D = False
      ImeName = 'Microsoft IME 2003'
      ParentCtl3D = False
      ReadOnly = True
      TabOrder = 3
    end
    object CheckBox2: TCheckBox
      Left = 208
      Top = 62
      Width = 97
      Height = 17
      Caption = 'PK Index'
      TabOrder = 4
    end
  end
  object ColGrid: TStringGrid
    Left = 0
    Top = 92
    Width = 1
    Height = 407
    Align = alLeft
    ColCount = 3
    Ctl3D = False
    FixedColor = clSilver
    FixedCols = 0
    Options = [goFixedVertLine, goFixedHorzLine, goVertLine, goHorzLine, goDrawFocusSelected, goColSizing, goRowSelect]
    ParentCtl3D = False
    TabOrder = 1
    ColWidths = (
      124
      81
      74)
  end
  object Panel2: TPanel
    Left = 217
    Top = 92
    Width = 59
    Height = 407
    Align = alLeft
    BevelInner = bvLowered
    TabOrder = 2
    object Button1: TButton
      Left = 6
      Top = 16
      Width = 43
      Height = 25
      Caption = '->'
      TabOrder = 0
      OnClick = Button1Click
    end
    object Button2: TButton
      Left = 6
      Top = 47
      Width = 43
      Height = 25
      Caption = '<-'
      TabOrder = 1
      OnClick = Button2Click
    end
    object SERVERNAME: TEdit
      Left = 6
      Top = 240
      Width = 121
      Height = 21
      ImeName = 'Microsoft IME 2003'
      TabOrder = 2
      Visible = False
    end
    object USERS: TEdit
      Left = 8
      Top = 272
      Width = 121
      Height = 21
      ImeName = 'Microsoft IME 2003'
      TabOrder = 3
      Visible = False
    end
  end
  object IndexGrid: TStringGrid
    Left = 276
    Top = 92
    Width = 274
    Height = 407
    Hint = 'empty row is ignored'
    Align = alClient
    ColCount = 2
    Ctl3D = False
    Options = [goFixedVertLine, goFixedHorzLine, goVertLine, goHorzLine, goColSizing, goRowMoving, goEditing]
    ParentCtl3D = False
    ParentShowHint = False
    ShowHint = True
    TabOrder = 3
    ColWidths = (
      158
      95)
  end
  object ColGrid2: TCheckListBox
    Left = 1
    Top = 92
    Width = 216
    Height = 407
    Align = alLeft
    Columns = 1
    Ctl3D = False
    ImeName = 'Microsoft IME 2003'
    ItemHeight = 13
    ParentCtl3D = False
    TabOrder = 4
    TabWidth = 1
  end
end
