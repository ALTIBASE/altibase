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
#include "sessionMgr.h"
#include "lockMgr.h"
#include "tablespaceForm.h"
#include "memsTatForm.h"
#include "memtblForm1.h"
#include "replMgrForm1.h"
#include "snapShot123.h"
    
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TmainCallForm *mainCallForm;
//---------------------------------------------------------------------------
__fastcall TmainCallForm::TmainCallForm(TComponent* Owner)
	: TForm(Owner)
{
}
//---------------------------------------------------------------------------
void __fastcall TmainCallForm::Button1Click(TObject *Sender)
{
	if (DSNLIST->Text.Length() == 0)
	{
		ShowMessage("Select ODBC DSN you want to Monitor.");
		return;
	}

	TsessionMgrForm *f1 = new TsessionMgrForm(Application);
	f1->DSN->Caption = DSNLIST->Text;
	f1->Show();
}
//---------------------------------------------------------------------------
// DSN LIST를 가져오는 함수
void __fastcall TmainCallForm::GetDsnList()
{
    HKEY hKey, hKey2;
	DWORD value_type, length;
	DWORD key_num;
	DWORD subkey_length;
	TCHAR  ByVal1[1024], sBuf[1024], subkey_name[1024], sBuf2[1024];
	FILETIME file_time;
	AnsiString x;

	// MainRootKey를 연다.
	wsprintf(sBuf , "Software\\ODBC\\ODBC.INI");
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
				  sBuf,
				  0,
				  KEY_ALL_ACCESS,
				  &hKey) != 0)
	{
        ShowMessage("RegOpenKeyEx-1 Fail!!");
		return;
	}

	key_num = 0;
    DSNLIST->Clear();
	
	// Enum이 에러가 날때까지 ODBC.INI를 뒤진다.
	while (1)
	{
		subkey_length = 1024;
		memset(subkey_name , 0x00, sizeof(subkey_name));
		// 이 함수를 통하면 DSNLIST가 나온다.
		if (RegEnumKeyEx( hKey,
						  key_num,
						  subkey_name,
						  &subkey_length,
						  0,
						  0 ,
						  0 ,
						  &file_time) != 0)
		{
			//ShowMessage("RegEnumKeyEx-1 Fail!!");
			break;
		}

		// DSN명을 가지고 다시 Key를 연다.
		wsprintf(sBuf, "Software\\ODBC\\ODBC.INI\\%s", subkey_name);
		if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
						 sBuf,
						 0,
						 KEY_ALL_ACCESS,
						 &hKey2) != 0)
		{
			//ShowMessage("RegOpenKeyEx-2 Fail");
			break;
		}

		// 열린 Key에서 Dirver가 Altibase용인지 확인한다.
		length = 1024;
		value_type = NULL;
		memset(ByVal1 , 0x00, sizeof(ByVal1));
		wsprintf(sBuf2, "Driver");
		if (RegQueryValueEx(hKey2,
							sBuf2,
							0,
							&value_type,
							ByVal1,
							&length) == 0)
		{
			// AltibaseDLL을 쓰는놈이냐?
		   AnsiString x = ByVal1;
		   int c;

		   // a4_CM451.dll 이다.
		   c = x.Pos("a4_");
		   if (c != 0)
		   {
              // ListBox에 등록한다.
			  DSNLIST->Items->Add(subkey_name) ;
		   }
		}

		// 안쪽에 열은것만 닫는다.
		RegCloseKey(hKey2);
		key_num++;

	}

	// 최종 Key닫는다.
	RegCloseKey(hKey);
    
}
void __fastcall TmainCallForm::FormShow(TObject *Sender)
{
    GetDsnList();
	EVENT->Cells[0][0] = "DateTime";
	EVENT->Cells[1][0] = "DSN";
	EVENT->Cells[2][0] = "Type";
	EVENT->Cells[3][0] = "Message";
	EVENT->Cells[4][0] = "RelationFile";

	EVENT->RowCount = 2;
}
//---------------------------------------------------------------------------
void __fastcall TmainCallForm::Button2Click(TObject *Sender)
{
	if (DSNLIST->Text.Length() == 0)
	{
		ShowMessage("Select ODBC DSN you want to Monitor.");
		return;
	}

	TlockMgrForm *f1 = new TlockMgrForm(Application);
	f1->DSN->Caption = DSNLIST->Text;
	f1->Show();	
}
//---------------------------------------------------------------------------

void __fastcall TmainCallForm::Button3Click(TObject *Sender)
{
	if (DSNLIST->Text.Length() == 0)
	{
		ShowMessage("Select ODBC DSN you want to Monitor.");
		return;
	}

	TtblForm *f1 = new TtblForm(Application);
	f1->DSN->Caption = DSNLIST->Text;
	f1->Show();
}
//---------------------------------------------------------------------------

void __fastcall TmainCallForm::Button4Click(TObject *Sender)
{
	if (DSNLIST->Text.Length() == 0)
	{
		ShowMessage("Select ODBC DSN you want to Monitor.");
		return;
	}

	TMemStat *f1 = new TMemStat(Application);
	f1->DSN->Caption = DSNLIST->Text;
	f1->Show();
}
//---------------------------------------------------------------------------

void __fastcall TmainCallForm::Button5Click(TObject *Sender)
{
	if (DSNLIST->Text.Length() == 0)
	{
		ShowMessage("Select ODBC DSN you want to Monitor.");
		return;
	}

	TmemtblMgrForm *f1 = new TmemtblMgrForm(Application);
	f1->DSN->Caption = DSNLIST->Text;
	f1->Show();
}
//---------------------------------------------------------------------------
void __fastcall TmainCallForm::Button6Click(TObject *Sender)
{
	if (DSNLIST->Text.Length() == 0)
	{
		ShowMessage("Select ODBC DSN you want to Monitor.");
		return;
	}

	TreplMgr *f1 = new TreplMgr(Application);
	f1->DSN->Caption = DSNLIST->Text;
	f1->Show();
}
//---------------------------------------------------------------------------
void __fastcall TmainCallForm::CopyToDown()
{
	 int i, start = EVENT->RowCount;

     EVENT->RowCount++;
	 for (i = start ; i > 1 ; i--)
	 {
		 EVENT->Cells[0][i] = EVENT->Cells[0][i-1];
		 EVENT->Cells[1][i] = EVENT->Cells[1][i-1];
		 EVENT->Cells[2][i] = EVENT->Cells[2][i-1];
		 EVENT->Cells[3][i] = EVENT->Cells[3][i-1];
		 EVENT->Cells[4][i] = EVENT->Cells[4][i-1];		 
	 }
	 
}
//---------------------------------------------------------------------------
void __fastcall TmainCallForm::Button7Click(TObject *Sender)
{
 	if (DSNLIST->Text.Length() == 0)
	{
		ShowMessage("Select ODBC DSN you want to Monitor.");
		return;
	}

	TSnapShotForm *f1 = new TSnapShotForm(Application);
	f1->DSN->Caption = DSNLIST->Text;
	f1->Show();
}
//---------------------------------------------------------------------------

