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

#ifndef replMgrForm1H
#define replMgrForm1H
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <ADODB.hpp>
#include <DB.hpp>
#include <DBGrids.hpp>
#include <ExtCtrls.hpp>
#include <Grids.hpp>
//---------------------------------------------------------------------------
class TreplMgr : public TForm
{
__published:	// IDE-managed Components
	TPanel *Panel1;
	TButton *Button1;
	TCheckBox *CheckBox1;
	TDBGrid *DBGrid1;
	TADOConnection *ADOConnection1;
	TTimer *Timer1;
	TADOQuery *ADOQuery1;
	TDataSource *DataSource1;
	TPanel *DSN;
	TDBGrid *DBGrid2;
	TDataSource *DataSource2;
	TADOQuery *ADOQuery2;
	TDBGrid *DBGrid3;
	TDataSource *DataSource3;
	TADOQuery *ADOQuery3;
	TPanel *Panel2;
	TPanel *Panel3;
	TPanel *Panel4;
	TLabel *Label3;
	TEdit *Edit1;
	TButton *Button3;
	TCheckBox *CheckBox2;
	TButton *Button4;
	TButton *Button2;
	void __fastcall CheckBox1Click(TObject *Sender);
	void __fastcall Timer1Timer(TObject *Sender);
	void __fastcall Button1Click(TObject *Sender);
	void __fastcall FormClose(TObject *Sender, TCloseAction &Action);
	void __fastcall Button2Click(TObject *Sender);
	void __fastcall Button4Click(TObject *Sender);
	void __fastcall Button3Click(TObject *Sender);
private:	// User declarations
public:		// User declarations
	__fastcall TreplMgr(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TreplMgr *replMgr;
//---------------------------------------------------------------------------
#endif
