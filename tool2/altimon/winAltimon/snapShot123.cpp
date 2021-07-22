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

#include "snapShot123.h"
#include "mainCall.h"
#include "sessionMgr.h"
#include "memsTatForm.h"
#include "memtblForm1.h"
#include "replMgrForm1.h"

//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TSnapShotForm *SnapShotForm;
//---------------------------------------------------------------------------
__fastcall TSnapShotForm::TSnapShotForm(TComponent* Owner)
	: TForm(Owner)
{
}
//---------------------------------------------------------------------------
void __fastcall TSnapShotForm::Button1Click(TObject *Sender)
{
	char temp[1024];
	char msg[1024];
	FILE *fp;
	char fname[255];
	int i;
    AnsiString curr = Now().DateTimeString();

	Panel7->Color = clRed;

	sprintf(fname, "%s_%s.log", DSN->Caption.c_str(), "snapshot");
	fp = fopen(fname, "a+");
	fprintf(fp, "DateTime=%-30s,", curr.c_str());

	Timer1->Enabled = false;

	if (ADOConnection1->Connected == false)
	{
		try {
            ADOConnection1->ConnectionString = "DSN=" + DSN->Caption;
			ADOConnection1->Open();
		} catch (...) {
			return;
		}
		
	}

	ADOQuery1->SQL->Text = "alter session set select_header_display = 1";
	try
	{
		ADOQuery1->ExecSQL();
	} catch (...) {
    	CheckBox1->Checked = false;
		Timer1->Enabled = false;
		return;
	}


    //// ÁÖ¼® ±ÍÂú´Ù.
	ADOQuery1->SQL->Text = "select count(*) from v$session";

	try
	{
		ADOQuery1->Open();
		sprintf(temp, " %.0f", ADOQuery1->Fields->FieldByNumber(1)->AsFloat);
		SSCOUNT->Text = temp;
		fprintf(fp, "sessionCount=%-10s,", temp);
    } catch (...) {
		CheckBox1->Checked = false;
		Timer1->Enabled = false;
		return;
	} 


	// long run
	ADOQuery1->SQL->Text = "select count(*) from v$session"
						   " where id in (select session_id from v$statement "
						   "              where (total_time/1000000) > 10) ";

	try
	{
		ADOQuery1->Open();
		sprintf(temp, " %.0f", ADOQuery1->Fields->FieldByNumber(1)->AsFloat);
		WCOUNT->Text = temp;
		fprintf(fp, "longRunSession=%-10s,", temp);
		if (ADOQuery1->Fields->FieldByNumber(1)->AsFloat > 0)
		{
            mainCallForm->CopyToDown();
			sprintf(msg, "Session Event Type [longRunQuery] : CHECK SessionMgr Screen ");
			mainCallForm->EVENT->Cells[0][1] = curr ;
			mainCallForm->EVENT->Cells[1][1] = DSN->Caption;
			mainCallForm->EVENT->Cells[2][1] = SnapShotForm->Caption;
		   	mainCallForm->EVENT->Cells[3][1] = msg;    
		}
    } catch (...) {
		CheckBox1->Checked = false;
		Timer1->Enabled = false;
		return;
	}



	// lock wait
	ADOQuery1->SQL->Text = "select nvl(count(*), 0) from v$session"
						   " where id in (select session_id from v$lock_statement "
						   "              where tx_id in (select trans_id from v$lock_wait)  "
						   "              ) ";

	try
	{
		ADOQuery1->Open();
		sprintf(temp, " %.0f", ADOQuery1->Fields->FieldByNumber(1)->AsFloat);
		LCOUNT->Text = temp;
		fprintf(fp, "lockWaitSession=%-10s,", temp);
		if (ADOQuery1->Fields->FieldByNumber(1)->AsFloat > 0)
		{
            mainCallForm->CopyToDown();
			sprintf(msg, "Session Event Type [lockWaitQuery] : CHECK SessionMgr Screen ");
			mainCallForm->EVENT->Cells[0][1] = curr ;
			mainCallForm->EVENT->Cells[1][1] = DSN->Caption;
			mainCallForm->EVENT->Cells[2][1] = SnapShotForm->Caption;
		   	mainCallForm->EVENT->Cells[3][1] = msg;    
		}
	} catch (...) {
		CheckBox1->Checked = false;
		Timer1->Enabled = false;
		return;
	}

	
    // total MaxAlloc
	ADOQuery1->SQL->Text = "select trunc(sum(max_total_size)/1024/1024, 2), "
						   "       trunc(sum(alloc_size)/1024/1024, 2) "
						   " from v$memstat";
	try
	{
		ADOQuery1->Open();
		sprintf(temp, " %.2f Mbyte", ADOQuery1->Fields->FieldByNumber(1)->AsFloat);
		TOTMEM->Text = temp;
		fprintf(fp, "MaxMem=%-16s,", temp);

		sprintf(temp, " %.2f Mbyte", ADOQuery1->Fields->FieldByNumber(2)->AsFloat);
		TOTUSE->Text = temp;
		fprintf(fp, "UseMem=%-16s,", temp);
	} catch (...) {
		CheckBox1->Checked = false;
		Timer1->Enabled = false;
		return;
	}

	

	// total Used
	ADOQuery1->SQL->Text = "select trunc(MEM_ALLOC_PAGE_COUNT / 1024 * 32, 2), "
						   "       trunc(MEM_FREE_PAGE_COUNT / 1024 * 32, 2) "
						   " from v$database ";
	try
	{
		ADOQuery1->Open();
		sprintf(temp, " %.2f Mbyte", ADOQuery1->Fields->FieldByNumber(1)->AsFloat);
		MEMALLOC->Text = temp;
		fprintf(fp, "memTblAlloc=%-16s,", temp);

		sprintf(temp, " %.2f Mbyte", ADOQuery1->Fields->FieldByNumber(2)->AsFloat);
		MEMFREE->Text = temp;
		fprintf(fp, "memTblUse=%-16s,", temp);
	} catch (...) {
		CheckBox1->Checked = false;
		Timer1->Enabled = false;
		return;
	}
	

    // total Used
	ADOQuery1->SQL->Text = "select trunc(Max_total_size/1024/1024, 2) "
	                       " from v$memstat where name = 'Index_Memory' ";
	try
	{
		ADOQuery1->Open();
		sprintf(temp, " %.2f Mbyte", ADOQuery1->Fields->FieldByNumber(1)->AsFloat);
		INDEXUSE->Text = temp;
		fprintf(fp, "IndexUse=%-16s,", temp);
	} catch (...) {
		CheckBox1->Checked = false;
		Timer1->Enabled = false;
		return;
	}


    // checkpoint
	ADOQuery1->SQL->Text = "select OLDEST_ACTIVE_LOGFILE, CURRENT_LOGFILE "
						   " from v$archive ";
	try
	{
		ADOQuery1->Open();
		OLDLOG->Text = ADOQuery1->Fields->FieldByNumber(1)->AsFloat;
		CURLOG->Text = ADOQuery1->Fields->FieldByNumber(2)->AsFloat;
		fprintf(fp, "OldLog=%-16.0f,CurrentLog=%-16.0f,", ADOQuery1->Fields->FieldByNumber(1)->AsFloat,
		                               ADOQuery1->Fields->FieldByNumber(2)->AsFloat );
	} catch (...) {
		CheckBox1->Checked = false;
		Timer1->Enabled = false;
		return;
	}


	// replication;
	ADOQuery1->SQL->Text = "select rpad(a.rep_name, 40, ' '), a.rep_gap,  "
						   "       case2 ( (select count(*) from v$repsender where rep_name = a.rep_name) >= 1, 'OK', 'SenderFail'), "
						   "       case2 ( (select count(*) from v$repsender where rep_name = a.rep_name) >= 1, 'OK', 'ReceiverFail') "
						   " from v$repGap A";
    ComboBox1->Clear();
	try
	{
		ADOQuery1->Open();
		ADOQuery1->Last();
		ADOQuery1->First();
		for (i = 0; i < ADOQuery1->RecordCount ; i++)
		{
			sprintf(temp, "[%-40s] Gap = %-12.0f  (sender:%-15s, receiver:%-15s)",
					  ADOQuery1->Fields->FieldByNumber(1)->AsString.Trim().c_str(),
					  ADOQuery1->Fields->FieldByNumber(2)->AsFloat,
					  ADOQuery1->Fields->FieldByNumber(3)->AsString.Trim().c_str(),
					  ADOQuery1->Fields->FieldByNumber(4)->AsString.Trim().c_str());
			ComboBox1->Items->Add(temp);
			fprintf(fp, "%-300s,", temp);
			ADOQuery1->Next();
        }
	} catch (...) {
		CheckBox1->Checked = false;
		Timer1->Enabled = false;
		return;
	}
    ComboBox1->ItemIndex = 0;

	fprintf(fp, "\n");
    fflush(fp);
	fclose(fp);

	if (CheckBox1->Checked == true)
	{
		Timer1->Enabled = true;
	}

	Panel7->Color = clBlue;
}
//---------------------------------------------------------------------------
void __fastcall TSnapShotForm::CheckBox1Click(TObject *Sender)
{
	if (CheckBox1->Checked == true)
	{
		Timer1->Enabled = true;
	}else {
        Timer1->Enabled = false;
    }	
}
//---------------------------------------------------------------------------
void __fastcall TSnapShotForm::Timer1Timer(TObject *Sender)
{
    Timer1->Interval = (int)atoi(mainCallForm->SleepTime->Text.c_str()) * 1000;
	Button1Click(this);		
}
//---------------------------------------------------------------------------
void __fastcall TSnapShotForm::FormClose(TObject *Sender, TCloseAction &Action)
{
    Action = caFree;	
}
//---------------------------------------------------------------------------
void __fastcall TSnapShotForm::GroupBox1DblClick(TObject *Sender)
{
	TsessionMgrForm *f1 = new TsessionMgrForm(Application);
	f1->DSN->Caption = DSN->Caption;
	f1->Show();
	f1->Button1Click(this);
}
//---------------------------------------------------------------------------
void __fastcall TSnapShotForm::GroupBox2DblClick(TObject *Sender)
{
	TMemStat *f1 = new TMemStat(Application);
	f1->DSN->Caption = DSN->Caption;
	f1->Show();
	f1->Button1Click(this);
}
//---------------------------------------------------------------------------
void __fastcall TSnapShotForm::GroupBox3DblClick(TObject *Sender)
{
	TmemtblMgrForm *f1 = new TmemtblMgrForm(Application);
	f1->DSN->Caption = DSN->Caption;
	f1->Show();
	f1->Button1Click(this);
}
//---------------------------------------------------------------------------
void __fastcall TSnapShotForm::GroupBox5DblClick(TObject *Sender)
{
	TreplMgr *f1 = new TreplMgr(Application);
	f1->DSN->Caption = DSN->Caption;
	f1->Show();
	f1->Button1Click(this);
}
//---------------------------------------------------------------------------

void __fastcall TSnapShotForm::Button2Click(TObject *Sender)
{
	STARTUPINFO          _si;
	PROCESS_INFORMATION  _pi;
	TDateTime t;
	DWORD dwExitCode;
	char runx[1024];

	ZeroMemory( &_si, sizeof(STARTUPINFO) );
	ZeroMemory( &_pi, sizeof(PROCESS_INFORMATION) );

	_si.cb            = sizeof( STARTUPINFO );
	_si.dwFlags       = STARTF_USESHOWWINDOW;
	_si.wShowWindow   = SW_SHOWNORMAL;

    sprintf(runx, "notepad %s_%s.log", DSN->Caption.c_str(), "snapshot");
	CreateProcess( NULL, runx, NULL, NULL, FALSE, DETACHED_PROCESS, NULL, NULL, &_si, &_pi );
}
//---------------------------------------------------------------------------

