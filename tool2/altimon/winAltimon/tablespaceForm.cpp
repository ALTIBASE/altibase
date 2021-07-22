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

#include "tablespaceForm.h"
#include "mainCall.h"
#include "disktblForm1.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TtblForm *tblForm;
//---------------------------------------------------------------------------
__fastcall TtblForm::TtblForm(TComponent* Owner)
	: TForm(Owner)
{
}
//---------------------------------------------------------------------------
void __fastcall TtblForm::Button1Click(TObject *Sender)
{

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
		return;
	}

	ADOQuery1->SQL->Text = "select rpad(maxDB, 14, ' ') as MaxDB_Size, usedSize, AllocSize, "
						   "       rpad(trunc((usedSize/maxDB)* 100.0, 2)  || '%', 10, ' ') as use_Rate , "
						   "       rpad(trunc((allocSize/maxdb) * 100.0, 2) || '%', 10, ' ') as alloc_Rate "
						   " from (        "
						   " select    "
						   "	 (select value1 from v$property where name = 'MEM_MAX_DB_SIZE') as maxDB, "
						   "     (select alloc_size from v$memstat where name = 'Storage_Memory_Manager') as usedSize, "
						   "     (select max_total_size from v$memstat where name = 'Storage_Memory_Manager') as AllocSize "
						   " from v$memstat limit 1) ";

    try {
		ADOQuery1->Open();
	} catch (...) {
		CheckBox1->Checked = false;
		Timer1->Enabled = false;
		return;
	}


	ADOQuery2->SQL->Text = " select rpad(t.name, 40, ' ') as tbs_Name,      "
						   "	d.avail * t.page_size as alloc_Size,        "
						   "	t.used_page_cnt * t.page_size as used_Size, "
						   "	rpad(trunc ( t.USED_PAGE_CNT / d.avail *  100 , 2 ) || '%', 10, ' ') as used_per "
						   " from                                                                         "
						   " (                                                                           "
						   "	select spaceid, sum(decode(autoextend, 0, currsize, 1, maxsize) ) avail "
						   "	from v$datafiles    "
						   "	group by spaceid    "
						   " ) d, V$TABLESPACES t   "
						   " where t.id = d.spaceid " ;
	try {
		ADOQuery2->Open();
	} catch (...) {
		CheckBox1->Checked = false;
		Timer1->Enabled = false;
		return;
	} 

	if (CheckBox1->Checked == true)
	{
		Timer1->Enabled = true;
	}else {
		Timer1->Enabled = false;
    }
}
//---------------------------------------------------------------------------
void __fastcall TtblForm::Timer1Timer(TObject *Sender)
{
    Timer1->Interval = (int)atoi(mainCallForm->SleepTime->Text.c_str()) * 1000;
	Button1Click(this);
}
//---------------------------------------------------------------------------
void __fastcall TtblForm::CheckBox1Click(TObject *Sender)
{
	if (CheckBox1->Checked == true)
	{
		Timer1->Enabled = true;
	}else {
		Timer1->Enabled = false;
	}
}
//---------------------------------------------------------------------------
void __fastcall TtblForm::FormClose(TObject *Sender, TCloseAction &Action)
{
    Action = caFree;	
}
//---------------------------------------------------------------------------

void __fastcall TtblForm::DBGrid2DblClick(TObject *Sender)
{
	if (DBGrid2->Fields[0]->AsString.Length() == 0)
	{
		return;
	}

	TdiskTblMgrForm *f1 = new TdiskTblMgrForm(Application);
	f1->DSN->Caption = DSN->Caption;
	f1->TBS_NAME->Text = DBGrid2->Fields[0]->AsString;
	f1->Show();
	f1->Button1Click(this);
}
//---------------------------------------------------------------------------

