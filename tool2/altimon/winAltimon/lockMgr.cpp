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

#include <vcl.h>
#pragma hdrstop

#include "lockMgr.h"
#include "sqlViewForm.h"
#include "mainCall.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TlockMgrForm *lockMgrForm;
//---------------------------------------------------------------------------
__fastcall TlockMgrForm::TlockMgrForm(TComponent* Owner)
	: TForm(Owner)
{
}
//---------------------------------------------------------------------------
void __fastcall TlockMgrForm::FormClose(TObject *Sender, TCloseAction &Action)
{
    Action = caFree;	
}
//---------------------------------------------------------------------------
void __fastcall TlockMgrForm::Button1Click(TObject *Sender)
{
    //ADOQuery2->Close();
    Timer1->Enabled = false;

	if (ADOConnection1->Connected == false)
	{
        ADOConnection1->ConnectionString = "DSN=" + DSN->Caption;
		ADOConnection1->Open();
	}

	ADOQuery1->SQL->Text = "alter session set select_header_display = 1";
	try {
		ADOQuery1->ExecSQL();
	} catch (...) {
    	CheckBox1->Checked = false;
		Timer1->Enabled = false;
	}

	ADOQuery1->SQL->Text = "select a.session_id as sid, "
						   "       rpad(b.table_name, 40, ' ') as tableName, "
						   "       a.tx_id, "
						   "       rpad(a.lock_desc, 20, ' ') as lockDesc, "
						   "       a.is_grant, trunc(c.total_time/1000000, 2) as runTime "
						   "from v$lock_statement a, system_.sys_tables_ b, v$statement c "
						   "where   a.table_oid  = b.table_oid "
						   "and     a.tx_id = c.tx_id "
						   "and     c.total_Time >= 500000 " ;

	//DBGrid1->Columns->BeginUpdate();
	try {
		ADOQuery1->Open();
		//ADOQuery1->Last();
		//ADOQuery1->First();
	} catch (...) {
		CheckBox1->Checked = false;
		Timer1->Enabled = false;
	}

    if (CheckBox1->Checked == true)
	{
		Timer1->Enabled = true;
	}
	//DBGrid1->Columns->EndUpdate();
}
//---------------------------------------------------------------------------
void __fastcall TlockMgrForm::DBGrid1DblClick(TObject *Sender)
{
	AnsiString TXID   = DBGrid1->Fields[2]->AsString;

	if (TXID.Length() == 0)
	{
        return;
	}


	ADOQuery2->SQL->Text = " select a.id, rpad(a.comm_name, 40, ' ') as commName , b.id as stmt_ID, "
                           "       b.total_time, b.execute_flag, b.execute_success, b.query "
						   " from v$session a, v$statement b "
						   " where b.tx_id = " + TXID +
						   " and   a.id = b.session_id ";
	
	ADOQuery2->Open();	
}
//---------------------------------------------------------------------------
void __fastcall TlockMgrForm::Timer1Timer(TObject *Sender)
{
    Timer1->Interval = (int)atoi(mainCallForm->SleepTime->Text.c_str()) * 1000;
	Button1Click(this);
}
//---------------------------------------------------------------------------
void __fastcall TlockMgrForm::CheckBox1Click(TObject *Sender)
{
	if (CheckBox1->Checked == true)
	{
		Timer1->Enabled = true;
	}else {
        Timer1->Enabled = false;
    }	
}
//---------------------------------------------------------------------------
void __fastcall TlockMgrForm::FormResize(TObject *Sender)
{
	DBGrid1->Top    = Panel1->Top + Panel1->Height;
	DBGrid1->Height = ( (Panel1->Top + Panel1->Height) + DSN->Top )  / 2 - DBGrid1->Top ;

    DBGrid1->Align = alTop;
	DBGrid2->Align = alClient;	
}
//---------------------------------------------------------------------------

void __fastcall TlockMgrForm::DBGrid2DblClick(TObject *Sender)
{
	/*AnsiString SID    = DBGrid2->Fields[0]->AsString;
	AnsiString STMTID = DBGrid2->Fields[2]->AsString;
	AnsiString SQLTEXT = "", PLANTEXT = "", TMP;
	int i, count, start = 0;


	if (SID.Length() == 0 || STMTID.Length() == 0)
	{
        return;    
	}
	ADOQuery3->SQL->Text = "select text from v$sqltext where sid = " + SID +
						   " and stmt_id = " + STMTID + " order by piece ";
	ADOQuery3->Open();

	ADOQuery3->Last();
	ADOQuery3->First();
	count = ADOQuery3->RecordCount;

	for (i = 0; i < count ; i++)
	{
		SQLTEXT = SQLTEXT + DBGrid3->Fields[0]->AsString;
		ADOQuery3->Next();
	}

	//ShowMessage(SQLTEXT);
	SQLVIEW->SQLTEXT->Text = SQLTEXT;




    STMTID = 5;
	ADOQuery3->SQL->Text = "select text from v$plantext where sid = " + SID +
						   " and stmt_id = " + STMTID + " order by piece ";
	ADOQuery3->Open();

	ADOQuery3->Last();
	ADOQuery3->First();
	count = ADOQuery3->RecordCount;

	for (i = 0; i < count ; i++)
	{
		PLANTEXT = PLANTEXT + DBGrid3->Fields[0]->AsString;
		ADOQuery3->Next();
	}

	start = 0;
    SQLVIEW->PLANTEXT->Clear();
	for (i=0; i < PLANTEXT.Length(); i++)
	{
		if (PLANTEXT.SubString(i, 1) == "\n")
		{
			TMP = PLANTEXT.SubString(start, i);
			start = i;
			SQLVIEW->PLANTEXT->Lines->Add(TMP);
			//ShowMessage(i);
		}
	}
    TMP = PLANTEXT.SubString(start, i);
	SQLVIEW->PLANTEXT->Lines->Add(TMP);
	//SQLVIEW->PLANTEXT->Text = PLANTEXT;



	SQLVIEW->Show();  */
}
//---------------------------------------------------------------------------

