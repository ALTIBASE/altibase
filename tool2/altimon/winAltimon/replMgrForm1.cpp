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

#include "mainCall.h"
#include "replMgrForm1.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TreplMgr *replMgr;
//---------------------------------------------------------------------------
__fastcall TreplMgr::TreplMgr(TComponent* Owner)
	: TForm(Owner)
{
}
//---------------------------------------------------------------------------
void __fastcall TreplMgr::CheckBox1Click(TObject *Sender)
{
	if (CheckBox1->Checked == true)
	{
		Timer1->Enabled = true;
	}else {
        Timer1->Enabled = false;
    }	
}
//---------------------------------------------------------------------------
void __fastcall TreplMgr::Timer1Timer(TObject *Sender)
{
    Timer1->Interval = (int)atoi(mainCallForm->SleepTime->Text.c_str()) * 1000;
	Button1Click(this);	
}
//---------------------------------------------------------------------------
void __fastcall TreplMgr::Button1Click(TObject *Sender)
{
	int i;
	char msg[1024];

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

	ADOQuery1->SQL->Text = "select * from v$repgap";

	try {
		ADOQuery1->Open();
		ADOQuery1->Last();
		ADOQuery1->First();

		for (i = 0; i < ADOQuery1->RecordCount ; i++)
		{
			if (CheckBox2->Checked == true)
			{
				if (ADOQuery1->Fields->FieldByNumber(4)->AsFloat > atof(Edit1->Text.c_str()) )
				{
                    mainCallForm->CopyToDown();
					sprintf(msg, "Replication GAP : [%s] : current [%.0f] > threshold [%s]",
                                ADOQuery1->Fields->FieldByNumber(1)->AsString.Trim().c_str(),
								ADOQuery1->Fields->FieldByNumber(4)->AsFloat,
								Edit1->Text.c_str());

					mainCallForm->EVENT->Cells[0][1] = Now().DateTimeString() ;
					mainCallForm->EVENT->Cells[1][1] = DSN->Caption;
					mainCallForm->EVENT->Cells[2][1] = replMgr->Caption;
					mainCallForm->EVENT->Cells[3][1] = msg;
				}

			}

			ADOQuery1->Next();
		}
        ADOQuery1->First();
		
	} catch (...) {
		CheckBox1->Checked = false;
		Timer1->Enabled = false;
	}


	ADOQuery2->SQL->Text = "select rpad(rep_name, 30, ' ') as repName, "
						   "       rpad(peer_ip, 20, ' ') as tip, peer_port, "
						   "       xsn, commit_xsn "
						   " from v$repsender";

	try {
		ADOQuery2->Open();
		//OQuery1->Last();
		//OQuery1->First();

		
	} catch (...) {
		CheckBox1->Checked = false;
		Timer1->Enabled = false;
	}


	ADOQuery3->SQL->Text = "select rpad(rep_name, 30, ' ') as repName, "
						   "       rpad(peer_ip, 20, ' ') as tip, peer_port, "
						   "       apply_xsn "
						   " from v$repreceiver";

	try {
		ADOQuery3->Open();
		//OQuery1->Last();
		//OQuery1->First();

		
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
void __fastcall TreplMgr::FormClose(TObject *Sender, TCloseAction &Action)
{
    Action = caFree;	
}
//---------------------------------------------------------------------------

void __fastcall TreplMgr::Button2Click(TObject *Sender)
{
    FILE *fp;
	char fname[255], temp[255];

    sprintf(fname, "%s_%s.cfg", DSN->Caption.c_str(), replMgr->Caption.c_str());
	Panel4->Visible = true;
	
	fp = fopen (fname, "r");
	if (fp == NULL) {
        Edit1->Text = "";
		return;
	}
	memset(temp, 0x00, sizeof(temp));
	fscanf(fp, "%s\n", temp);
	fclose(fp);

	Edit1->Text = temp;	
}
//---------------------------------------------------------------------------

void __fastcall TreplMgr::Button4Click(TObject *Sender)
{
    Panel4->Visible = false;	
}
//---------------------------------------------------------------------------

void __fastcall TreplMgr::Button3Click(TObject *Sender)
{
	FILE *fp;
	char fname[255];
	
	sprintf(fname, "%s_%s.cfg", DSN->Caption.c_str(), replMgr->Caption.c_str());

	fp = fopen (fname, "w+");
	fprintf(fp, "%s\n", Edit1->Text.c_str());
	fclose(fp);

	if (CheckBox2->Checked == true)
	{
		CheckBox2->Checked = false;
		Button3->Caption = "Start";
	} else {
        CheckBox2->Checked = true;
		Button3->Caption = "Stop";
    }

	Panel4->Visible = false;	
}
//---------------------------------------------------------------------------

