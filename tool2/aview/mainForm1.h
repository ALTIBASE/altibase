/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 
//---------------------------------------------------------------------------

#ifndef mainForm1H
#define mainForm1H
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <ExtCtrls.hpp>
#include <Menus.hpp>
#include <ComCtrls.hpp>
#include <Grids.hpp>
#include <Dialogs.hpp>
#include <ImgList.hpp>
//---------------------------------------------------------------------------
class TForm5 : public TForm
{
__published:	// IDE-managed Components
	TPanel *gMSG;
	TMainMenu *MainMenu1;
	TMenuItem *Server1;
	TMenuItem *Management1;
	TMenuItem *SQL1;
	TMenuItem *Resource1;
	TMenuItem *Option1;
	TMenuItem *About1;
	TMenuItem *Help1;
	TMenuItem *About2;
	TMenuItem *imeout1;
	TMenuItem *CommitMode1;
	TMenuItem *System1;
	TMenuItem *Altibase1;
	TMenuItem *Information1;
	TMenuItem *Report1;
	TMenuItem *Pstack1;
	TMenuItem *SummaryMonitor1;
	TMenuItem *winISQL1;
	TMenuItem *UserSQL1;
	TMenuItem *StartDBMS1;
	TMenuItem *StopDBMS1;
	TMenuItem *Replication1;
	TMenuItem *ExportData1;
	TMenuItem *ImportData1;
	TMenuItem *SchemaScript1;
	TMenuItem *AddDSN1;
	TMenuItem *Connect1;
	TMenuItem *DisConnect1;
	TPanel *SERVERNAME;
	TTreeView *DBNODE;
	TPopupMenu *TreePopup1;
	TMenuItem *Connect2;
	TMenuItem *DisConnect2;
	TMenuItem *Start1;
	TMenuItem *StopAltibase1;
	TMenuItem *userAdd1;
	TMenuItem *schemaOut1;
	TMenuItem *ExportData2;
	TMenuItem *ImportData2;
	TPopupMenu *TreePopup2;
	TMenuItem *dropUser1;
	TMenuItem *schemaOut2;
	TMenuItem *CreateTableSpace1;
	TPopupMenu *TreePopup3;
	TMenuItem *truncate1;
	TMenuItem *droptable1;
	TMenuItem *selectTable1;
	TMenuItem *alterTable1;
	TMenuItem *createTable1;
	TPopupMenu *TreePopup4;
	TMenuItem *ReCompile1;
	TMenuItem *DropObject1;
	TMenuItem *selectView1;
	TPopupMenu *TreePopup5;
	TMenuItem *ReCompile2;
	TMenuItem *dropTrigger1;
	TPopupMenu *TreePopup6;
	TMenuItem *ReCompile3;
	TMenuItem *DropProcedure1;
	TMenuItem *estProcedure1;
	TPanel *Panel1;
	TPanel *TNAMES;
	TStringGrid *ColGrid;
	TStringGrid *IndexGrid;
	TStringGrid *DataGrid;
	TMenuItem *AddUser1;
	TMenuItem *DropUser2;
	TPanel *CRTTBL_PANEL;
	TEdit *CNAME;
	TComboBox *CTYPE;
	TEdit *CPRE;
	TEdit *CSCALE;
	TButton *Button1;
	TLabel *Label1;
	TLabel *Label2;
	TLabel *Label3;
	TLabel *Label4;
	TLabel *Label5;
	TComboBox *ISNULL;
	TLabel *Label6;
	TEdit *CDEFAULT;
	TPanel *CRTTBL_BUTTON;
	TButton *Button2;
	TButton *Button3;
	TEdit *targetUser;
	TPanel *Panel2;
	TEdit *CRTNAME;
	TMemo *CRTMEMO;
	TOpenDialog *OpenDialog1;
	TButton *Button4;
	TPanel *FETCHPANEL;
	TButton *NEXTFETCH;
	TButton *Button6;
	TSaveDialog *SaveDialog1;
	TMenuItem *Delimeter1;
	TMenuItem *ScriptOut1;
	TMemo *PROC;
	TPanel *PROCPANEL;
	TButton *Button5;
	TButton *Button7;
	TButton *Button8;
	TMenuItem *createProcedure1;
	TMenuItem *createView1;
	TMenuItem *create1;
	TImageList *ImageList1;
	TStringGrid *ProcGrid;
	TButton *Button9;
	TButton *Button10;
	TPopupMenu *IndexPopup;
	TMenuItem *CreateIndex1;
	TMenuItem *DropIndex1;
	TMenuItem *DDL1;
	TMenuItem *createTable2;
	TMenuItem *createProcedure2;
	TMenuItem *createTrigger1;
	TMenuItem *droptable2;
	TMenuItem *dropProcedure2;
	TMenuItem *dropTrigger2;
	TMenuItem *createView2;
	TMenuItem *dropView1;
	TMenuItem *recompile4;
	TMenuItem *createIndex2;
	TMenuItem *DML1;
	TMenuItem *selectdata1;
	TMenuItem *testProcedure1;
	TMenuItem *schemaout3;
	TMenuItem *exportData4;
	TMenuItem *importData4;
	TTimer *Timer1;
	TMenuItem *iLoaderStatus1;
	TPanel *TBS_PANEL;
	TPanel *Panel3;
	TStringGrid *DATAFILE;
	TProgressBar *ProgressBar1;
	TPanel *Panel4;
	TLabel *Label7;
	TEdit *TOTAL_SIZE;
	TLabel *Label8;
	TEdit *USED_SIZE;
	TLabel *Label9;
	TEdit *TABLE_USE;
	TLabel *Label10;
	TEdit *INDEX_USE;
	TLabel *RATE1;
	TMenuItem *droptablespace1;
	TPopupMenu *TreePopup7;
	TMenuItem *DropTableSpace2;
	TMenuItem *AlterTableSpace1;
	TPopupMenu *TreePopup8;
	TMenuItem *Reload1;
	TLabel *Label11;
	TLabel *Label12;
	TLabel *Label13;
	TLabel *Label14;
	void __fastcall AddDSN1Click(TObject *Sender);
	void __fastcall FormShow(TObject *Sender);
	void __fastcall DBNODEClick(TObject *Sender);
	void __fastcall Connect1Click(TObject *Sender);
	void __fastcall DisConnect1Click(TObject *Sender);
	void __fastcall DBNODEDblClick(TObject *Sender);
	void __fastcall Connect2Click(TObject *Sender);
	void __fastcall DisConnect2Click(TObject *Sender);
	void __fastcall AddUser1Click(TObject *Sender);
	void __fastcall userAdd1Click(TObject *Sender);
	void __fastcall DropUser2Click(TObject *Sender);
	void __fastcall dropUser1Click(TObject *Sender);
	void __fastcall createTable1Click(TObject *Sender);
	void __fastcall Button1Click(TObject *Sender);
	void __fastcall Button2Click(TObject *Sender);
	void __fastcall Button3Click(TObject *Sender);
	void __fastcall Button4Click(TObject *Sender);
	void __fastcall droptable1Click(TObject *Sender);
	void __fastcall truncate1Click(TObject *Sender);
	void __fastcall selectTable1Click(TObject *Sender);
	void __fastcall NEXTFETCHClick(TObject *Sender);
	void __fastcall selectView1Click(TObject *Sender);
	void __fastcall Button6Click(TObject *Sender);
	void __fastcall Delimeter1Click(TObject *Sender);
	void __fastcall ScriptOut1Click(TObject *Sender);
	void __fastcall schemaOut2Click(TObject *Sender);
	void __fastcall Button5Click(TObject *Sender);
	void __fastcall Button7Click(TObject *Sender);
	void __fastcall Button8Click(TObject *Sender);
	void __fastcall ReCompile3Click(TObject *Sender);
	void __fastcall ReCompile1Click(TObject *Sender);
	void __fastcall DropProcedure1Click(TObject *Sender);
	void __fastcall DropObject1Click(TObject *Sender);
	void __fastcall winISQL1Click(TObject *Sender);
	void __fastcall System1Click(TObject *Sender);
	void __fastcall estProcedure1Click(TObject *Sender);
	void __fastcall alterTable1Click(TObject *Sender);
	void __fastcall ReCompile2Click(TObject *Sender);
	void __fastcall dropTrigger1Click(TObject *Sender);
	void __fastcall createProcedure1Click(TObject *Sender);
	void __fastcall createView1Click(TObject *Sender);
	void __fastcall create1Click(TObject *Sender);
	void __fastcall DBNODEGetImageIndex(TObject *Sender, TTreeNode *Node);
	void __fastcall Button9Click(TObject *Sender);
	void __fastcall DataGridDblClick(TObject *Sender);
	void __fastcall Button10Click(TObject *Sender);
	void __fastcall CreateIndex1Click(TObject *Sender);
	void __fastcall DropIndex1Click(TObject *Sender);
	void __fastcall createTable2Click(TObject *Sender);
	void __fastcall createProcedure2Click(TObject *Sender);
	void __fastcall createView2Click(TObject *Sender);
	void __fastcall createTrigger1Click(TObject *Sender);
	void __fastcall droptable2Click(TObject *Sender);
	void __fastcall dropProcedure2Click(TObject *Sender);
	void __fastcall dropView1Click(TObject *Sender);
	void __fastcall dropTrigger2Click(TObject *Sender);
	void __fastcall recompile4Click(TObject *Sender);
	void __fastcall createIndex2Click(TObject *Sender);
	void __fastcall schemaout3Click(TObject *Sender);
	void __fastcall selectdata1Click(TObject *Sender);
	void __fastcall testProcedure1Click(TObject *Sender);
	void __fastcall exportData4Click(TObject *Sender);
	void __fastcall Timer1Timer(TObject *Sender);
	void __fastcall iLoaderStatus1Click(TObject *Sender);
	void __fastcall FormCloseQuery(TObject *Sender, bool &CanClose);
	void __fastcall ExportData1Click(TObject *Sender);
	void __fastcall ExportData2Click(TObject *Sender);
	void __fastcall ExportData3Click(TObject *Sender);
	void __fastcall ImportData2Click(TObject *Sender);
	void __fastcall importData4Click(TObject *Sender);
	void __fastcall ImportData1Click(TObject *Sender);
	void __fastcall CreateTableSpace1Click(TObject *Sender);
	void __fastcall droptablespace1Click(TObject *Sender);
	void __fastcall DropTableSpace2Click(TObject *Sender);
	void __fastcall AlterTableSpace1Click(TObject *Sender);
	void __fastcall Reload1Click(TObject *Sender);
	void __fastcall SchemaScript1Click(TObject *Sender);
private:	// User declarations
public:		// User declarations
	__fastcall TForm5(TComponent* Owner);
	void __fastcall TForm5::GetDsnList();
	int __fastcall TForm5::getDsnInfo(AnsiString, char *, char *);
	void __fastcall TForm5::getTableInfo(AnsiString , AnsiString , AnsiString );
	void __fastcall TForm5::selectObject(AnsiString );
	void __fastcall TForm5::tableScript(AnsiString, AnsiString );
	void __fastcall TForm5::getColumnInfo(AnsiString , AnsiString );
	void __fastcall TForm5::getIndexInfo(AnsiString , AnsiString ) ;
	void __fastcall TForm5::getPKInfo(AnsiString , AnsiString );
	void __fastcall TForm5::getFKInfo(AnsiString , AnsiString );
	int __fastcall TForm5::_ExecDirect(char *);
	AnsiString __fastcall TForm5::getProcedure( AnsiString , AnsiString);
	void __fastcall TForm5::displayObject2(AnsiString, int);
	AnsiString __fastcall TForm5::getView( AnsiString , AnsiString );
	AnsiString __fastcall TForm5::getTrigger( AnsiString , AnsiString );
	void __fastcall TForm5::MakeexecuteProc(AnsiString , AnsiString ) ;
	void __fastcall TForm5::_procExec();
	void __fastcall TForm5::_procFunc();
	bool __fastcall TForm5::checkSelection();
	void __fastcall TForm5::execAexportOut(int , AnsiString , AnsiString );
	void __fastcall TForm5::execAexportIn(int , AnsiString uname, AnsiString );
	void __fastcall TForm5::freeIloaderNode();
	void __fastcall TForm5::TerminateIloaderNode();
	int __fastcall TForm5::findTaskIloader();
	AnsiString __fastcall TForm5::ExecLineSQL(AnsiString );
	void __fastcall TForm5::getTbsInfo(AnsiString , AnsiString , AnsiString );
	void __fastcall TForm5::getMemTbsInfo(AnsiString , AnsiString , AnsiString );
};
//---------------------------------------------------------------------------
extern PACKAGE TForm5 *Form5;
//---------------------------------------------------------------------------
#endif
