object Form4: TForm4
  Left = 0
  Top = 0
  Caption = 'Form4'
  ClientHeight = 522
  ClientWidth = 637
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
  object PageControl1: TPageControl
    Left = 112
    Top = 19
    Width = 525
    Height = 484
    ActivePage = TabSheet1
    Align = alRight
    PopupMenu = PopupMenu2
    TabOrder = 2
    TabPosition = tpBottom
    object TabSheet1: TTabSheet
      object PageControl11: TPageControl
        Left = 0
        Top = 168
        Width = 517
        Height = 290
        ActivePage = TabSheet1Data
        Align = alBottom
        TabOrder = 0
        object TabSheet1Data: TTabSheet
          Caption = 'Data'
          object DBGrid11: TDBGrid
            Left = 0
            Top = 0
            Width = 537
            Height = 259
            ImeName = 'Microsoft IME 2003'
            Options = [dgEditing, dgAlwaysShowEditor, dgTitles, dgIndicator, dgColumnResize, dgColLines, dgRowLines, dgTabs, dgConfirmDelete, dgCancelOnExit]
            TabOrder = 0
            TitleFont.Charset = DEFAULT_CHARSET
            TitleFont.Color = clWindowText
            TitleFont.Height = -11
            TitleFont.Name = 'Tahoma'
            TitleFont.Style = []
          end
        end
        object TabSheet1Message: TTabSheet
          Caption = 'Message'
          ImageIndex = 1
          object ListBox11: TListBox
            Left = 0
            Top = 0
            Width = 537
            Height = 259
            ImeName = 'Microsoft IME 2003'
            ItemHeight = 13
            TabOrder = 0
          end
        end
        object TabSheet1Plan: TTabSheet
          Caption = 'Explain Plan'
          ImageIndex = 2
          object Memo11: TMemo
            Left = 0
            Top = 0
            Width = 537
            Height = 259
            ImeName = 'Microsoft IME 2003'
            TabOrder = 0
          end
        end
        object TabSheet1History: TTabSheet
          Caption = 'History'
          ImageIndex = 3
          object ListBox12: TListBox
            Left = 3
            Top = 0
            Width = 531
            Height = 259
            Color = clInfoBk
            ImeName = 'Microsoft IME 2003'
            ItemHeight = 13
            TabOrder = 0
          end
        end
      end
      object RichEdit11: TRichEdit
        Left = 0
        Top = 0
        Width = 517
        Height = 148
        Align = alTop
        Font.Charset = HANGEUL_CHARSET
        Font.Color = clWindowText
        Font.Height = -11
        Font.Name = 'Tahoma'
        Font.Style = []
        ImeName = 'Microsoft IME 2003'
        ParentFont = False
        TabOrder = 1
      end
    end
    object TabSheet2: TTabSheet
      Caption = 'TabSheet2'
      ImageIndex = 1
      TabVisible = False
      object RichEdit21: TRichEdit
        Left = 0
        Top = 0
        Width = 517
        Height = 148
        Align = alTop
        Font.Charset = HANGEUL_CHARSET
        Font.Color = clWindowText
        Font.Height = -11
        Font.Name = 'Tahoma'
        Font.Style = []
        ImeName = 'Microsoft IME 2003'
        ParentFont = False
        TabOrder = 0
      end
      object PageControl21: TPageControl
        Left = 0
        Top = 168
        Width = 517
        Height = 290
        ActivePage = TabSheet2History
        Align = alBottom
        TabOrder = 1
        object TabSheet2Data: TTabSheet
          Caption = 'Data'
          object DBGrid21: TDBGrid
            Left = 0
            Top = 0
            Width = 537
            Height = 259
            ImeName = 'Microsoft IME 2003'
            Options = [dgEditing, dgAlwaysShowEditor, dgTitles, dgIndicator, dgColumnResize, dgColLines, dgRowLines, dgTabs, dgConfirmDelete, dgCancelOnExit]
            TabOrder = 0
            TitleFont.Charset = DEFAULT_CHARSET
            TitleFont.Color = clWindowText
            TitleFont.Height = -11
            TitleFont.Name = 'Tahoma'
            TitleFont.Style = []
          end
        end
        object TabSheet2Message: TTabSheet
          Caption = 'Message'
          ImageIndex = 1
          object ListBox21: TListBox
            Left = 0
            Top = 0
            Width = 537
            Height = 259
            ImeName = 'Microsoft IME 2003'
            ItemHeight = 13
            TabOrder = 0
          end
        end
        object TabSheet2Plan: TTabSheet
          Caption = 'Explain Plan'
          ImageIndex = 2
          object Memo21: TMemo
            Left = 0
            Top = 0
            Width = 537
            Height = 259
            ImeName = 'Microsoft IME 2003'
            TabOrder = 0
          end
        end
        object TabSheet2History: TTabSheet
          Caption = 'History'
          ImageIndex = 3
          object ListBox22: TListBox
            Left = 3
            Top = 0
            Width = 531
            Height = 259
            Color = clInfoBk
            ImeName = 'Microsoft IME 2003'
            ItemHeight = 13
            TabOrder = 0
          end
        end
      end
    end
    object TabSheet3: TTabSheet
      Caption = 'TabSheet3'
      ImageIndex = 2
      TabVisible = False
      object RichEdit31: TRichEdit
        Left = 0
        Top = 0
        Width = 517
        Height = 148
        Align = alTop
        Font.Charset = HANGEUL_CHARSET
        Font.Color = clWindowText
        Font.Height = -11
        Font.Name = 'Tahoma'
        Font.Style = []
        ImeName = 'Microsoft IME 2003'
        ParentFont = False
        TabOrder = 0
      end
      object PageControl31: TPageControl
        Left = 0
        Top = 168
        Width = 517
        Height = 290
        ActivePage = TabSheet3History
        Align = alBottom
        TabOrder = 1
        object TabSheet3Data: TTabSheet
          Caption = 'Data'
          object DBGrid31: TDBGrid
            Left = 0
            Top = 0
            Width = 537
            Height = 259
            ImeName = 'Microsoft IME 2003'
            Options = [dgEditing, dgAlwaysShowEditor, dgTitles, dgIndicator, dgColumnResize, dgColLines, dgRowLines, dgTabs, dgConfirmDelete, dgCancelOnExit]
            TabOrder = 0
            TitleFont.Charset = DEFAULT_CHARSET
            TitleFont.Color = clWindowText
            TitleFont.Height = -11
            TitleFont.Name = 'Tahoma'
            TitleFont.Style = []
          end
        end
        object TabSheet3Message: TTabSheet
          Caption = 'Message'
          ImageIndex = 1
          object ListBox31: TListBox
            Left = 0
            Top = 0
            Width = 537
            Height = 259
            ImeName = 'Microsoft IME 2003'
            ItemHeight = 13
            TabOrder = 0
          end
        end
        object TabSheet3Plan: TTabSheet
          Caption = 'Explain Plan'
          ImageIndex = 2
          object Memo31: TMemo
            Left = 0
            Top = 0
            Width = 537
            Height = 259
            ImeName = 'Microsoft IME 2003'
            TabOrder = 0
          end
        end
        object TabSheet3History: TTabSheet
          Caption = 'History'
          ImageIndex = 3
          object ListBox32: TListBox
            Left = 3
            Top = 0
            Width = 531
            Height = 259
            ImeName = 'Microsoft IME 2003'
            ItemHeight = 13
            TabOrder = 0
          end
        end
      end
    end
    object TabSheet4: TTabSheet
      Caption = 'TabSheet4'
      ImageIndex = 3
      TabVisible = False
      object RichEdit41: TRichEdit
        Left = 0
        Top = 0
        Width = 517
        Height = 148
        Align = alTop
        Font.Charset = HANGEUL_CHARSET
        Font.Color = clWindowText
        Font.Height = -11
        Font.Name = 'Tahoma'
        Font.Style = []
        ImeName = 'Microsoft IME 2003'
        ParentFont = False
        TabOrder = 0
      end
      object PageControl41: TPageControl
        Left = 0
        Top = 168
        Width = 517
        Height = 290
        ActivePage = TabSheet4History
        Align = alBottom
        TabOrder = 1
        object TabSheet4Data: TTabSheet
          Caption = 'Data'
          object DBGrid41: TDBGrid
            Left = 0
            Top = 0
            Width = 537
            Height = 259
            ImeName = 'Microsoft IME 2003'
            Options = [dgEditing, dgAlwaysShowEditor, dgTitles, dgIndicator, dgColumnResize, dgColLines, dgRowLines, dgTabs, dgConfirmDelete, dgCancelOnExit]
            TabOrder = 0
            TitleFont.Charset = DEFAULT_CHARSET
            TitleFont.Color = clWindowText
            TitleFont.Height = -11
            TitleFont.Name = 'Tahoma'
            TitleFont.Style = []
          end
        end
        object TabSheet4Message: TTabSheet
          Caption = 'Message'
          ImageIndex = 1
          object ListBox41: TListBox
            Left = 0
            Top = 0
            Width = 537
            Height = 259
            ImeName = 'Microsoft IME 2003'
            ItemHeight = 13
            TabOrder = 0
          end
        end
        object TabSheet4Plan: TTabSheet
          Caption = 'Explain Plan'
          ImageIndex = 2
          object Memo41: TMemo
            Left = 0
            Top = 0
            Width = 537
            Height = 259
            ImeName = 'Microsoft IME 2003'
            TabOrder = 0
          end
        end
        object TabSheet4History: TTabSheet
          Caption = 'History'
          ImageIndex = 3
          object ListBox42: TListBox
            Left = 3
            Top = 0
            Width = 531
            Height = 259
            Color = clInfoBk
            ImeName = 'Microsoft IME 2003'
            ItemHeight = 13
            TabOrder = 0
          end
        end
      end
    end
    object TabSheet5: TTabSheet
      Caption = 'TabSheet5'
      ImageIndex = 4
      TabVisible = False
      object RichEdit51: TRichEdit
        Left = 0
        Top = 0
        Width = 517
        Height = 148
        Align = alTop
        Font.Charset = HANGEUL_CHARSET
        Font.Color = clWindowText
        Font.Height = -11
        Font.Name = 'Tahoma'
        Font.Style = []
        ImeName = 'Microsoft IME 2003'
        ParentFont = False
        TabOrder = 0
      end
      object PageControl51: TPageControl
        Left = 0
        Top = 168
        Width = 517
        Height = 290
        ActivePage = TabSheet5History
        Align = alBottom
        TabOrder = 1
        object TabSheet5Data: TTabSheet
          Caption = 'Data'
          object DBGrid51: TDBGrid
            Left = 0
            Top = 0
            Width = 537
            Height = 259
            ImeName = 'Microsoft IME 2003'
            Options = [dgEditing, dgAlwaysShowEditor, dgTitles, dgIndicator, dgColumnResize, dgColLines, dgRowLines, dgTabs, dgConfirmDelete, dgCancelOnExit]
            TabOrder = 0
            TitleFont.Charset = DEFAULT_CHARSET
            TitleFont.Color = clWindowText
            TitleFont.Height = -11
            TitleFont.Name = 'Tahoma'
            TitleFont.Style = []
          end
        end
        object TabSheet5Message: TTabSheet
          Caption = 'Message'
          ImageIndex = 1
          object ListBox51: TListBox
            Left = 0
            Top = 0
            Width = 537
            Height = 259
            ImeName = 'Microsoft IME 2003'
            ItemHeight = 13
            TabOrder = 0
          end
        end
        object TabSheet5Plan: TTabSheet
          Caption = 'Explain Plan'
          ImageIndex = 2
          object Memo51: TMemo
            Left = 0
            Top = 0
            Width = 537
            Height = 259
            ImeName = 'Microsoft IME 2003'
            TabOrder = 0
          end
        end
        object TabSheet5History: TTabSheet
          Caption = 'History'
          ImageIndex = 3
          object ListBox52: TListBox
            Left = 3
            Top = 0
            Width = 531
            Height = 259
            Color = clInfoBk
            ImeName = 'Microsoft IME 2003'
            ItemHeight = 13
            TabOrder = 0
          end
        end
      end
    end
    object TabSheet6: TTabSheet
      Caption = 'TabSheet6'
      ImageIndex = 5
      TabVisible = False
      object RichEdit61: TRichEdit
        Left = 0
        Top = 0
        Width = 517
        Height = 148
        Align = alTop
        Font.Charset = HANGEUL_CHARSET
        Font.Color = clWindowText
        Font.Height = -11
        Font.Name = 'Tahoma'
        Font.Style = []
        ImeName = 'Microsoft IME 2003'
        ParentFont = False
        TabOrder = 0
      end
      object PageControl61: TPageControl
        Left = 0
        Top = 168
        Width = 517
        Height = 290
        ActivePage = TabSheet6History
        Align = alBottom
        TabOrder = 1
        object TabSheet6Data: TTabSheet
          Caption = 'Data'
          object DBGrid61: TDBGrid
            Left = 0
            Top = 0
            Width = 537
            Height = 259
            ImeName = 'Microsoft IME 2003'
            Options = [dgEditing, dgAlwaysShowEditor, dgTitles, dgIndicator, dgColumnResize, dgColLines, dgRowLines, dgTabs, dgConfirmDelete, dgCancelOnExit]
            TabOrder = 0
            TitleFont.Charset = DEFAULT_CHARSET
            TitleFont.Color = clWindowText
            TitleFont.Height = -11
            TitleFont.Name = 'Tahoma'
            TitleFont.Style = []
          end
        end
        object TabSheet6Message: TTabSheet
          Caption = 'Message'
          ImageIndex = 1
          object ListBox61: TListBox
            Left = 0
            Top = 0
            Width = 537
            Height = 259
            ImeName = 'Microsoft IME 2003'
            ItemHeight = 13
            TabOrder = 0
          end
        end
        object TabSheet6Plan: TTabSheet
          Caption = 'Explain Plan'
          ImageIndex = 2
          object Memo61: TMemo
            Left = 0
            Top = 0
            Width = 537
            Height = 259
            ImeName = 'Microsoft IME 2003'
            TabOrder = 0
          end
        end
        object TabSheet6History: TTabSheet
          Caption = 'History'
          ImageIndex = 3
          object ListBox62: TListBox
            Left = 3
            Top = 0
            Width = 531
            Height = 259
            Color = clInfoBk
            ImeName = 'Microsoft IME 2003'
            ItemHeight = 13
            TabOrder = 0
          end
        end
      end
    end
    object TabSheet7: TTabSheet
      Caption = 'TabSheet7'
      ImageIndex = 6
      TabVisible = False
      object RichEdit71: TRichEdit
        Left = 0
        Top = 0
        Width = 517
        Height = 148
        Align = alTop
        Font.Charset = HANGEUL_CHARSET
        Font.Color = clWindowText
        Font.Height = -11
        Font.Name = 'Tahoma'
        Font.Style = []
        ImeName = 'Microsoft IME 2003'
        ParentFont = False
        TabOrder = 0
      end
      object PageControl71: TPageControl
        Left = 0
        Top = 168
        Width = 517
        Height = 290
        ActivePage = TabSheet7History
        Align = alBottom
        TabOrder = 1
        object TabSheet7Data: TTabSheet
          Caption = 'Data'
          object DBGrid71: TDBGrid
            Left = 0
            Top = 0
            Width = 537
            Height = 259
            ImeName = 'Microsoft IME 2003'
            Options = [dgEditing, dgAlwaysShowEditor, dgTitles, dgIndicator, dgColumnResize, dgColLines, dgRowLines, dgTabs, dgConfirmDelete, dgCancelOnExit]
            TabOrder = 0
            TitleFont.Charset = DEFAULT_CHARSET
            TitleFont.Color = clWindowText
            TitleFont.Height = -11
            TitleFont.Name = 'Tahoma'
            TitleFont.Style = []
          end
        end
        object TabSheet7Message: TTabSheet
          Caption = 'Message'
          ImageIndex = 1
          object ListBox71: TListBox
            Left = 0
            Top = 0
            Width = 537
            Height = 259
            ImeName = 'Microsoft IME 2003'
            ItemHeight = 13
            TabOrder = 0
          end
        end
        object TabSheet7Plan: TTabSheet
          Caption = 'Explain Plan'
          ImageIndex = 2
          object Memo71: TMemo
            Left = 0
            Top = 0
            Width = 537
            Height = 259
            ImeName = 'Microsoft IME 2003'
            TabOrder = 0
          end
        end
        object TabSheet7History: TTabSheet
          Caption = 'History'
          ImageIndex = 3
          object ListBox72: TListBox
            Left = 3
            Top = 0
            Width = 531
            Height = 259
            Color = clInfoBk
            ImeName = 'Microsoft IME 2003'
            ItemHeight = 13
            TabOrder = 0
          end
        end
      end
    end
  end
  object StatusBar1: TStatusBar
    Left = 0
    Top = 503
    Width = 637
    Height = 19
    Panels = <>
  end
  object ToolBar1: TToolBar
    Left = 0
    Top = 0
    Width = 637
    Height = 19
    AutoSize = True
    ButtonHeight = 19
    ButtonWidth = 70
    Caption = 'ToolBar1'
    List = True
    ShowCaptions = True
    TabOrder = 0
    object ToolButton1: TToolButton
      Left = 0
      Top = 0
      AutoSize = True
      Caption = 'ToolButton1'
      ImageIndex = 0
      OnClick = ToolButton1Click
    end
    object ToolButton2: TToolButton
      Left = 74
      Top = 0
      Caption = 'ToolButton2'
      ImageIndex = 1
      OnClick = ToolButton2Click
    end
    object ToolButton3: TToolButton
      Left = 144
      Top = 0
      Caption = 'ToolButton3'
      ImageIndex = 2
      OnClick = ToolButton3Click
    end
  end
  object ListBox1: TListBox
    Left = 0
    Top = 19
    Width = 113
    Height = 484
    Align = alLeft
    ImeName = 'Microsoft IME 2003'
    ItemHeight = 13
    PopupMenu = PopupMenu1
    TabOrder = 3
  end
  object MainMenu1: TMainMenu
    Left = 512
    Top = 8
    object File1: TMenuItem
      Caption = 'File'
      object DSNManager1: TMenuItem
        Caption = 'DSN Manager'
        OnClick = DSNManager1Click
      end
      object Connect1: TMenuItem
        Caption = 'Connect'
      end
      object Disconnect1: TMenuItem
        Caption = 'Disconnect'
      end
      object AllDisconnect1: TMenuItem
        Caption = 'All Disconnect'
      end
      object N1: TMenuItem
        Caption = '-'
      end
      object Open1: TMenuItem
        Caption = 'Open'
      end
      object Save1: TMenuItem
        Caption = 'Save'
      end
      object Print1: TMenuItem
        Caption = 'Print'
      end
      object Exit1: TMenuItem
        Caption = 'Exit'
      end
    end
    object Edit1: TMenuItem
      Caption = 'Edit'
    end
    object Query: TMenuItem
      Caption = 'Query'
    end
    object ransaction1: TMenuItem
      Caption = 'Transaction'
    end
    object Help1: TMenuItem
      Caption = 'Help'
    end
  end
  object Query1: TQuery
    Left = 480
    Top = 8
  end
  object Database1: TDatabase
    SessionName = 'Default'
    Left = 448
    Top = 8
  end
  object DataSource1: TDataSource
    Left = 416
    Top = 8
  end
  object Database2: TDatabase
    SessionName = 'Default'
    Left = 448
    Top = 40
  end
  object Database3: TDatabase
    SessionName = 'Default'
    Left = 448
    Top = 72
  end
  object Database4: TDatabase
    SessionName = 'Default'
    Left = 448
    Top = 104
  end
  object Database5: TDatabase
    SessionName = 'Default'
    Left = 448
    Top = 136
  end
  object Database6: TDatabase
    SessionName = 'Default'
    Left = 448
    Top = 168
  end
  object Database7: TDatabase
    SessionName = 'Default'
    Left = 448
    Top = 200
  end
  object Query2: TQuery
    Left = 480
    Top = 40
  end
  object Query3: TQuery
    Left = 480
    Top = 72
  end
  object Query4: TQuery
    Left = 480
    Top = 104
  end
  object Query5: TQuery
    Left = 480
    Top = 136
  end
  object Query6: TQuery
    Left = 480
    Top = 168
  end
  object Query7: TQuery
    Left = 480
    Top = 200
  end
  object DataSource2: TDataSource
    Left = 416
    Top = 40
  end
  object DataSource3: TDataSource
    Left = 416
    Top = 72
  end
  object DataSource4: TDataSource
    Left = 416
    Top = 104
  end
  object DataSource5: TDataSource
    Left = 416
    Top = 136
  end
  object DataSource6: TDataSource
    Left = 416
    Top = 168
  end
  object DataSource7: TDataSource
    Left = 416
    Top = 200
  end
  object PopupMenu1: TPopupMenu
    Left = 512
    Top = 40
    object Connect2: TMenuItem
      Caption = 'Connect'
      OnClick = Connect2Click
    end
    object Disconnect2: TMenuItem
      Caption = 'Disconnect'
      OnClick = Disconnect2Click
    end
  end
  object PopupMenu2: TPopupMenu
    Left = 512
    Top = 72
    object Disconnect3: TMenuItem
      Caption = 'Disconnect'
      OnClick = Disconnect3Click
    end
  end
end
