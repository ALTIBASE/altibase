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

#include "disktblForm1.h"
#include "tablespaceForm.h"
#include "mainCall.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TdiskTblMgrForm *diskTblMgrForm;
//---------------------------------------------------------------------------
__fastcall TdiskTblMgrForm::TdiskTblMgrForm(TComponent* Owner)
	: TForm(Owner)
{
}
//---------------------------------------------------------------------------
void __fastcall TdiskTblMgrForm::CheckBox1Click(TObject *Sender)
{
	if (CheckBox1->Checked == true)
	{
		Timer1->Enabled = true;
	}else {
		Timer1->Enabled = false;
	}
}
//---------------------------------------------------------------------------
void __fastcall TdiskTblMgrForm::Timer1Timer(TObject *Sender)
{
    Timer1->Interval = (int)atoi(mainCallForm->SleepTime->Text.c_str()) * 1000;
	Button1Click(this);	
}
//---------------------------------------------------------------------------
void __fastcall TdiskTblMgrForm::Button1Click(TObject *Sender)
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

	ADOQuery1->SQL->Text = "select a.table_name, "
						   "         (b.disk_total_page_cnt*"
						   " (select page_size from v$tablespaces where name = '" +TBS_NAME->Text.Trim().UpperCase()+"')) as UsedSize "
						   " from system_.sys_tables_ a , v$disktbl_info b, v$tablespaces c "
						   "where a.table_oid = b.table_oid "
						   " and   a.tbs_id = b.tablespace_id  "
						   " and   a.tbs_id = c.id        "
						   " and   b.tablespace_id = c.id "
						   " and   c.name =  '" + TBS_NAME->Text.Trim().UpperCase() + "'" ;

	try {
		ADOQuery1->Open();
		ADOQuery1->Last();
		ADOQuery1->First();
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
