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

#ifndef lockMgrH
#define lockMgrH
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
class TlockMgrForm : public TForm
{
__published:	// IDE-managed Components
	TPanel *DSN;
	TDBGrid *DBGrid2;
	TDBGrid *DBGrid1;
	TPanel *Panel1;
	TButton *Button1;
	TCheckBox *CheckBox1;
	TADOConnection *ADOConnection1;
	TTimer *Timer1;
	TADOQuery *ADOQuery1;
	TDataSource *DataSource2;
	TADOQuery *ADOQuery2;
	TDataSource *DataSource1;
	TDBGrid *DBGrid3;
	TADOQuery *ADOQuery3;
	TDataSource *DataSource3;
	void __fastcall FormClose(TObject *Sender, TCloseAction &Action);
	void __fastcall Button1Click(TObject *Sender);
	void __fastcall DBGrid1DblClick(TObject *Sender);
	void __fastcall Timer1Timer(TObject *Sender);
	void __fastcall CheckBox1Click(TObject *Sender);
	void __fastcall FormResize(TObject *Sender);
	void __fastcall DBGrid2DblClick(TObject *Sender);
private:	// User declarations
public:		// User declarations
	__fastcall TlockMgrForm(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TlockMgrForm *lockMgrForm;
//---------------------------------------------------------------------------
#endif
