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

#include "winISQL.h"
#include "winISQL_dsnManager.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TForm4 *Form4;

typedef struct conn_node
{
	int idx;
	conn_node * next;
}conn_node;

int free_conn_cnt;
conn_node * free_conn_head;
conn_node * free_conn_tail;
conn_node free_conn_node[7];

//LPEXCEPTION_POINTERS e;
//---------------------------------------------------------------------------
__fastcall TForm4::TForm4(TComponent* Owner)
	: TForm(Owner)
{
	free_conn_cnt = 7;
	free_conn_head = NULL;
	free_conn_tail = NULL;

	for (i = 0; i < 7; i++) {
		free_conn_node[i].idx = i+1;
		free_conn_node[i].next = NULL;

		if (free_conn_head == NULL) {
			free_conn_head = &free_conn_node[i];
			free_conn_tail = free_conn_head;
		}
		else {
			free_conn_tail->next = &free_conn_node[i];
			free_conn_tail = &free_conn_node[i];
		}
	}
}
//---------------------------------------------------------------------------
void __fastcall TForm4::ToolButton1Click(TObject *Sender)
{
	Query1->Close();
	Query1->SQL->Clear();
/*
	UpdateSQL1->InsertSQL->Clear();
	UpdateSQL1->DeleteSQL->Clear();
	UpdateSQL1->ModifySQL->Clear();
	ListBox2->Items->AddStrings(Memo1->Lines->substring(1, 6));
*/
	Query1->SQL->AddStrings(RichEdit11->Lines);
//	Query2->Open();
	Query1->ExecSQL();
/*
	try {
	Query2->Close();
	Query2->SQL->Clear();
	UpdateSQL1->InsertSQL->Clear();
	UpdateSQL1->DeleteSQL->Clear();
	UpdateSQL1->ModifySQL->Clear();
	UpdateSQL1->InsertSQL->AddStrings(Memo1->Lines);
	UpdateSQL1->ExecSQL(ukInsert);
	//Query2->Open();
/*   	sprintf(query,  "select a.parse "
					"from system_.sys_proc_parse_ a , system_.sys_users_ b, system_.sys_procedures_ c "
					"where b.user_name = '%s' "
					"and   a.user_id = b.user_id "
					"and   a.user_id = c.user_id "
					"and   c.proc_name = '%s' "
					"and   a.proc_oid = c.proc_oid order by a.seq_no",  user.c_str(), pname.c_str());
	if (SQLExecDirect(stmt, query, SQL_NTS) != SQL_SUCCESS)
	{
		dbErrMsg(env, dbc, stmt);
		SQLFreeStmt(stmt, SQL_DROP);
		freeDBHandle();
		return "";
	}

	} catch (Exception &exception)
	{
		//e = GetExceptionInformation();
		//ListBox1->Items->AddStrings((TStrings * )e->ExceptionRecord->ExceptionInformation);
		//Application->ShowMessage((AnsiString(exception.ClassName())+exception.Message);
		Application->ShowException(&exception);
	}

	ListBox2->Items->AddStrings(Memo1->Lines);
    */
}
//---------------------------------------------------------------------------

void __fastcall TForm4::ToolButton2Click(TObject *Sender)
{
//	ListBox2->Clear();
	//Memo2->Lines->Clear();
}
//---------------------------------------------------------------------------

void __fastcall TForm4::ToolButton3Click(TObject *Sender)
{
	//Memo2->Lines->AddStrings(Memo1->Lines);
}
//---------------------------------------------------------------------------



void __fastcall TForm4::DSNManager1Click(TObject *Sender)
{
	Form11->Show();
}
//---------------------------------------------------------------------------

void __fastcall TForm4::FormShow(TObject *Sender)
{
	Form11->GetDsnList_winISQL(this->ListBox1);
}
//---------------------------------------------------------------------------

/*		Form4->Database2->AliasName = ListBox1->Items->Strings[ListBox1->ItemIndex];
		Form4->Query2->DatabaseName = ListBox1->Items->Strings[ListBox1->ItemIndex];
		Form4->DataSource2->DataSet = Form4->Query2;
		Form4->DBGrid21->DataSource = Form4->DataSource2;
		Form4->PageControl1->ActivePage = TabSheet2;
		Form4->TabSheet2->Visible = true;
		*/

void __fastcall TForm4::Connect2Click(TObject *Sender)
{
	if (free_conn_cnt == 0) {
		ShowMessage("You can't try more connection because the max connection count is seven in winISQL.");
	}

	switch (free_conn_head->idx) {
	case 1 :
		Form4->Database1->AliasName = ListBox1->Items->Strings[ListBox1->ItemIndex];
		Form4->Query1->DatabaseName = ListBox1->Items->Strings[ListBox1->ItemIndex];
		Form4->DataSource1->DataSet = Form4->Query1;
		Form4->DBGrid11->DataSource = Form4->DataSource1;
		Form4->PageControl1->ActivePage = TabSheet1;
		Form4->TabSheet1->Visible = true;
		Form4->TabSheet1->TabVisible = true;
		Form4->TabSheet1->Caption = ListBox1->Items->Strings[ListBox1->ItemIndex];
		break;
	case 2 :
		Form4->Database2->AliasName = ListBox1->Items->Strings[ListBox1->ItemIndex];
		Form4->Query2->DatabaseName = ListBox1->Items->Strings[ListBox1->ItemIndex];
		Form4->DataSource2->DataSet = Form4->Query2;
		Form4->DBGrid21->DataSource = Form4->DataSource2;
		Form4->PageControl1->ActivePage = TabSheet2;
		Form4->TabSheet2->Visible = true;
		Form4->TabSheet2->TabVisible = true;
		Form4->TabSheet2->Caption = ListBox1->Items->Strings[ListBox1->ItemIndex];
		break;
	case 3 :
		Form4->Database3->AliasName = ListBox1->Items->Strings[ListBox1->ItemIndex];
		Form4->Query3->DatabaseName = ListBox1->Items->Strings[ListBox1->ItemIndex];
		Form4->DataSource3->DataSet = Form4->Query3;
		Form4->DBGrid31->DataSource = Form4->DataSource3;
		Form4->PageControl1->ActivePage = TabSheet3;
		Form4->TabSheet3->Visible = true;
		Form4->TabSheet3->TabVisible = true;
		Form4->TabSheet3->Caption = ListBox1->Items->Strings[ListBox1->ItemIndex];
		break;
	case 4 :
		Form4->Database4->AliasName = ListBox1->Items->Strings[ListBox1->ItemIndex];
		Form4->Query4->DatabaseName = ListBox1->Items->Strings[ListBox1->ItemIndex];
		Form4->DataSource4->DataSet = Form4->Query4;
		Form4->DBGrid41->DataSource = Form4->DataSource4;
		Form4->PageControl1->ActivePage = TabSheet4;
		Form4->TabSheet4->Visible = true;
		Form4->TabSheet4->TabVisible = true;
		Form4->TabSheet4->Caption = ListBox1->Items->Strings[ListBox1->ItemIndex];
		break;
	case 5 :
		Form4->Database5->AliasName = ListBox1->Items->Strings[ListBox1->ItemIndex];
		Form4->Query5->DatabaseName = ListBox1->Items->Strings[ListBox1->ItemIndex];
		Form4->DataSource5->DataSet = Form4->Query5;
		Form4->DBGrid51->DataSource = Form4->DataSource5;
		Form4->PageControl1->ActivePage = TabSheet5;
		Form4->TabSheet5->Visible = true;
		Form4->TabSheet5->TabVisible = true;
		Form4->TabSheet5->Caption = ListBox1->Items->Strings[ListBox1->ItemIndex];
		break;
	case 6 :
		Form4->Database6->AliasName = ListBox1->Items->Strings[ListBox1->ItemIndex];
		Form4->Query6->DatabaseName = ListBox1->Items->Strings[ListBox1->ItemIndex];
		Form4->DataSource6->DataSet = Form4->Query6;
		Form4->DBGrid61->DataSource = Form4->DataSource6;
		Form4->PageControl1->ActivePage = TabSheet6;
		Form4->TabSheet6->Visible = true;
		Form4->TabSheet6->TabVisible = true;
		Form4->TabSheet6->Caption = ListBox1->Items->Strings[ListBox1->ItemIndex];
		break;
	case 7 :
		Form4->Database7->AliasName = ListBox1->Items->Strings[ListBox1->ItemIndex];
		Form4->Query7->DatabaseName = ListBox1->Items->Strings[ListBox1->ItemIndex];
		Form4->DataSource7->DataSet = Form4->Query7;
		Form4->DBGrid71->DataSource = Form4->DataSource7;
		Form4->PageControl1->ActivePage = TabSheet7;
		Form4->TabSheet7->Visible = true;
		Form4->TabSheet7->TabVisible = true;
		Form4->TabSheet7->Caption = ListBox1->Items->Strings[ListBox1->ItemIndex];
		break;
	};

	free_conn_head = free_conn_head->next;
	if (free_conn_head == NULL)
	{
		free_conn_tail = NULL;
	}
	free_conn_cnt--;
}
//---------------------------------------------------------------------------
void __fastcall TForm4::Disconnect2Click(TObject *Sender)
{
	Form4->PageControl1->ActivePage->Visible = false;
	Form4->PageControl1->ActivePage->TabVisible = false;
	Form4->PageControl1->ActivePage->Caption = "";
	conn_idx--;
}
//---------------------------------------------------------------------------

void __fastcall TForm4::Disconnect3Click(TObject *Sender)
{
	switch (free_conn_head->idx) {
	case 1 :
		Form4->Database1->AliasName = ListBox1->Items->Strings[ListBox1->ItemIndex];
		Form4->TabSheet1->Visible = false;
		Form4->TabSheet1->TabVisible = false;
		break;
	case 2 :
		Form4->Database2->AliasName = ListBox1->Items->Strings[ListBox1->ItemIndex];
		Form4->TabSheet2->Visible = false;
		Form4->TabSheet2->TabVisible = false;
		break;
	case 3 :
		Form4->Database3->AliasName = ListBox1->Items->Strings[ListBox1->ItemIndex];
		Form4->TabSheet3->Visible = false;
		Form4->TabSheet3->TabVisible = false;
		break;
	case 4 :
		Form4->Database4->AliasName = ListBox1->Items->Strings[ListBox1->ItemIndex];
		Form4->TabSheet4->Visible = false;
		Form4->TabSheet4->TabVisible = false;
		break;
	case 5 :
		Form4->Database5->AliasName = ListBox1->Items->Strings[ListBox1->ItemIndex];
		Form4->TabSheet5->Visible = false;
		Form4->TabSheet5->TabVisible = false;
		break;
	case 6 :
		Form4->Database6->AliasName = ListBox1->Items->Strings[ListBox1->ItemIndex];
		Form4->TabSheet6->Visible = false;
		Form4->TabSheet6->TabVisible = false;
		break;
	case 7 :
		Form4->Database7->AliasName = ListBox1->Items->Strings[ListBox1->ItemIndex];
		Form4->TabSheet7->Visible = false;
		Form4->TabSheet7->TabVisible = false;
		break;
	};
	if (free_conn_tail == NULL)
	{
		free_conn_head = free_conn_tail = &free_conn_node[idx-1];
		free_conn_node[idx-1].next = NULL;
	}
	else
	{
		free_conn_tail->next = &free_conn_node[idx-1];
		free_conn_tail = &free_conn_node[idx-1];
	}
	free_conn_cnt++;
}
//---------------------------------------------------------------------------


