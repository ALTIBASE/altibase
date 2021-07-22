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

#ifndef sessionMgrH
#define sessionMgrH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <Grids.hpp>
#include <ADODB.hpp>
#include <DB.hpp>
#include <DBGrids.hpp>
#include <ExtCtrls.hpp>
//---------------------------------------------------------------------------
class TsessionMgrForm : public TForm
{
__published:	// IDE-managed Components
	TADOConnection *ADOConnection1;
	TADOQuery *ADOQuery1;
	TDBGrid *DBGrid1;
	TDataSource *DataSource2;
	TRadioGroup *RadioGroup1;
	TPanel *Panel1;
	TButton *Button1;
	TADOQuery *ADOQuery2;
	TDBGrid *DBGrid2;
	TDataSource *DataSource1;
	TPanel *DSN;
	TTimer *Timer1;
	TCheckBox *CheckBox1;
	TMemo *Memo1;
	TADOQuery *ADOQuery3;
	void __fastcall Button1Click(TObject *Sender);
	void __fastcall DBGrid1DblClick(TObject *Sender);
	void __fastcall FormClose(TObject *Sender, TCloseAction &Action);
	void __fastcall FormResize(TObject *Sender);
	void __fastcall CheckBox1Click(TObject *Sender);
	void __fastcall Timer1Timer(TObject *Sender);
	void __fastcall FormShow(TObject *Sender);
	void __fastcall DBGrid2DblClick(TObject *Sender);
	void __fastcall Memo1DblClick(TObject *Sender);
private:	// User declarations
public:		// User declarations
	__fastcall TsessionMgrForm(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TsessionMgrForm *sessionMgrForm;
//---------------------------------------------------------------------------
#endif
