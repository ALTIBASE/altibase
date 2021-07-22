object Form5: TForm5
  Left = 0
  Top = 0
  Caption = 'AVIEW'
  ClientHeight = 728
  ClientWidth = 910
  Color = clBtnFace
  Font.Charset = ANSI_CHARSET
  Font.Color = clWindowText
  Font.Height = -13
  Font.Name = #44404#47548#52404
  Font.Style = []
  Menu = MainMenu1
  OldCreateOrder = False
  OnCloseQuery = FormCloseQuery
  OnShow = FormShow
  PixelsPerInch = 96
  TextHeight = 13
  object gMSG: TPanel
    Left = 0
    Top = 687
    Width = 910
    Height = 41
    Align = alBottom
    BevelInner = bvLowered
    TabOrder = 0
    ExplicitTop = 668
    object SERVERNAME: TPanel
      Left = 776
      Top = 2
      Width = 132
      Height = 37
      Align = alRight
      BevelInner = bvLowered
      Color = 65408
      TabOrder = 0
    end
  end
  object DBNODE: TTreeView
    Left = 0
    Top = 0
    Width = 257
    Height = 687
    Align = alLeft
    Color = clWhite
    Ctl3D = False
    Images = ImageList1
    Indent = 19
    MultiSelect = True
    MultiSelectStyle = [msControlSelect, msShiftSelect]
    ParentCtl3D = False
    TabOrder = 1
    OnClick = DBNODEClick
    OnDblClick = DBNODEDblClick
    OnGetImageIndex = DBNODEGetImageIndex
    ExplicitHeight = 668
  end
  object Panel1: TPanel
    Left = 257
    Top = 0
    Width = 653
    Height = 687
    Align = alClient
    BevelInner = bvLowered
    TabOrder = 2
    ExplicitHeight = 668
    object TNAMES: TPanel
      Left = 2
      Top = 2
      Width = 649
      Height = 31
      Align = alTop
      BevelInner = bvLowered
      Color = clGray
      Ctl3D = False
      Font.Charset = ANSI_CHARSET
      Font.Color = clWhite
      Font.Height = -13
      Font.Name = #44404#47548#52404
      Font.Style = [fsBold]
      ParentCtl3D = False
      ParentFont = False
      TabOrder = 0
    end
    object ColGrid: TStringGrid
      Left = 6
      Top = 39
      Width = 320
      Height = 74
      Ctl3D = False
      FixedCols = 0
      Options = [goFixedVertLine, goFixedHorzLine, goVertLine, goHorzLine, goRangeSelect, goColSizing, goRowSelect]
      ParentCtl3D = False
      PopupMenu = IndexPopup
      TabOrder = 1
      Visible = False
    end
    object IndexGrid: TStringGrid
      Left = 6
      Top = 119
      Width = 320
      Height = 76
      Ctl3D = False
      FixedCols = 0
      Options = [goFixedVertLine, goFixedHorzLine, goVertLine, goHorzLine, goRangeSelect, goColSizing, goRowSelect]
      ParentCtl3D = False
      PopupMenu = IndexPopup
      TabOrder = 2
      Visible = False
    end
    object DataGrid: TStringGrid
      Left = 6
      Top = 201
      Width = 320
      Height = 72
      Ctl3D = False
      FixedCols = 0
      Options = [goFixedVertLine, goFixedHorzLine, goVertLine, goHorzLine, goRangeSelect, goColSizing, goRowMoving, goColMoving, goRowSelect]
      ParentCtl3D = False
      TabOrder = 3
      Visible = False
      OnDblClick = DataGridDblClick
    end
    object CRTTBL_PANEL: TPanel
      Left = 6
      Top = 256
      Width = 651
      Height = 96
      BevelInner = bvLowered
      TabOrder = 4
      Visible = False
      object Label1: TLabel
        Left = 8
        Top = 47
        Width = 70
        Height = 13
        Caption = 'ColumnName'
      end
      object Label2: TLabel
        Left = 135
        Top = 47
        Width = 70
        Height = 13
        Caption = 'ColumnType'
      end
      object Label3: TLabel
        Left = 247
        Top = 47
        Width = 63
        Height = 13
        Caption = 'Precision'
      end
      object Label4: TLabel
        Left = 328
        Top = 47
        Width = 35
        Height = 13
        Caption = 'Scale'
      end
      object Label5: TLabel
        Left = 400
        Top = 47
        Width = 56
        Height = 13
        Caption = 'NullAble'
      end
      object Label6: TLabel
        Left = 471
        Top = 47
        Width = 91
        Height = 13
        Caption = 'Default Value'
      end
      object CNAME: TEdit
        Left = 8
        Top = 66
        Width = 121
        Height = 21
        ImeName = 'Microsoft IME 2003'
        MaxLength = 40
        TabOrder = 0
      end
      object CTYPE: TComboBox
        Left = 135
        Top = 66
        Width = 106
        Height = 21
        ImeName = 'Microsoft IME 2003'
        ItemHeight = 13
        TabOrder = 1
        Items.Strings = (
          'char'
          'varchar'
          'blob'
          'clob'
          'date'
          'integer'
          'double'
          'numeric'
          'number')
      end
      object CPRE: TEdit
        Left = 247
        Top = 66
        Width = 63
        Height = 21
        ImeName = 'Microsoft IME 2003'
        MaxLength = 40
        TabOrder = 2
      end
      object CSCALE: TEdit
        Left = 327
        Top = 66
        Width = 58
        Height = 21
        ImeName = 'Microsoft IME 2003'
        MaxLength = 40
        TabOrder = 3
      end
      object Button1: TButton
        Left = 607
        Top = 58
        Width = 25
        Height = 25
        Caption = '+'
        TabOrder = 4
        OnClick = Button1Click
      end
      object ISNULL: TComboBox
        Left = 400
        Top = 66
        Width = 65
        Height = 21
        ImeName = 'Microsoft IME 2003'
        ItemHeight = 13
        TabOrder = 5
        Items.Strings = (
          'NULL'
          'NOT NULL')
      end
      object CDEFAULT: TEdit
        Left = 472
        Top = 66
        Width = 113
        Height = 21
        ImeName = 'Microsoft IME 2003'
        MaxLength = 40
        TabOrder = 6
      end
      object Panel2: TPanel
        Left = 8
        Top = 8
        Width = 121
        Height = 25
        Caption = 'TableName'
        TabOrder = 7
      end
      object CRTNAME: TEdit
        Left = 133
        Top = 10
        Width = 177
        Height = 21
        ImeName = 'Microsoft IME 2003'
        MaxLength = 40
        TabOrder = 8
      end
      object Button10: TButton
        Left = 607
        Top = 27
        Width = 25
        Height = 25
        Caption = '-'
        TabOrder = 9
        OnClick = Button10Click
      end
    end
    object CRTTBL_BUTTON: TPanel
      Left = 6
      Top = 405
      Width = 539
      Height = 41
      TabOrder = 5
      Visible = False
      object Button2: TButton
        Left = 8
        Top = 8
        Width = 75
        Height = 25
        Caption = 'Create'
        TabOrder = 0
        OnClick = Button2Click
      end
      object Button3: TButton
        Left = 88
        Top = 8
        Width = 75
        Height = 25
        Caption = 'LoadFile'
        TabOrder = 1
        OnClick = Button3Click
      end
      object targetUser: TEdit
        Left = 408
        Top = 8
        Width = 121
        Height = 21
        ImeName = 'Microsoft IME 2003'
        TabOrder = 2
        Visible = False
      end
      object Button4: TButton
        Left = 168
        Top = 8
        Width = 75
        Height = 25
        Caption = 'AddSheet'
        TabOrder = 3
        OnClick = Button4Click
      end
    end
    object CRTMEMO: TMemo
      Left = 484
      Top = 39
      Width = 185
      Height = 89
      Ctl3D = False
      ImeName = 'Microsoft IME 2003'
      ParentCtl3D = False
      ScrollBars = ssBoth
      TabOrder = 6
      Visible = False
    end
    object FETCHPANEL: TPanel
      Left = 14
      Top = 358
      Width = 529
      Height = 41
      Alignment = taRightJustify
      TabOrder = 7
      Visible = False
      object NEXTFETCH: TButton
        Left = 8
        Top = 9
        Width = 75
        Height = 25
        Caption = 'Next'
        TabOrder = 0
        OnClick = NEXTFETCHClick
      end
      object Button6: TButton
        Left = 89
        Top = 9
        Width = 75
        Height = 25
        Caption = 'SaveTo'
        TabOrder = 1
        OnClick = Button6Click
      end
    end
    object PROC: TMemo
      Left = 332
      Top = 39
      Width = 185
      Height = 89
      Ctl3D = False
      ImeName = 'Microsoft IME 2003'
      ParentCtl3D = False
      ScrollBars = ssBoth
      TabOrder = 8
      Visible = False
    end
    object PROCPANEL: TPanel
      Left = 6
      Top = 452
      Width = 499
      Height = 41
      TabOrder = 9
      Visible = False
      object Button5: TButton
        Left = 8
        Top = 8
        Width = 75
        Height = 25
        Caption = 'Compile'
        TabOrder = 0
        OnClick = Button5Click
      end
      object Button7: TButton
        Left = 89
        Top = 8
        Width = 75
        Height = 25
        Caption = 'SaveTo'
        TabOrder = 1
        OnClick = Button7Click
      end
      object Button8: TButton
        Left = 168
        Top = 8
        Width = 75
        Height = 25
        Caption = 'LoadFrom'
        TabOrder = 2
        OnClick = Button8Click
      end
      object Button9: TButton
        Left = 248
        Top = 7
        Width = 75
        Height = 25
        Caption = 'Execute'
        TabOrder = 3
        OnClick = Button9Click
      end
    end
    object ProcGrid: TStringGrid
      Left = 332
      Top = 134
      Width = 229
      Height = 86
      Ctl3D = False
      FixedCols = 3
      Options = [goFixedVertLine, goFixedHorzLine, goVertLine, goHorzLine, goRangeSelect, goColSizing, goEditing]
      ParentCtl3D = False
      TabOrder = 10
      Visible = False
    end
    object TBS_PANEL: TPanel
      Left = 71
      Top = 55
      Width = 504
      Height = 297
      TabOrder = 11
      Visible = False
      object Panel3: TPanel
        Left = 1
        Top = 1
        Width = 502
        Height = 152
        Align = alTop
        BevelInner = bvLowered
        TabOrder = 0
        object TLabel
          Left = 136
          Top = 80
          Width = 7
          Height = 13
        end
        object RATE1: TLabel
          Left = 320
          Top = 104
          Width = 35
          Height = 13
          Caption = 'RATE1'
        end
        object ProgressBar1: TProgressBar
          Left = 2
          Top = 72
          Width = 498
          Height = 26
          Align = alTop
          TabOrder = 0
        end
        object Panel4: TPanel
          Left = 2
          Top = 2
          Width = 498
          Height = 70
          Align = alTop
          TabOrder = 1
          object Label7: TLabel
            Left = 8
            Top = 16
            Width = 49
            Height = 13
            Caption = 'MaxSize'
          end
          object Label8: TLabel
            Left = 8
            Top = 38
            Width = 77
            Height = 13
            Caption = 'CurrentSize'
          end
          object Label9: TLabel
            Left = 286
            Top = 16
            Width = 63
            Height = 13
            Caption = 'TableUsed'
          end
          object Label10: TLabel
            Left = 286
            Top = 39
            Width = 63
            Height = 13
            Caption = 'IndexUsed'
          end
          object Label11: TLabel
            Left = 246
            Top = 17
            Width = 28
            Height = 13
            Caption = 'byte'
          end
          object Label12: TLabel
            Left = 246
            Top = 39
            Width = 28
            Height = 13
            Caption = 'byte'
          end
          object Label13: TLabel
            Left = 481
            Top = 16
            Width = 28
            Height = 13
            Caption = 'byte'
          end
          object Label14: TLabel
            Left = 481
            Top = 38
            Width = 28
            Height = 13
            Caption = 'byte'
          end
          object TOTAL_SIZE: TEdit
            Left = 119
            Top = 12
            Width = 121
            Height = 19
            Ctl3D = False
            ImeName = 'Microsoft IME 2003'
            ParentCtl3D = False
            ReadOnly = True
            TabOrder = 0
          end
          object USED_SIZE: TEdit
            Left = 120
            Top = 36
            Width = 121
            Height = 19
            Ctl3D = False
            ImeName = 'Microsoft IME 2003'
            ParentCtl3D = False
            ReadOnly = True
            TabOrder = 1
          end
          object TABLE_USE: TEdit
            Left = 354
            Top = 12
            Width = 121
            Height = 19
            Ctl3D = False
            ImeName = 'Microsoft IME 2003'
            ParentCtl3D = False
            ReadOnly = True
            TabOrder = 2
          end
          object INDEX_USE: TEdit
            Left = 354
            Top = 35
            Width = 121
            Height = 19
            Ctl3D = False
            ImeName = 'Microsoft IME 2003'
            ParentCtl3D = False
            ReadOnly = True
            TabOrder = 3
          end
        end
      end
      object DATAFILE: TStringGrid
        Left = 1
        Top = 153
        Width = 502
        Height = 143
        Align = alClient
        FixedCols = 0
        Options = [goFixedVertLine, goFixedHorzLine, goVertLine, goHorzLine, goRangeSelect, goColSizing, goColMoving, goRowSelect]
        TabOrder = 1
        ColWidths = (
          188
          82
          88
          80
          64)
      end
    end
  end
  object MainMenu1: TMainMenu
    Left = 672
    Top = 392
    object Server1: TMenuItem
      Caption = 'Server'
      object AddDSN1: TMenuItem
        Caption = 'DSN Manager'
        OnClick = AddDSN1Click
      end
      object Connect1: TMenuItem
        Caption = 'Connect'
        OnClick = Connect1Click
      end
      object DisConnect1: TMenuItem
        Caption = 'DisConnect'
        OnClick = DisConnect1Click
      end
    end
    object DDL1: TMenuItem
      Caption = 'DDL'
      object createTable2: TMenuItem
        Caption = 'create Table'
        OnClick = createTable2Click
      end
      object createProcedure2: TMenuItem
        Caption = 'create Procedure'
        OnClick = createProcedure2Click
      end
      object createView2: TMenuItem
        Caption = 'create View'
        OnClick = createView2Click
      end
      object createTrigger1: TMenuItem
        Caption = 'create Trigger'
        OnClick = createTrigger1Click
      end
      object droptable2: TMenuItem
        Caption = 'drop table'
        OnClick = droptable2Click
      end
      object dropProcedure2: TMenuItem
        Caption = 'drop Procedure'
        OnClick = dropProcedure2Click
      end
      object dropView1: TMenuItem
        Caption = 'drop View'
        OnClick = dropView1Click
      end
      object dropTrigger2: TMenuItem
        Caption = 'drop Trigger'
        OnClick = dropTrigger2Click
      end
      object recompile4: TMenuItem
        Caption = 'recompile'
        OnClick = recompile4Click
      end
      object createIndex2: TMenuItem
        Caption = 'create Index'
        OnClick = createIndex2Click
      end
      object schemaout3: TMenuItem
        Caption = 'schema out'
        OnClick = schemaout3Click
      end
      object droptablespace1: TMenuItem
        Caption = 'drop tablespace'
        OnClick = droptablespace1Click
      end
    end
    object DML1: TMenuItem
      Caption = 'DML'
      object selectdata1: TMenuItem
        Caption = 'select data'
        OnClick = selectdata1Click
      end
      object testProcedure1: TMenuItem
        Caption = 'test Procedure'
        OnClick = testProcedure1Click
      end
    end
    object Management1: TMenuItem
      Caption = 'Management'
      object StartDBMS1: TMenuItem
        Caption = 'Start DBMS'
      end
      object StopDBMS1: TMenuItem
        Caption = 'Stop DBMS'
      end
      object Replication1: TMenuItem
        Caption = 'Replication'
      end
      object ExportData1: TMenuItem
        Caption = 'Export Data'
        OnClick = ExportData1Click
      end
      object ImportData1: TMenuItem
        Caption = 'Import Data'
        OnClick = ImportData1Click
      end
      object SchemaScript1: TMenuItem
        Caption = 'Schema Script'
        OnClick = SchemaScript1Click
      end
      object AddUser1: TMenuItem
        Caption = 'Add User'
        OnClick = AddUser1Click
      end
      object DropUser2: TMenuItem
        Caption = 'Drop User'
        OnClick = DropUser2Click
      end
      object iLoaderStatus1: TMenuItem
        Caption = 'iLoaderStatus'
        OnClick = iLoaderStatus1Click
      end
    end
    object SQL1: TMenuItem
      Caption = 'winISQL'
      object winISQL1: TMenuItem
        Caption = 'winISQL'
        OnClick = winISQL1Click
      end
      object UserSQL1: TMenuItem
        Caption = 'UserSQL'
      end
    end
    object Resource1: TMenuItem
      Caption = 'Resource'
      object System1: TMenuItem
        Caption = 'System'
        OnClick = System1Click
      end
      object Altibase1: TMenuItem
        Caption = 'Altibase'
      end
      object Information1: TMenuItem
        Caption = 'Information'
      end
      object Report1: TMenuItem
        Caption = 'Report'
      end
      object Pstack1: TMenuItem
        Caption = 'Pstack'
      end
      object SummaryMonitor1: TMenuItem
        Caption = 'SummaryMonitor'
      end
    end
    object Option1: TMenuItem
      Caption = 'Option'
      object imeout1: TMenuItem
        Caption = 'Timeout'
      end
      object CommitMode1: TMenuItem
        Caption = 'CommitMode'
      end
      object Delimeter1: TMenuItem
        Caption = 'Delimeter'
        OnClick = Delimeter1Click
      end
    end
    object About1: TMenuItem
      Caption = 'About'
      object Help1: TMenuItem
        Caption = 'Help'
      end
      object About2: TMenuItem
        Caption = 'About'
      end
    end
  end
  object TreePopup1: TPopupMenu
    Left = 632
    Top = 392
    object Connect2: TMenuItem
      Caption = 'Connect'
      OnClick = Connect2Click
    end
    object DisConnect2: TMenuItem
      Caption = 'DisConnect'
      OnClick = DisConnect2Click
    end
    object Start1: TMenuItem
      Caption = 'Start Altibase'
    end
    object StopAltibase1: TMenuItem
      Caption = 'Stop Altibase'
    end
    object userAdd1: TMenuItem
      Caption = 'userAdd'
      OnClick = userAdd1Click
    end
    object schemaOut1: TMenuItem
      Caption = 'schemaOut'
    end
    object ExportData2: TMenuItem
      Caption = 'Export Data'
      OnClick = ExportData2Click
    end
    object ImportData2: TMenuItem
      Caption = 'ImportData'
      OnClick = ImportData2Click
    end
  end
  object TreePopup2: TPopupMenu
    Left = 632
    Top = 360
    object dropUser1: TMenuItem
      Caption = 'dropUser'
      OnClick = dropUser1Click
    end
    object schemaOut2: TMenuItem
      Caption = 'schemaOut'
      OnClick = schemaOut2Click
    end
    object CreateTableSpace1: TMenuItem
      Caption = 'Create TableSpace'
      OnClick = CreateTableSpace1Click
    end
    object createTable1: TMenuItem
      Caption = 'create Table'
      OnClick = createTable1Click
    end
    object createProcedure1: TMenuItem
      Caption = 'create Procedure'
      OnClick = createProcedure1Click
    end
    object createView1: TMenuItem
      Caption = 'create View'
      OnClick = createView1Click
    end
    object create1: TMenuItem
      Caption = 'create Trigger'
      OnClick = create1Click
    end
  end
  object TreePopup3: TPopupMenu
    Left = 592
    Top = 360
    object truncate1: TMenuItem
      Caption = 'truncate'
      OnClick = truncate1Click
    end
    object droptable1: TMenuItem
      Caption = 'drop table'
      OnClick = droptable1Click
    end
    object selectTable1: TMenuItem
      Caption = 'select Table'
      OnClick = selectTable1Click
    end
    object alterTable1: TMenuItem
      Caption = 'alter Table'
      OnClick = alterTable1Click
    end
    object ScriptOut1: TMenuItem
      Caption = 'ScriptOut'
      OnClick = ScriptOut1Click
    end
    object exportData4: TMenuItem
      Caption = 'export Data'
      OnClick = exportData4Click
    end
    object importData4: TMenuItem
      Caption = 'import Data'
      OnClick = importData4Click
    end
  end
  object TreePopup4: TPopupMenu
    Left = 592
    Top = 392
    object ReCompile1: TMenuItem
      Caption = 'ReCompile'
      OnClick = ReCompile1Click
    end
    object DropObject1: TMenuItem
      Caption = 'DropView'
      OnClick = DropObject1Click
    end
    object selectView1: TMenuItem
      Caption = 'selectView'
      OnClick = selectView1Click
    end
  end
  object TreePopup5: TPopupMenu
    Left = 560
    Top = 392
    object ReCompile2: TMenuItem
      Caption = 'ReCompile'
      OnClick = ReCompile2Click
    end
    object dropTrigger1: TMenuItem
      Caption = 'drop Trigger'
      OnClick = dropTrigger1Click
    end
  end
  object TreePopup6: TPopupMenu
    Left = 560
    Top = 360
    object ReCompile3: TMenuItem
      Caption = 'ReCompile'
      OnClick = ReCompile3Click
    end
    object DropProcedure1: TMenuItem
      Caption = 'Drop Procedure'
      OnClick = DropProcedure1Click
    end
    object estProcedure1: TMenuItem
      Caption = 'Test Procedure'
      OnClick = estProcedure1Click
    end
  end
  object OpenDialog1: TOpenDialog
    Left = 872
    Top = 360
  end
  object SaveDialog1: TSaveDialog
    Left = 872
    Top = 392
  end
  object ImageList1: TImageList
    Left = 824
    Top = 72
    Bitmap = {
      494C010106000900040010001000FFFFFFFFFF10FFFFFFFFFFFFFFFF424D3600
      0000000000003600000028000000400000003000000001002000000000000030
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      00000000000000000000C6BDD6006339940063399400C6BDD600000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      000000000000000000008C8C8C0018212900212931008C8C8C00000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000523973001810180018081800181018001808180018081800181018007352
      9400000000000000000000000000000000000000000000000000000000000000
      000031394A001821310021294200213952002939520018293900182129002929
      3900000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000002110
      2100211029002110290052315200634263006342630052315200180818001008
      1800181029000000000000000000000000000000000000000000000000001821
      310021294200314A6B0039528400395A8400395A840039528400314A6B001821
      3900101821000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000523973003921
      4A00422942006342630042294A002910290021102900211821004A314A001808
      1800181018007B529C0000000000000000000000000000000000394252002131
      4A00314A6B00395A8C00395A8C00426394004263940042639400395A8400314A
      6B00182139002931390000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      000000000000000000000000000000000000000000000000000052315A006339
      63005A395A006B426B005A395A004A314A00392139003121310063396B00734A
      730018101800100818000000000000000000000000000000000021294200314A
      6B00314A7B00395A8400395A840039527B00395A8400426BA500426394003952
      840029426B001018290000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      00000000000000000000000000000000000000000000BDB5C60073427B003121
      31006B426B006B426B006B426B00634263005A395A00392939006B426B007342
      73005231520018101800C6BDD60000000000000000009C9C9C00293152002942
      5A00294A6B00426B9C004A73A5004A73A5004A73AD004A6BA500426B9C00395A
      8C0031527B00182942008C8C8C00000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      000000000000000000000000000000000000000000006339840084528C003921
      39006B4A6B006B4A6B007B527B007B4A7B006B426B00422942006B426B00734A
      7300734A73001810180063399400000000000000000031394A0029395A002942
      6300294263004A6BA5004A73A5004A73A5004A73A5004A73A500426B9C004263
      9400395284002931520018212900000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000063398400945A9C003121
      31006B426B006B4A6B007B527B00845A84007B527B004A314A006B426B006B4A
      6B006B426B001808180063399400000000000000000039425200314263002942
      5A0029395A004A73A50029425A004A73AD004A73A5004A73A500314A6B004263
      9400395A84002131520018213100000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      00000000000000000000000000000000000000000000B5ADC6009C63A5002118
      21005A395A00734A73008C5A8C007B4A7B00734A73005A395A006B4A6B006B4A
      6B005A395A0018101800C6BDD6000000000000000000ADADAD0039426300314A
      730021314200314A6B004A73A50031527300395A8400314A6B0029426300395A
      8C0031528400212942008C8C9400000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      00000000000000000000000000000000000000000000000000009C63A500AD73
      B500312131006B426B007B4A7300AD73AD0094639400211821006B426B006B42
      63004229420018102100000000000000000000000000000000005A6B7B00314A
      7300395A840042639400426B9C0010182900426BA500426B9C0042639400395A
      8400314A6B001821390000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      00000000000000000000000000000000000000000000000000008C529C00AD73
      B500B57BB50031213100B584B500AD7BAD009C6B9C0052315A005A3152005A39
      630042294A006342840000000000000000000000000000000000848CA5004A5A
      7B00314A7300395A8400395A8C00426394004263940042639400395A8400314A
      73001829420031394A0000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      000000000000000000000000000000000000000000000000000000000000A563
      B500B573BD00B57BBD0021102100A56BAD009C6BA500211821007B4A7B005A39
      630042214A000000000000000000000000000000000000000000000000007B84
      9C004A5A7B00314A730039528400395284003952840039528400314A73002131
      4A00182131000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000945AAD00A56BB500AD6BB500A56BAD009C63A5008C5A940073427B005A31
      7300000000000000000000000000000000000000000000000000000000000000
      0000848C9C005A6B8400394A6B00314263002942630029315200212942003142
      5200000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      00000000000000000000B5A5C600844A9C007B4A9400B5A5C600000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      00000000000000000000ADADB50039425A0031394A009C9C9C00000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000C0C0C000C0C0C000C0C0C000C0C0C0000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      000000000000000000000000000000000000000000000000FF000000FF000000
      FF000000FF000000FF000000FF000000FF000000FF000000FF00000000000000
      0000C0C0C000C0C0C000C0C0C000C0C0C0000000000000000000000000008080
      8000808080008080800080808000808080008080800000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      00000000000094B5BD00082121000810100008181800104A5200000000000000
      000000000000000000000000000000000000000000000000000000000000FFFF
      FF00FFFFFF00FFFFFF0000000000000000000000000000000000FFFFFF00FFFF
      FF00FFFFFF00000000000000000000000000C0C0C000000000000000FF000000
      FF000000FF000000000000000000000000000000FF000000000000000000C0C0
      C000C0C0C000C0C0C000C0C0C000C0C0C0000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      00000000000000000000000000000000000000000000000000000000000084A5
      AD0008181800101818001018180010181800101818001018180008101000184A
      5A00000000000000000000000000000000000000000000000000FFFFFF00FFFF
      FF00FFFFFF00FFFFFF00FFFFFF000000000000000000FFFFFF00FFFFFF00FFFF
      FF00FFFFFF00FFFFFF000000000000000000C0C0C000C0C0C000000000000000
      000000000000FFFFFF00C0C0C000FFFFFF000000000000000000000000000000
      000000000000000000000000000000000000000000000000000000000000C0C0
      C000808080008080800080808000808080008080800080808000808080000080
      000000000000000000000000000000000000000000000000000084A5AD002129
      2900213131002129290018212100101818001018100010181800101818002129
      29001029290000000000000000000000000000000000FFFFFF00FFFFFF00FFFF
      FF00FFFFFF00FFFFFF00FFFFFF000000000000000000FFFFFF00FFFFFF00FFFF
      FF00FFFFFF00FFFFFF00FFFFFF0000000000C0C0C000C0C0C000C0C0C0008080
      8000FFFFFF00C0C0C000FFFFFF00C0C0C0000000000080808000C0C0C000C0C0
      C000C0C0C000C0C0C000C0C0C000C0C0C0000000000080808000808080008080
      8000808080008080800080808000808080008080800080808000808080008080
      800080808000808080000000000000000000000000000000000021393900314A
      4A00314A4A003142420029393900212929002931310021312900213131002129
      2900101818008CADB500000000000000000000000000FFFFFF00FFFFFF000000
      000000000000FFFFFF00FFFFFF000000000000000000FFFFFF00FFFFFF000000
      000000000000FFFFFF00FFFFFF0000000000C0C0C000C0C0C000C0C0C0000000
      000000000000FFFFFF00C0C0C000000000000000000000000000FFFFFF00FFFF
      FF00FFFFFF00FFFFFF00FFFFFF00FFFFFF00000000000000000000000000C0C0
      C000C0C0C000C0C0C000C0C0C000C0C0C000C0C0C000C0C0C000C0C0C000C0C0
      C000C0C0C0008080800080808000000000000000000094ADB50042635A004A6B
      63004A636300394A4A0042524A00314A42002939390021292900101818002131
      29001018180008181800000000000000000000000000FFFFFF00FFFFFF000000
      00000000000000000000FFFFFF00FFFFFF00FFFFFF00FFFFFF00000000000000
      000000000000FFFFFF00FFFFFF0000000000C0C0C000C0C0C000000000000000
      000000000000C0C0C000FFFFFF00C0C0C000FFFFFF00C0C0C00000000000FFFF
      FF00FFFFFF00FFFFFF00FFFFFF00FFFFFF00000000000000000000000000F0CA
      A600F0CAA600000000000000000000000000F0CAA600F0CAA600F0CAA6000000
      0000F0CAA6008080800080808000000000000000000039636300527B73005A7B
      7300293939003142390039524A0039524A0039524A00394A4200394A42002131
      29001018180010181800000000000000000000000000FFFFFF00FFFFFF000000
      0000FF00000000000000FFFFFF00FFFFFF00FFFFFF00FFFFFF0000000000FF00
      000000000000FFFFFF00FFFFFF0000000000C0C0C00000000000000000000000
      00000000000000000000C0C0C000FFFFFF00C0C0C000FFFFFF0000000000FFFF
      FF00FFFFFF00FFFFFF00FFFFFF00FFFFFF00000000000000000000000000F0CA
      A60000000000F0CAA600F0CAA600000000000000000000000000000000000000
      0000F0CAA600808080008080800000000000000000004A737300638C84006B94
      8C006B8C8C00314242005A7B73005A7B73005A736B00526B63004A635A002939
      39001018180010181800000000000000000000000000FFFFFF00FFFFFF000000
      000000000000FFFFFF00FFFFFF000000000000000000FFFFFF00FFFFFF000000
      000000000000FFFFFF00FFFFFF0000000000C0C0C00000000000000000000000
      000000000000C0C0C000FFFFFF00C0C0C000FFFFFF00C0C0C00000000000FFFF
      FF00FFFFFF00FFFFFF00FFFFFF00FFFFFF00000000000000000000000000F0CA
      A600FF000000FF000000FF00000000000000FF0000000000000000FFFF000000
      0000F0CAA60080808000808080000000000000000000527B7B006B948C00739C
      94007B9C9400394A42005A736B00637B7B00637B7300637B7B004A635A003142
      3900182121001018180000000000000000000000000000000000FFFFFF00FFFF
      FF00FFFFFF00FFFFFF0000000000FFFFFF00FFFFFF0000000000FFFFFF00FFFF
      FF00FFFFFF00FFFFFF000000000000000000C0C0C00000000000000000000000
      000000000000FFFFFF00C0C0C000FFFFFF00C0C0C000FFFFFF00C0C0C0000000
      0000FF000000FF000000FF000000FF000000000000000000000000000000F0CA
      A600F0CAA6000000FF000000FF0000000000FF00000000000000000000000000
      0000F0CAA600808080008080800000000000000000004A7B7B0073A59C007BA5
      9C0094B5AD0042524A004A635A005A736B00637B7300637B7300526B63003142
      4200213131001018100000000000000000000000000000FFFF00000000000000
      0000000000000000000000FFFF0000FFFF00FFFFFF00FFFFFF00000000000000
      00000000000000000000FFFFFF0000000000C0C0C00000000000000000000000
      00000000000000000000FFFFFF00C0C0C000FFFFFF00FF00000000000000FFFF
      FF00FFFFFF00FFFFFF00FFFFFF00FFFFFF00000000000000000000000000F0CA
      A6000000FF00F0CAA600F0CAA600000000000000000000000000008000000000
      0000F0CAA600808080008080800000000000000000000000000073A59C0084AD
      A5009CBDBD004A5A52005A736B00425A5200425A5200425A52004A635A00314A
      42002139390010313900000000000000000000000000FFFFFF00FFFFFF00FFFF
      FF00FFFFFF00FFFFFF00FFFFFF000000000000000000FFFFFF00FFFFFF00FFFF
      FF00FFFFFF00FFFFFF00FFFFFF0000000000C0C0C00000000000000000000000
      0000000000000000000000000000FFFFFF00C0C0C000FFFFFF0000000000FFFF
      FF00FFFFFF00FFFFFF00FFFFFF00FFFFFF00000000000000000000000000F0CA
      A60000000000F0CAA600F0CAA600000000000000FF0000000000000000000000
      0000F0CAA60080808000808080000000000000000000000000005A9494007BAD
      A50084B5AD0052736B005A7B73004A6363004A635A00425A5200425A5A00394A
      4A00213939000000000000000000000000000000000000FFFF0000FFFF0000FF
      FF0000FFFF0000FFFF0000FFFF000000000000000000FFFFFF00FFFFFF00FFFF
      FF00FFFFFF00FFFFFF00FFFFFF0000000000C0C0C00000000000000000000000
      00000000000000000000000000000000000000000000C0C0C000FFFFFF000000
      0000FFFFFF00FFFFFF00FFFFFF00FFFFFF00000000000000000000000000F0CA
      A600F0CAA600F0CAA600F0CAA600F0CAA6000000000000000000F0CAA600F0CA
      A600F0CAA60080808000808080000000000000000000000000000000000073A5
      9C0029393900314239003142390029393900293931002939310029313100314A
      4A005A848C000000000000000000000000000000000000000000FFFFFF00FFFF
      FF00FFFFFF00FFFFFF00FFFFFF000000000000000000FFFFFF00FFFFFF00FFFF
      FF00FFFFFF00FFFFFF000000000000000000C0C0C000C0C0C000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000FF000000FF000000FF000000FF0000000000000000000000C0C0C0000000
      0000000000000000000000000000000000000000000000000000000000000000
      000000000000C0C0C00080808000000000000000000000000000000000000000
      00006B9CA5006BA59C0073A59C006B9C940063948C00527B7B00396363000000
      00000000000000000000000000000000000000000000000000000000000000FF
      FF0000FFFF0000FFFF0000000000000000000000000000000000FFFFFF00FFFF
      FF00FFFFFF00000000000000000000000000C0C0C000C0C0C000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000FFFFFF00FFFFFF00FFFFFF00FFFFFF000000000080808000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000080808000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      000000000000000000000000000000000000C0C0C000C0C0C000C0C0C0000000
      0000000000000000000000000000000000000000000000000000FFFFFF00FFFF
      FF00FFFFFF00FFFFFF00FFFFFF00FFFFFF000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      000000000000000000000000000000000000424D3E000000000000003E000000
      2800000040000000300000000100010000000000800100000000000000000000
      000000000000000000000000FFFFFF0000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      00000000000000000000000000000000FFFFFFFF00000000FC3FFC3F00000000
      F00FF00F00000000E007E00700000000C003C00300000000C003C00300000000
      8001800100000000800180010000000080018001000000008001800100000000
      C003C00300000000C003C00300000000E007E00700000000F00FF00F00000000
      FC3FFC3F00000000FFFFFFFF00000000FFFFFFF0FFFFFFFFE3C78030C003F83F
      C1834760C003E00F818138C0C003C007800100008001C00380010000C0018003
      00000000C001800300000000C001800300000000C001800300000000C0018003
      00000000C001C00380010000C001C00780010000C001E00781810000C001F01F
      C1830000BFFDFFFFE3C70000FFFFFFFF00000000000000000000000000000000
      000000000000}
  end
  object IndexPopup: TPopupMenu
    Left = 472
    Top = 168
    object CreateIndex1: TMenuItem
      Caption = 'CreateIndex'
      OnClick = CreateIndex1Click
    end
    object DropIndex1: TMenuItem
      Caption = 'DropIndex'
      OnClick = DropIndex1Click
    end
  end
  object Timer1: TTimer
    Enabled = False
    Interval = 3000
    OnTimer = Timer1Timer
    Left = 832
    Top = 136
  end
  object TreePopup7: TPopupMenu
    Left = 520
    Top = 360
    object DropTableSpace2: TMenuItem
      Caption = 'Drop TableSpace'
      OnClick = DropTableSpace2Click
    end
    object AlterTableSpace1: TMenuItem
      Caption = 'add Datafile'
      OnClick = AlterTableSpace1Click
    end
  end
  object TreePopup8: TPopupMenu
    Left = 488
    Top = 360
    object Reload1: TMenuItem
      Caption = 'Refresh'
      OnClick = Reload1Click
    end
  end
end
