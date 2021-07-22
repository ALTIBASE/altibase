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

#include "memtblForm1.h"
#include "mainCall.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TmemtblMgrForm *memtblMgrForm;
//---------------------------------------------------------------------------
__fastcall TmemtblMgrForm::TmemtblMgrForm(TComponent* Owner)
	: TForm(Owner)
{
}
//---------------------------------------------------------------------------
void __fastcall TmemtblMgrForm::CheckBox1Click(TObject *Sender)
{
	if (CheckBox1->Checked == true)
	{
		Timer1->Enabled = true;
	}else {
        Timer1->Enabled = false;
    }	
}
//---------------------------------------------------------------------------
void __fastcall TmemtblMgrForm::Timer1Timer(TObject *Sender)
{
    Timer1->Interval = (int)atoi(mainCallForm->SleepTime->Text.c_str()) * 1000;
	Button1Click(this);	
}
//---------------------------------------------------------------------------
void __fastcall TmemtblMgrForm::Button1Click(TObject *Sender)
{
	int i;
	double alloc_sum = 0, used_sum = 0;
    char temp[100], msg[1024];

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

	if (RadioGroup1->ItemIndex == 0) {
        ADOQuery1->SQL->Text = "select rpad(a.table_name, 40, ' ') as tblname, "
							   "       (b.fixed_alloc_mem+b.var_alloc_mem) as alloc_mem, "
							   "       (b.fixed_used_mem+b.var_used_mem) as used_mem, "
							   "       trunc((b.fixed_used_mem+b.var_used_mem)/(b.fixed_alloc_mem+b.var_alloc_mem) * 100.0, 2) as ratio "
							   " from system_.sys_tables_ a, v$memtbl_info b "
							   " where a.table_oid = b.table_oid "
							   " and a.table_type = 'T' "
						       " order by 1 asc, 2 desc ";
	} else if (RadioGroup1->ItemIndex == 1)
	{
		ADOQuery1->SQL->Text = "select rpad(a.table_name, 40, ' ') as tblname, "
							   "       (b.fixed_alloc_mem+b.var_alloc_mem) as alloc_mem, "
							   "       (b.fixed_used_mem+b.var_used_mem) as used_mem, "
							   "       trunc((b.fixed_used_mem+b.var_used_mem)/(b.fixed_alloc_mem+b.var_alloc_mem) * 100.0, 2) as ratio "
							   " from system_.sys_tables_ a, v$memtbl_info b "
							   " where a.table_oid = b.table_oid "
							   " and a.table_type = 'T' "
						       " order by 2 desc, 1 asc ";
	} else if (RadioGroup1->ItemIndex == 2)
	{
		ADOQuery1->SQL->Text = "select rpad(a.table_name, 40, ' ') as tblname, "
							   "       (b.fixed_alloc_mem+b.var_alloc_mem) as alloc_mem, "
							   "       (b.fixed_used_mem+b.var_used_mem) as used_mem, "
							   "       trunc((b.fixed_used_mem+b.var_used_mem)/(b.fixed_alloc_mem+b.var_alloc_mem) * 100.0, 2) as ratio "
							   " from system_.sys_tables_ a, v$memtbl_info b "
							   " where a.table_oid = b.table_oid "
							   " and a.table_type = 'T' "
						       " order by 4 desc, 1 asc ";
	} else {
	    ADOQuery1->SQL->Text = "select rpad(a.table_name, 40, ' ') as tblname, "
							   "       (b.fixed_alloc_mem+b.var_alloc_mem) as alloc_mem, "
							   "       (b.fixed_used_mem+b.var_used_mem) as used_mem, "
							   "       trunc((b.fixed_used_mem+b.var_used_mem)/(b.fixed_alloc_mem+b.var_alloc_mem) * 100.0, 2) as ratio "
							   " from system_.sys_tables_ a, v$memtbl_info b "
							   " where a.table_oid = b.table_oid "
							   " and a.table_type = 'T' "
							   " order by 2 desc, 1 asc ";
	}
	
	try {
		ADOQuery1->Open();
		ADOQuery1->Last();
		ADOQuery1->First();

		for (i = 0; i < ADOQuery1->RecordCount ; i++)
		{
			alloc_sum = alloc_sum + ADOQuery1->Fields->FieldByNumber(2)->AsFloat;
			used_sum  = used_sum + ADOQuery1->Fields->FieldByNumber(3)->AsFloat;

			if (CheckBox2->Checked == true)
			{
				if (ADOQuery1->Fields->FieldByNumber(2)->AsFloat > atof(Edit1->Text.c_str()) )
				{
                    mainCallForm->CopyToDown();
					sprintf(msg, "Table Alloc Size : [%s] : current [%.0f] > threshold [%s]",
                                ADOQuery1->Fields->FieldByNumber(1)->AsString.Trim().c_str(),
								ADOQuery1->Fields->FieldByNumber(2)->AsFloat,
								Edit1->Text.c_str());

					mainCallForm->EVENT->Cells[0][1] = Now().DateTimeString() ;
					mainCallForm->EVENT->Cells[1][1] = DSN->Caption;
					mainCallForm->EVENT->Cells[2][1] = memtblMgrForm->Caption;
					mainCallForm->EVENT->Cells[3][1] = msg;
				}

			}

			ADOQuery1->Next();
		}

		sprintf(temp, " %.0f byte", alloc_sum);
		total_alloc->Text = temp;

		sprintf(temp, " %.0f byte", used_sum);
		total_used->Text = temp;
		
		ADOQuery1->First();	
	} catch (...) {
		CheckBox1->Checked = false;
		Timer1->Enabled = false;
	}

    if (CheckBox1->Checked == true)
	{
		Timer1->Enabled = true;
	}	
}
//---------------------------------------------------------------------------
void __fastcall TmemtblMgrForm::FormClose(TObject *Sender, TCloseAction &Action)
{
    Action = caFree;	
}
//---------------------------------------------------------------------------

void __fastcall TmemtblMgrForm::Button2Click(TObject *Sender)
{
    FILE *fp;
	char fname[255], temp[255];

    sprintf(fname, "%s_%s.cfg", DSN->Caption.c_str(), memtblMgrForm->Caption.c_str());
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

void __fastcall TmemtblMgrForm::Button3Click(TObject *Sender)
{
	FILE *fp;
	char fname[255];
	
	sprintf(fname, "%s_%s.cfg", DSN->Caption.c_str(), memtblMgrForm->Caption.c_str());

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

void __fastcall TmemtblMgrForm::Button4Click(TObject *Sender)
{
    Panel4->Visible = false;	
}
//---------------------------------------------------------------------------

