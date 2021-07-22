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

#ifndef disktblForm1H
#define disktblForm1H
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <ExtCtrls.hpp>
#include <ADODB.hpp>
#include <DB.hpp>
#include <DBGrids.hpp>
#include <Grids.hpp>
//---------------------------------------------------------------------------
class TdiskTblMgrForm : public TForm
{
__published:	// IDE-managed Components
	TPanel *Panel1;
	TCheckBox *CheckBox1;
	TButton *Button1;
	TADOConnection *ADOConnection1;
	TTimer *Timer1;
	TDBGrid *DBGrid1;
	TADOQuery *ADOQuery1;
	TDataSource *DataSource2;
	TEdit *TBS_NAME;
	TPanel *DSN;
	void __fastcall CheckBox1Click(TObject *Sender);
	void __fastcall Timer1Timer(TObject *Sender);
	void __fastcall Button1Click(TObject *Sender);
private:	// User declarations
public:		// User declarations
	__fastcall TdiskTblMgrForm(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TdiskTblMgrForm *diskTblMgrForm;
//---------------------------------------------------------------------------
#endif
