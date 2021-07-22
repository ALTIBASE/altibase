object Form1: TForm1
  Left = 0
  Top = 0
  Caption = 'Result'
  ClientHeight = 540
  ClientWidth = 785
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  OldCreateOrder = False
  OnClose = FormClose
  PixelsPerInch = 96
  TextHeight = 13
  object DSN: TPanel
    Left = 0
    Top = 499
    Width = 785
    Height = 41
    Align = alBottom
    BevelInner = bvLowered
    TabOrder = 0
  end
  object DataGrid: TStringGrid
    Left = 0
    Top = 41
    Width = 785
    Height = 306
    Align = alClient
    BevelKind = bkFlat
    Ctl3D = False
    FixedCols = 0
    Options = [goFixedVertLine, goFixedHorzLine, goVertLine, goHorzLine, goRangeSelect, goRowSizing, goColSizing, goRowMoving, goColMoving, goRowSelect]
    ParentCtl3D = False
    TabOrder = 1
  end
  object Panel1: TPanel
    Left = 0
    Top = 0
    Width = 785
    Height = 41
    Align = alTop
    BevelInner = bvLowered
    BevelKind = bkSoft
    Ctl3D = False
    ParentCtl3D = False
    TabOrder = 2
    object NEXTFETCH: TButton
      Left = 14
      Top = 7
      Width = 75
      Height = 25
      Caption = 'Next'
      TabOrder = 0
      OnClick = NEXTFETCHClick
    end
    object Button1: TButton
      Left = 95
      Top = 7
      Width = 75
      Height = 25
      Caption = 'SaveFile'
      TabOrder = 1
      OnClick = Button1Click
    end
  end
  object RichEdit1: TRichEdit
    Left = 0
    Top = 347
    Width = 785
    Height = 152
    Align = alBottom
    BevelInner = bvLowered
    BevelKind = bkSoft
    Color = clSilver
    Ctl3D = False
    Font.Charset = HANGEUL_CHARSET
    Font.Color = clWindowText
    Font.Height = -11
    Font.Name = 'Tahoma'
    Font.Style = []
    HideScrollBars = False
    ImeName = 'Microsoft IME 2003'
    ParentCtl3D = False
    ParentFont = False
    ReadOnly = True
    ScrollBars = ssBoth
    TabOrder = 3
    OnMouseDown = RichEdit1MouseDown
    OnMouseMove = RichEdit1MouseMove
  end
  object SaveDialog1: TSaveDialog
    Left = 728
    Top = 16
  end
end
