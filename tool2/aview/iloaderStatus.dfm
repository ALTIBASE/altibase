object Form8: TForm8
  Left = 0
  Top = 0
  Caption = 'iLoaderStatus'
  ClientHeight = 380
  ClientWidth = 771
  Color = clBtnFace
  Font.Charset = ANSI_CHARSET
  Font.Color = clWindowText
  Font.Height = -13
  Font.Name = #44404#47548#52404
  Font.Style = []
  OldCreateOrder = False
  OnCloseQuery = FormCloseQuery
  OnShow = FormShow
  PixelsPerInch = 96
  TextHeight = 13
  object CheckListBox1: TCheckListBox
    Left = 0
    Top = 65
    Width = 771
    Height = 315
    Align = alClient
    Ctl3D = False
    ImeName = 'Microsoft IME 2003'
    ItemHeight = 13
    ParentCtl3D = False
    TabOrder = 0
    TabWidth = 1
    OnDblClick = CheckListBox1DblClick
    ExplicitTop = 82
    ExplicitHeight = 288
  end
  object Panel1: TPanel
    Left = 0
    Top = 0
    Width = 771
    Height = 41
    Align = alTop
    BevelInner = bvLowered
    TabOrder = 1
    ExplicitTop = 41
    object Button1: TButton
      Left = 8
      Top = 8
      Width = 75
      Height = 25
      Hint = 
        'if you do not clear log ended,  resource is insufficient to run ' +
        'export data.'
      Caption = 'Clear'
      ParentShowHint = False
      ShowHint = True
      TabOrder = 0
      OnClick = Button1Click
    end
  end
  object Panel2: TPanel
    Left = 0
    Top = 41
    Width = 771
    Height = 24
    Align = alTop
    Alignment = taLeftJustify
    BevelInner = bvLowered
    TabOrder = 2
  end
  object Memo1: TMemo
    Left = 144
    Top = 71
    Width = 489
    Height = 290
    ImeName = 'Microsoft IME 2003'
    Lines.Strings = (
      'Memo1')
    ScrollBars = ssBoth
    TabOrder = 3
    Visible = False
    OnDblClick = Memo1DblClick
  end
end
