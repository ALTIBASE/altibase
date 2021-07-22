object Form6: TForm6
  Left = 0
  Top = 0
  Caption = 'SQLView'
  ClientHeight = 507
  ClientWidth = 749
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  Menu = MainMenu1
  OldCreateOrder = False
  OnShow = FormShow
  PixelsPerInch = 96
  TextHeight = 13
  object Panel1: TPanel
    Left = 0
    Top = 0
    Width = 749
    Height = 41
    Align = alTop
    BevelInner = bvLowered
    BevelKind = bkSoft
    Ctl3D = False
    ParentCtl3D = False
    TabOrder = 0
    object Label1: TLabel
      Left = 12
      Top = 12
      Width = 20
      Height = 13
      Caption = 'DSN'
    end
    object Button1: TButton
      Left = 191
      Top = 6
      Width = 75
      Height = 25
      Caption = 'Execute'
      TabOrder = 0
      OnClick = Button1Click
    end
    object DSNLIST: TComboBox
      Left = 41
      Top = 9
      Width = 145
      Height = 21
      BevelInner = bvLowered
      Ctl3D = False
      ImeName = 'Microsoft IME 2003'
      ItemHeight = 13
      ParentCtl3D = False
      TabOrder = 1
    end
  end
  object Memo1: TMemo
    Left = 0
    Top = 360
    Width = 749
    Height = 147
    Align = alBottom
    BevelInner = bvLowered
    BevelKind = bkTile
    Color = clSilver
    Ctl3D = False
    Font.Charset = HANGEUL_CHARSET
    Font.Color = clWindowText
    Font.Height = -12
    Font.Name = #49352#44404#47548
    Font.Style = []
    ImeName = 'Microsoft IME 2003'
    ParentCtl3D = False
    ParentFont = False
    PopupMenu = PopupMenu1
    ReadOnly = True
    ScrollBars = ssBoth
    TabOrder = 1
    OnDblClick = Memo1DblClick
    OnMouseDown = Memo1MouseDown
    OnMouseMove = Memo1MouseMove
    ExplicitTop = 340
  end
  object RichEdit2: TRichEdit
    Left = 336
    Top = 200
    Width = 249
    Height = 89
    Font.Charset = HANGEUL_CHARSET
    Font.Color = clWindowText
    Font.Height = -11
    Font.Name = 'Tahoma'
    Font.Style = []
    ImeName = 'Microsoft IME 2003'
    Lines.Strings = (
      'RichEdit2')
    ParentFont = False
    TabOrder = 2
    Visible = False
  end
  object PageControl1: TPageControl
    Left = 0
    Top = 41
    Width = 749
    Height = 319
    Align = alClient
    PopupMenu = PopupMenu2
    TabOrder = 3
    ExplicitLeft = 208
    ExplicitTop = 25
  end
  object MainMenu1: TMainMenu
    Left = 496
    Top = 8
    object Menu1: TMenuItem
      Caption = 'Menu'
      object DSNreload1: TMenuItem
        Caption = 'DSN reload'
        OnClick = DSNreload1Click
      end
      object New1: TMenuItem
        Caption = 'New File'
        OnClick = New1Click
      end
      object OpenFrom: TMenuItem
        Caption = 'Open File'
        OnClick = OpenFromClick
      end
      object SaveFile1: TMenuItem
        Caption = 'Save File'
        OnClick = SaveFile1Click
      end
      object CloseFile1: TMenuItem
        Caption = 'Close File'
        OnClick = CloseFile1Click
      end
      object Exit1: TMenuItem
        Caption = 'Exit'
        OnClick = Exit1Click
      end
    end
  end
  object OpenDialog1: TOpenDialog
    Left = 528
    Top = 8
  end
  object SaveDialog1: TSaveDialog
    Left = 560
    Top = 8
  end
  object PopupMenu1: TPopupMenu
    Left = 568
    Top = 352
    object ClearMessage1: TMenuItem
      Caption = 'Clear Message'
      OnClick = ClearMessage1Click
    end
    object SaveMessage1: TMenuItem
      Caption = 'Save Message'
      OnClick = SaveMessage1Click
    end
  end
  object PopupMenu2: TPopupMenu
    Left = 568
    Top = 96
    object execute1: TMenuItem
      Caption = 'Execute'
      OnClick = execute1Click
    end
    object Newfile1: TMenuItem
      Caption = 'New File'
      OnClick = Newfile1Click
    end
    object Save1: TMenuItem
      Caption = 'Open File'
      OnClick = Save1Click
    end
    object Save2: TMenuItem
      Caption = 'Save File'
      OnClick = Save2Click
    end
    object CloseFile2: TMenuItem
      Caption = 'Close File'
      OnClick = CloseFile2Click
    end
    object Copy1: TMenuItem
      Caption = 'Copy'
      OnClick = Copy1Click
    end
    object Paste1: TMenuItem
      Caption = 'Paste'
      OnClick = Paste1Click
    end
  end
end
