object SQLVIEW: TSQLVIEW
  Left = 0
  Top = 0
  Caption = 'SQLViewer'
  ClientHeight = 535
  ClientWidth = 706
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  OldCreateOrder = False
  PixelsPerInch = 96
  TextHeight = 13
  object SQLTEXT: TMemo
    Left = 0
    Top = 0
    Width = 706
    Height = 185
    Align = alTop
    Ctl3D = False
    ImeName = 'Microsoft IME 2003'
    ParentCtl3D = False
    ScrollBars = ssBoth
    TabOrder = 0
  end
  object PLANTEXT: TMemo
    Left = 0
    Top = 185
    Width = 706
    Height = 168
    Align = alTop
    Ctl3D = False
    ImeName = 'Microsoft IME 2003'
    ParentCtl3D = False
    ScrollBars = ssBoth
    TabOrder = 1
  end
end
