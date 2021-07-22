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
#include <stdio.h>
#include <vcl.h>
#pragma hdrstop

#include "sessionMgr.h"
#include "mainCall.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TsessionMgrForm *sessionMgrForm;
//---------------------------------------------------------------------------
__fastcall TsessionMgrForm::TsessionMgrForm(TComponent* Owner)
	: TForm(Owner)
{
}
//---------------------------------------------------------------------------
void __fastcall TsessionMgrForm::Button1Click(TObject *Sender)
{
	char msg[1024];
	
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


	
	switch (RadioGroup1->ItemIndex) {

	case 1:
		ADOQuery1->SQL->Text = "select a.id, rpad(a.comm_name, 40, ' ') as IP, a.client_pid, "
		                       "       trunc(b.total_time/1000000, 2) as runTime "
							   "from v$session a, v$statement b "
							   "where b.begin_flag = 1 "
							   "and (b.Total_time/1000000) > 10 "
							   "and a.id = b.session_id " ;
		break;
	case 2:
		ADOQuery1->SQL->Text = "select  ss.id, rpad(ss.comm_name, 40, ' ') as IP, ss.client_pid, st.id, (base_time - tr.FIRST_UPDATE_TIME) as utrans_time "
							   "from V$TRANSACTION tr, v$statement st, v$sessionmgr, v$session ss "
							   "where tr.id = st.tx_id     "
							   "and st.session_id = ss.id  "
							   "and tr.FIRST_UPDATE_TIME != 0 "
							   "and (base_time - tr.FIRST_UPDATE_TIME) > 60 ";
		break;
	case 3:
		ADOQuery1->SQL->Text = "select a.id, rpad(comm_name, 40, ' ') as IP,  a.client_pid, b.trans_id, c.id as stmt_id, "
							   " (select id from v$session "
							   "  where id = (select session_id from v$statement "
							   "              where tx_id = b.WAIT_FOR_TRANS_ID) "
							   "  ) as waitSession, b.wait_for_trans_id as waitTID "
							   "from v$session a,  v$lock_wait b, v$statement c "
							   "where b.trans_id   = c.tx_id "
							   "and   c.session_id = a.id " ;
		break;
	default:
		ADOQuery1->SQL->Text = "select id, rpad(comm_name, 41, ' ') as IP, client_pid, OPENED_STMT_COUNT, "
		                       " rpad(DB_USERNAME, 20, ' ') as userName,  AUTOCOMMIT_FLAG  from v$session";
        ;
	}

    try {
		ADOQuery1->Open();
		ADOQuery1->Last();
		ADOQuery1->First();

		if (RadioGroup1->ItemIndex == 1 ||
			RadioGroup1->ItemIndex == 2 ||
			RadioGroup1->ItemIndex == 3)
		{
			if (ADOQuery1->RecordCount > 0)
			{
                mainCallForm->CopyToDown();
				sprintf(msg, "Session Event Type [%s] : CHECK SessionMgr Screen ",
								 RadioGroup1->Items->Strings[RadioGroup1->ItemIndex].c_str() );
				mainCallForm->EVENT->Cells[0][1] = Now().DateTimeString() ;
				mainCallForm->EVENT->Cells[1][1] = DSN->Caption;
				mainCallForm->EVENT->Cells[2][1] = sessionMgrForm->Caption;
				mainCallForm->EVENT->Cells[3][1] = msg;
			}

		}
	} catch (...) {
		CheckBox1->Checked = false;
		Timer1->Enabled = false;
		return;
	}

	if (CheckBox1->Checked == true)
	{
		Timer1->Enabled = true;
	}

}
//---------------------------------------------------------------------------
void __fastcall TsessionMgrForm::DBGrid1DblClick(TObject *Sender)
{
	AnsiString SID = DBGrid1->Fields[0]->AsString;
    AnsiString waitSID;
	//ShowMessage(SID);
	if (SID.Length() == 0)
	{
        return;    
	}
	if (RadioGroup1->ItemIndex == 3)
	{
        waitSID = DBGrid1->Fields[5]->AsString;
		ADOQuery2->SQL->Text = "select  session_id, id, 'waitSession' as sname, tx_id, execute_flag, total_time,  "
							   "       execute_success, execute_failure, mem_cursor_full_scan, disk_cursor_full_scan "
							   " from v$statement "
							   "where session_id = " + SID +
							   " union "
							   "select  session_id, id, 'grantSession' as sname, tx_id, execute_flag, total_time,  "
							   "       execute_success, execute_failure, mem_cursor_full_scan, disk_cursor_full_scan "
							   " from v$statement "
							   "where session_id = " + waitSID +
							   " order by session_id, Total_time desc " ;
	} else {
		ADOQuery2->SQL->Text = "select session_id, id, tx_id, execute_flag, total_time,  "
							   "       execute_success, execute_failure, mem_cursor_full_scan, disk_cursor_full_scan "
							   "from v$statement "
							   " where session_id = " + SID +
							   " order by Total_time desc ";
	}
	
	ADOQuery2->Open();
}
//---------------------------------------------------------------------------
void __fastcall TsessionMgrForm::FormClose(TObject *Sender,
      TCloseAction &Action)
{
    Action = caFree;	
}
//---------------------------------------------------------------------------
void __fastcall TsessionMgrForm::FormResize(TObject *Sender)
{
	DBGrid1->Top    = Panel1->Top + Panel1->Height;
	DBGrid1->Height = ( (Panel1->Top + Panel1->Height) + DSN->Top )  / 2 - DBGrid1->Top ;

    DBGrid1->Align = alTop;
	DBGrid2->Align = alClient;
}
//---------------------------------------------------------------------------
void __fastcall TsessionMgrForm::CheckBox1Click(TObject *Sender)
{
	if (CheckBox1->Checked == true)
	{
		Timer1->Enabled = true;
	}else {
        Timer1->Enabled = false;
    }

}
//---------------------------------------------------------------------------
void __fastcall TsessionMgrForm::Timer1Timer(TObject *Sender)
{
	Timer1->Interval = (int)atoi(mainCallForm->SleepTime->Text.c_str()) * 1000;
	Button1Click(this);
}
//---------------------------------------------------------------------------
void __fastcall TsessionMgrForm::FormShow(TObject *Sender)
{
    Button1Click(this);	
}
//---------------------------------------------------------------------------


void __fastcall TsessionMgrForm::DBGrid2DblClick(TObject *Sender)
{
	AnsiString sid = DBGrid2->Fields[0]->AsFloat;
	AnsiString stmt = DBGrid2->Fields[1]->AsFloat;
	AnsiString statement1 = "";
	int i;

	
	ADOQuery3->SQL->Text = "select text from v$sqltext where sid = " + sid + " and stmt_id = " + stmt +
						   " order by piece asc ";
	//ShowMessage( ADOQuery3->SQL->Text );
    Memo1->Clear();
	try {
      	ADOQuery3->Open();  
		ADOQuery3->Last();
		ADOQuery3->First();

		for (i = 0; i < ADOQuery3->RecordCount; i++)
		{
			statement1 = statement1 + ADOQuery3->Fields->FieldByNumber(1)->AsString;
            //ShowMessage(statement1);
			ADOQuery3->Next();
		}

		Memo1->Text = statement1;
	} catch (...) {
		CheckBox1->Checked = false;
		Timer1->Enabled = false ;
		return;
	}

	if (statement1.Length() > 0) {
        Memo1->Visible = true;
	}



}
//---------------------------------------------------------------------------

void __fastcall TsessionMgrForm::Memo1DblClick(TObject *Sender)
{
    Memo1->Visible = false;	
}
//---------------------------------------------------------------------------

