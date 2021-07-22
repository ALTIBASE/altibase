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

#ifndef winISQLH
#define winISQLH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <Menus.hpp>
#include <ComCtrls.hpp>
#include <DBGrids.hpp>
#include <Grids.hpp>
#include <ToolWin.hpp>
#include <DB.hpp>
#include <DBTables.hpp>
//---------------------------------------------------------------------------
class TForm4 : public TForm
{
__published:	// IDE-managed Components
	TMainMenu *MainMenu1;
	TMenuItem *File1;
	TMenuItem *DSNManager1;
	TMenuItem *Connect1;
	TMenuItem *Disconnect1;
	TMenuItem *AllDisconnect1;
	TMenuItem *Edit1;
	TMenuItem *Query;
	TMenuItem *ransaction1;
	TMenuItem *Help1;
	TMenuItem *N1;
	TMenuItem *Open1;
	TMenuItem *Save1;
	TMenuItem *Print1;
	TMenuItem *Exit1;
	TPageControl *PageControl11;
	TTabSheet *TabSheet1Data;
	TTabSheet *TabSheet1Message;
	TTabSheet *TabSheet1Plan;
	TTabSheet *TabSheet1History;
	TStatusBar *StatusBar1;
	TDBGrid *DBGrid11;
	TMemo *Memo11;
	TListBox *ListBox11;
	TListBox *ListBox12;
	TToolBar *ToolBar1;
	TToolButton *ToolButton1;
	TToolButton *ToolButton2;
	TToolButton *ToolButton3;
	TQuery *Query1;
	TDatabase *Database1;
	TDataSource *DataSource1;
	TPageControl *PageControl1;
	TTabSheet *TabSheet1;
	TTabSheet *TabSheet2;
	TTabSheet *TabSheet3;
	TTabSheet *TabSheet4;
	TTabSheet *TabSheet5;
	TTabSheet *TabSheet6;
	TTabSheet *TabSheet7;
	TRichEdit *RichEdit31;
	TPageControl *PageControl31;
	TTabSheet *TabSheet3Data;
	TDBGrid *DBGrid31;
	TTabSheet *TabSheet3Message;
	TListBox *ListBox31;
	TTabSheet *TabSheet3Plan;
	TMemo *Memo31;
	TTabSheet *TabSheet3History;
	TListBox *ListBox32;
	TRichEdit *RichEdit41;
	TPageControl *PageControl41;
	TTabSheet *TabSheet4Data;
	TDBGrid *DBGrid41;
	TTabSheet *TabSheet4Message;
	TListBox *ListBox41;
	TTabSheet *TabSheet4Plan;
	TMemo *Memo41;
	TTabSheet *TabSheet4History;
	TListBox *ListBox42;
	TRichEdit *RichEdit51;
	TPageControl *PageControl51;
	TTabSheet *TabSheet5Data;
	TDBGrid *DBGrid51;
	TTabSheet *TabSheet5Message;
	TListBox *ListBox51;
	TTabSheet *TabSheet5Plan;
	TMemo *Memo51;
	TTabSheet *TabSheet5History;
	TListBox *ListBox52;
	TRichEdit *RichEdit71;
	TPageControl *PageControl71;
	TTabSheet *TabSheet7Data;
	TDBGrid *DBGrid71;
	TTabSheet *TabSheet7Message;
	TListBox *ListBox71;
	TTabSheet *TabSheet7Plan;
	TMemo *Memo71;
	TTabSheet *TabSheet7History;
	TListBox *ListBox72;
	TRichEdit *RichEdit21;
	TPageControl *PageControl21;
	TTabSheet *TabSheet2Data;
	TDBGrid *DBGrid21;
	TTabSheet *TabSheet2Message;
	TListBox *ListBox21;
	TTabSheet *TabSheet2Plan;
	TMemo *Memo21;
	TTabSheet *TabSheet2History;
	TListBox *ListBox22;
	TRichEdit *RichEdit11;
	TRichEdit *RichEdit61;
	TPageControl *PageControl61;
	TTabSheet *TabSheet6Data;
	TDBGrid *DBGrid61;
	TTabSheet *TabSheet6Message;
	TListBox *ListBox61;
	TTabSheet *TabSheet6Plan;
	TMemo *Memo61;
	TTabSheet *TabSheet6History;
	TListBox *ListBox62;
	TDatabase *Database2;
	TDatabase *Database3;
	TDatabase *Database4;
	TDatabase *Database5;
	TDatabase *Database6;
	TDatabase *Database7;
	TQuery *Query2;
	TQuery *Query3;
	TQuery *Query4;
	TQuery *Query5;
	TQuery *Query6;
	TQuery *Query7;
	TDataSource *DataSource2;
	TDataSource *DataSource3;
	TDataSource *DataSource4;
	TDataSource *DataSource5;
	TDataSource *DataSource6;
	TDataSource *DataSource7;
	TListBox *ListBox1;
	TPopupMenu *PopupMenu1;
	TMenuItem *Connect2;
	TMenuItem *Disconnect2;
	TPopupMenu *PopupMenu2;
	TMenuItem *Disconnect3;
	void __fastcall ToolButton1Click(TObject *Sender);
	void __fastcall ToolButton2Click(TObject *Sender);
	void __fastcall ToolButton3Click(TObject *Sender);
	void __fastcall DSNManager1Click(TObject *Sender);
	void __fastcall FormShow(TObject *Sender);
	void __fastcall Connect2Click(TObject *Sender);
	void __fastcall Disconnect2Click(TObject *Sender);
	void __fastcall Disconnect3Click(TObject *Sender);
private:	// User declarations
public:		// User declarations
	__fastcall TForm4(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TForm4 *Form4;
//---------------------------------------------------------------------------
#endif
