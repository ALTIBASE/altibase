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

#include "iloaderStatus.h"
#include "mainForm1.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TForm8 *Form8;
//---------------------------------------------------------------------------
__fastcall TForm8::TForm8(TComponent* Owner)
	: TForm(Owner)
{
}
//---------------------------------------------------------------------------
// 화면에서 사용자가 지워줘야 구조체도 free시킨다.
void __fastcall TForm8::Button1Click(TObject *Sender)
{
	if (MessageBox(NULL, "This Command, Try to Stop running process and erase logs you selected.", "Confirm", MB_OKCANCEL) == ID_CANCEL)
	{
        return;    
	}
	Form5->freeIloaderNode();
}
//---------------------------------------------------------------------------
void __fastcall TForm8::FormShow(TObject *Sender)
{
	char msg[1024];

	sprintf(msg, " %-10s %15s.%-40s %-26s %-26s", "PID","USER","TABLE", "STARTIME", "ENDTIME");
	Panel2->Caption = msg;	
}
//---------------------------------------------------------------------------

void __fastcall TForm8::CheckListBox1DblClick(TObject *Sender)
{
	Form5->findTaskIloader();	
}
//---------------------------------------------------------------------------

void __fastcall TForm8::Memo1DblClick(TObject *Sender)
{
    Memo1->Clear();
	Memo1->Visible = false;
}
//---------------------------------------------------------------------------

void __fastcall TForm8::FormCloseQuery(TObject *Sender, bool &CanClose)
{
	Memo1->Clear();
	Memo1->Visible = false;	
}
//---------------------------------------------------------------------------

