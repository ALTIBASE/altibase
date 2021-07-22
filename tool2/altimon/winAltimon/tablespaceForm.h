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

#ifndef tblFormH
#define tblFormH
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
class TtblForm : public TForm
{
__published:	// IDE-managed Components
	TDBGrid *DBGrid1;
	TDBGrid *DBGrid2;
	TPanel *Panel1;
	TADOConnection *ADOConnection1;
	TCheckBox *CheckBox1;
	TButton *Button1;
	TTimer *Timer1;
	TADOQuery *ADOQuery1;
	TADOQuery *ADOQuery2;
	TDataSource *DataSource1;
	TDataSource *DataSource2;
	TPanel *DSN;
	void __fastcall Button1Click(TObject *Sender);
	void __fastcall Timer1Timer(TObject *Sender);
	void __fastcall CheckBox1Click(TObject *Sender);
	void __fastcall FormClose(TObject *Sender, TCloseAction &Action);
	void __fastcall DBGrid2DblClick(TObject *Sender);
private:	// User declarations
public:		// User declarations
	__fastcall TtblForm(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TtblForm *tblForm;
//---------------------------------------------------------------------------
#endif
