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

#include "memsTatForm.h"
#include "mainCall.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TMemStat *MemStat;
//---------------------------------------------------------------------------
__fastcall TMemStat::TMemStat(TComponent* Owner)
	: TForm(Owner)
{
}
//---------------------------------------------------------------------------
void __fastcall TMemStat::Button1Click(TObject *Sender)
{
	double alloc_sum = 0, used_sum = 0;
	int i;
	char temp[100];
	char msg[1024];

	Timer2->Enabled = false;

	if (ADOConnection2->Connected == false)
	{
		ADOConnection2->ConnectionString = "DSN=" + DSN->Caption;
		ADOConnection2->Open();
	}

	ADOQuery1->SQL->Text = "alter session set select_header_display = 1";
	try {
		ADOQuery1->ExecSQL();
	} catch (...) {
    	CheckBox1->Checked = false;
		Timer2->Enabled = false;
	}

	
	ADOQuery1->SQL->Text = "select name, alloc_size, alloc_count, max_total_size "
	                       "  from v$memstat order by max_total_size desc ";
	//DBGrid1->Columns->BeginUpdate();
	try {
		ADOQuery1->Open();
		ADOQuery1->Last();
		ADOQuery1->First();

		for (i = 0; i < ADOQuery1->RecordCount ; i++)
		{
			alloc_sum = alloc_sum + ADOQuery1->Fields->FieldByNumber(4)->AsFloat;
			used_sum  = used_sum + ADOQuery1->Fields->FieldByNumber(2)->AsFloat;
			ADOQuery1->Next();
		}

		sprintf(temp, " %.0f byte", alloc_sum);
		total_alloc->Text = temp;

		sprintf(temp, " %.0f byte", used_sum);
		total_used->Text = temp;
		
		ADOQuery1->First();
	} catch (...) {
		CheckBox1->Checked = false;
		Timer2->Enabled = false;
		return;
	}

	if (CheckBox2->Checked == true)
	{
		if ( alloc_sum > atof(Edit1->Text.c_str()) ) {
            mainCallForm->CopyToDown();
			sprintf(msg, "Total Alloc Size : current [%s] > threshold [%s]",
							total_alloc->Text.c_str(), Edit1->Text.c_str());
						
			mainCallForm->EVENT->Cells[0][1] = Now().DateTimeString() ;
			mainCallForm->EVENT->Cells[1][1] = DSN->Caption;
			mainCallForm->EVENT->Cells[2][1] = MemStat->Caption;
			mainCallForm->EVENT->Cells[3][1] = msg;
		}

	}


	if (CheckBox1->Checked == true)
	{
		Timer2->Enabled = true;
	}
}
//---------------------------------------------------------------------------
void __fastcall TMemStat::Timer2Timer(TObject *Sender)
{
    Timer2->Interval = (int)atoi(mainCallForm->SleepTime->Text.c_str()) * 1000;
	Button1Click(this);	
}
//---------------------------------------------------------------------------
void __fastcall TMemStat::CheckBox1Click(TObject *Sender)
{
	if (CheckBox1->Checked == true)
	{
		Timer2->Enabled = true;
	}else {
        Timer2->Enabled = false;
    }	
}
//---------------------------------------------------------------------------
void __fastcall TMemStat::Button2Click(TObject *Sender)
{
	FILE *fp;
	char fname[255], temp[255];

    sprintf(fname, "%s_%s.cfg", DSN->Caption.c_str(), MemStat->Caption.c_str());
	Panel2->Visible = true;
	
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
void __fastcall TMemStat::Button3Click(TObject *Sender)
{
	FILE *fp;
	char fname[255];
	
	sprintf(fname, "%s_%s.cfg", DSN->Caption.c_str(), MemStat->Caption.c_str());

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

	Panel2->Visible = false;
}
//---------------------------------------------------------------------------
void __fastcall TMemStat::Button4Click(TObject *Sender)
{
    Panel2->Visible = false;
}
//---------------------------------------------------------------------------

void __fastcall TMemStat::FormClose(TObject *Sender, TCloseAction &Action)
{
	Action = caFree;
	//Timer1->Enabled = false;	
}
//---------------------------------------------------------------------------
void __fastcall TMemStat::Button5Click(TObject *Sender)
{
	int i;
	FILE *fp;

	ADOQuery1->First();

	if (!SaveDialog1->Execute()) {
		 return;
	}

	fp  = fopen(SaveDialog1->FileName.c_str(), "w+");
	for (i = 0; i < ADOQuery1->RecordCount; i++)
	{
		fprintf(fp, "%s, %.0f, %.0f, %.0f\n",
			 ADOQuery1->Fields->FieldByNumber(1)->AsString.c_str(),
			 ADOQuery1->Fields->FieldByNumber(2)->AsFloat,
			 ADOQuery1->Fields->FieldByNumber(3)->AsFloat,
			 ADOQuery1->Fields->FieldByNumber(4)->AsFloat);
		ADOQuery1->Next();
	}
	fflush(fp);fclose(fp);
}
//---------------------------------------------------------------------------

