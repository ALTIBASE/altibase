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

#include "winISQL_dsnManager.h"
#include "winISQL.h"
#include "mainForm1.h"
#include "Unit12.h"

#include <sql.h>
#include <sqlext.h>

//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TForm11 *Form11;

//---------------------------------------------------------------------------
__fastcall TForm11::TForm11(TComponent* Owner)
	: TForm(Owner)
{
	env  = NULL;
	dbc  = NULL;
	stmt = NULL;
}
//---------------------------------------------------------------------------
// DSN LIST를 가져오는 함수
void __fastcall TForm11::GetDsnList_winISQL(TListBox * DSNLIST)
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
//---------------------------------------------------------------------------
// Form이 열릴때 List를 가져온다.
void __fastcall TForm11::FormShow(TObject *Sender)
{
	GetDsnList_winISQL(this->DSNLIST);
}
//---------------------------------------------------------------------------
// DSNLIST 에서 DSN 을 클릭할때 처리한다. 정보를 보여줘야 하니까!!
void __fastcall TForm11::DSNLISTClick(TObject *Sender)
{
	HKEY hKey2;
	DWORD value_type, length;
	TCHAR  ByVal1[1024], sBuf[1024], sBuf2[1024];
	AnsiString x;


	// DSN alias name 셋팅
	DSN->Text = DSNLIST->Items->Strings[DSNLIST->ItemIndex] ;

	// DSN Name을 키로 하여 전체 옵션값이 무언지 찾는다.
	wsprintf(sBuf, "Software\\ODBC\\ODBC.INI\\%s", DSN->Text.c_str() );
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
					 sBuf,
					 0,
					 KEY_ALL_ACCESS,
					 &hKey2) != 0)
	{
		ShowMessage("RegOpenKeyEx-3 Fail");
		return;
	}

	// Database
	length = 1024;
	value_type = NULL;
	memset(ByVal1 , 0x00, sizeof(ByVal1));
	wsprintf(sBuf2, "DataBase");
	if (RegQueryValueEx(hKey2,
						sBuf2,
						0,
						&value_type,
						ByVal1,
						&length) == 0)
	{
        DataBase->Text = ByVal1;
	}

	// Driver
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
        DLL->Text = ByVal1;
	}

	// NLS_USE
	length = 1024;
	value_type = NULL;
	memset(ByVal1 , 0x00, sizeof(ByVal1));
	wsprintf(sBuf2, "NLS_USE");
	if (RegQueryValueEx(hKey2,
						sBuf2,
						0,
						&value_type,
						ByVal1,
						&length) == 0)
	{
		NLS->Text = ByVal1;
	}

	// PASSWORD
	length = 1024;
	value_type = NULL;
	memset(ByVal1 , 0x00, sizeof(ByVal1));
	wsprintf(sBuf2, "Password");
	if (RegQueryValueEx(hKey2,
						sBuf2,
						0,
						&value_type,
						ByVal1,
						&length) == 0)
	{
        PASSWD->Text = ByVal1;
	}

	// PORT
	length = 1024;
	value_type = NULL;
	memset(ByVal1 , 0x00, sizeof(ByVal1));
	wsprintf(sBuf2, "Port");
	if (RegQueryValueEx(hKey2,
						sBuf2,
						0,
						&value_type,
						ByVal1,
						&length) == 0)
	{
		PORT->Text = ByVal1;
	}

	// Server
	length = 1024;
	value_type = NULL;
	memset(ByVal1 , 0x00, sizeof(ByVal1));
	wsprintf(sBuf2, "Server");
	if (RegQueryValueEx(hKey2,
						sBuf2,
						0,
						&value_type,
						ByVal1,
						&length) == 0)
	{
		SERVER->Text = ByVal1;
	}

	// User
	length = 1024;
	value_type = NULL;
	memset(ByVal1 , 0x00, sizeof(ByVal1));
	wsprintf(sBuf2, "User");
	if (RegQueryValueEx(hKey2,
						sBuf2,
						0,
						&value_type,
						ByVal1,
						&length) == 0)
	{
		USER->Text = ByVal1;
	}

	RegCloseKey(hKey2);	
}
//---------------------------------------------------------------------------
// DSN 정보를 추가 / 변경 할때 처리 한다.
void __fastcall TForm11::Button1Click(TObject *Sender)
{
   HKEY sRootKey;
   HKEY sKey;
   DWORD sDisp;
   TCHAR sBuf[MAX_PATH];
   bool already_exist = false;

   DSNLIST->Items->Add(DSN->Text) ;

   if (DSN->Text.Length() == 0)
   {
	   ShowMessage("First, Input DSN NAME");
	   return;
   }
   if (SERVER->Text.Length() == 0)
   {
	   ShowMessage("First, Input Server IP");
	   return;
   }
   if (PORT->Text.Length() == 0)
   {
	   ShowMessage("First, Input Port Number");
	   return;
   }
   if (USER->Text.Length() == 0)
   {
	   ShowMessage("First, Input User Name");
	   return;
   }
   if (PASSWD->Text.Length() == 0)
   {
	   ShowMessage("First, Input Password");
	   return;
   }
   if (DataBase->Text.Length() == 0)
   {
	   ShowMessage("First, Input Database Name");
	   return;
   }
   if (DLL->Text.Length() == 0)
   {
	   ShowMessage("First, Input Dll location (FullPath)");
	   return;
   }
   sRootKey = HKEY_LOCAL_MACHINE;

   // 선택된 DSN으로 KEY를 연다.
   wsprintf( sBuf, "Software\\ODBC\\ODBC.INI\\%s", DSN->Text.c_str());
   if( 0 != RegCreateKeyEx(sRootKey,
                           sBuf,
                           0,
                           NULL,
                           REG_OPTION_NON_VOLATILE,
                           KEY_WRITE,
                           NULL,
                           &sKey,
                           &sDisp))
   {
	  ShowMessage("RegCreateKeyEx Fail!!");
	  return;
   }

   // Server
   if( 0 != RegSetValueEx(sKey, "Server", 0, REG_SZ, SERVER->Text.c_str(),
                          (DWORD) SERVER->Text.Length()*sizeof(TCHAR)))
   {
	  RegCloseKey(sKey);
	  ShowMessage("Save Fail: RegOpenKeyEx:Server");
	  return;
   }

   // Port
   if( 0 != RegSetValueEx(sKey, "Port", 0, REG_SZ, PORT->Text.c_str(),
                          (DWORD) PORT->Text.Length()*sizeof(TCHAR)))
   {
	  RegCloseKey(sKey);
	  ShowMessage("Save Fail: RegOpenKeyEx:Port");
	  return;
   }

   // User
   if( 0 != RegSetValueEx(sKey, "User", 0, REG_SZ, USER->Text.c_str(),
						  (DWORD) USER->Text.Length()*sizeof(TCHAR)))
   {
	  RegCloseKey(sKey);
	  ShowMessage("Save Fail: RegOpenKeyEx:User");
	  return;
   }

   // Password
   if( 0 != RegSetValueEx(sKey, "Password", 0, REG_SZ, PASSWD->Text.c_str(),
                          (DWORD) PASSWD->Text.Length()*sizeof(TCHAR)))
   {
	  RegCloseKey(sKey);
	  ShowMessage("Save Fail: RegOpenKeyEx:Password");
	  return;
   }

   // DataBase
   if( 0 != RegSetValueEx(sKey, "DataBase", 0, REG_SZ, DataBase->Text.c_str(),
                          (DWORD) DataBase->Text.Length()*sizeof(TCHAR)))
   {
	  RegCloseKey(sKey);
	  ShowMessage("Save Fail: RegOpenKeyEx:Database");
	  return;
   }

   // NLS_USE
   if( 0 != RegSetValueEx(sKey, "NLS_USE", 0, REG_SZ, NLS->Text.c_str(),
						  (DWORD) NLS->Text.Length()*sizeof(TCHAR)))
   {
	  RegCloseKey(sKey);
	  ShowMessage("Save Fail: RegOpenKeyEx:NLS_USE");
	  return;
   }

   // Driver
   memset( sBuf, 0x00, MAX_PATH);
   wsprintf( sBuf, "%s", DLL->Text.c_str());
   if( 0 != RegSetValueEx(sKey, "Driver", 0, REG_SZ, (LPBYTE) sBuf,
                          (DWORD) (lstrlen(sBuf)+1)*sizeof(TCHAR)))
   {
	  RegCloseKey(sKey);
	  ShowMessage("Save Fail: RegOpenKeyEx:DLL");
	  return;
   }

   RegCloseKey(sKey);

   // DATASOURCE에도 등록해야 한다.
   memset( sBuf, 0x00, MAX_PATH);
   wsprintf( sBuf, "Software\\ODBC\\ODBC.INI\\ODBC Data Sources");
   if( 0 != RegOpenKeyEx( sRootKey,
                          sBuf,
                          0,
                          KEY_WRITE,
                          &sKey ))
   {
	  ShowMessage("Save Fail: RegOpenKeyEx:ODBC LAST");
	  return;
   }

   // DLL을 등록한다.
   memset( sBuf, 0x00, MAX_PATH);
   wsprintf( sBuf, "Altibase_ODBC_cm451");
   if( 0 != RegSetValueEx(sKey, DSN->Text.c_str(), 0, REG_SZ, (LPBYTE) sBuf,
                          (DWORD) (lstrlen(sBuf)+1)*sizeof(TCHAR)))
   {
	  RegCloseKey(sKey);
	  ShowMessage("Save Fail: RegSetValueEx:ALIAS");
	  return;
   }

   RegCloseKey(sKey);

   // 처리 결과 보여주기
   ShowMessage("Save Success!!");

   // DSN LIST 갱신
   GetDsnList_winISQL(this->DSNLIST);

   // mainForm의 트리구조를 갱신해줘야 한다.
   // 아 귀찮다..
   // 죄다 돌면서 등록된 넘인지 체크한다. 좋은 방법을 몰라서리..
   TTreeNode *tNode = Form5->DBNODE->Items->GetFirstNode();
   already_exist = false;
   while (tNode)
   {
       //ShowMessage(tNode->Text + " " + DSN->Text);
	   if (tNode->Text == DSN->Text)
	   {

		   already_exist = true;
		   break;
	   }
	   tNode = tNode->GetNext();
   }
   // 없는놈이라고 판단되면 root에 등록한다.
   if (!already_exist)
   {
	   tNode = Form5->DBNODE->Items->GetFirstNode();
	   Form5->DBNODE->Items->Add(tNode, DSN->Text);
   }

   return;
}
//---------------------------------------------------------------------------
// DSN을 삭제할때 처리한다.
void __fastcall TForm11::Button2Click(TObject *Sender)
{
   HKEY sRootKey;
   HKEY sKey;
   TCHAR sBuf[MAX_PATH];

   if (DSN->Text.Length() == 0)
   {
	   ShowMessage("First, Select DSN to delete");
       return;
   }
   sRootKey = HKEY_LOCAL_MACHINE;

   // ODBC.INI에서 삭제
   wsprintf( sBuf, "Software\\ODBC\\ODBC.INI\\%s", DSN->Text.c_str());
   if( 0 != RegDeleteKey(sRootKey, sBuf))
   {
	  ShowMessage("RegDeleteKey Fail");
	  return;
   }


   // DATASOURCE 에서 삭제
   memset( sBuf, 0x00, MAX_PATH);
   wsprintf( sBuf, "Software\\ODBC\\ODBC.INI\\ODBC Data Sources");
   if( 0 != RegOpenKeyEx( sRootKey,
                          sBuf,
                          0,
                          KEY_WRITE,
                          &sKey ))
   {
	  ShowMessage("RegOpenKeyEx Fail");
	  return ;
   }

   if( 0 != RegDeleteValue(sKey, DSN->Text.c_str()))
   {
	  ShowMessage("RegDeleteValue");
	  return ;
   }

   RegCloseKey(sKey);
   ShowMessage("Delete Success!!");
   
   // Form5가 갖는 DBNODE treeNode에서 찾아서 삭제한다.
   TTreeNode *tNode = Form5->DBNODE->Items->GetFirstNode();
   while (tNode != NULL)
   {

	   if (tNode->Text == DSN->Text)
	   {
           //ShowMessage("delete");
		   tNode->Delete();
		   break;
	   }
	   tNode = tNode->GetNext();
   }

   // DSN LIST를 갱신하고 정보화면을 Clear한다.
   GetDsnList_winISQL(this->DSNLIST);
   DSN->Text      = "";
   SERVER->Text   = "";
   PORT->Text     = "";
   USER->Text     = "";
   PASSWD->Text   = "";
   DataBase->Text = "";
   DLL->Text      = "";
   NLS->Text      = "";

   return ;
}
//---------------------------------------------------------------------------
// 사용자가 필요한 부분에 대해서
// DSN등록정보를 요청하면 리턴해준다.
int _fastcall TForm11::getDsnInfo(AnsiString DSN, char *type, char *ret)
{
	HKEY hKey2;
	DWORD value_type, length;
	TCHAR  ByVal1[1024], sBuf[1024], sBuf2[1024];
	AnsiString x;


	// DSN Name을 키로 하여 전체 옵션값이 무언지 찾는다.
	wsprintf(sBuf, "Software\\ODBC\\ODBC.INI\\%s", DSN.c_str() );
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
					 sBuf,
					 0,
					 KEY_ALL_ACCESS,
					 &hKey2) != 0)
	{
		ShowMessage("RegOpenKeyEx-3 Fail");
		return 0;
	}

	// Database
	length = 1024;
	value_type = NULL;
	memset(ByVal1 , 0x00, sizeof(ByVal1));
	wsprintf(sBuf2, "%s", type);
	if (RegQueryValueEx(hKey2,
						sBuf2,
						0,
						&value_type,
						ByVal1,
						&length) == 0)
	{
		wsprintf(ret, "%s", ByVal1);
		RegCloseKey(hKey2);
		return 1;
	}

	return 0;
}
//---------------------------------------------------------------------------
// connection
int __fastcall TForm11::ISQLConnect(TObject *Sender)
{
	AnsiString DSN = DSNLIST->Items->Strings[DSNLIST->ItemIndex];
	SQLHSTMT stmt;
	char query[4096], uname[41];
	int rc;
	SQLCHAR USER[41];
	SQLCHAR PASSWD[41];

	// 연결을 위한 메모리 할당.
	if (SQLAllocEnv(&env) != SQL_SUCCESS)
	{
		ShowMessage("SQLAllocEnv Fail");
		return 0;
	}

	if (SQLAllocConnect(env, &dbc) != SQL_SUCCESS)
	{
		ShowMessage("SQLAllocConnect Fail");
		//freeDBHandle();
		return 0;
	}

	// 접속정보를 가져옵니당..
	if (!Form11->getDsnInfo(DSN, "User",     USER))
	{
		ShowMessage("Can't Get User");
		return 0;
	}
	if (!Form11->getDsnInfo(DSN, "Password", PASSWD))
	{
		ShowMessage("Can't Get Password");
		return 0;
	}

	// 진짜 연결해봅니다.
	if (SQLConnect(dbc, DSN.c_str(), SQL_NTS, USER, SQL_NTS, PASSWD, SQL_NTS) != SQL_SUCCESS)
	{
		//dbErrMsg(env, dbc, SQL_NULL_HSTMT);
		//freeDBHandle();
		return 0;
	}

	if (SQLAllocStmt(dbc, &stmt) != SQL_SUCCESS)
	{
		ShowMessage("SQLAllocStmt Fail");
		return 0;
	}

	return 1;
}

//---------------------------------------------------------------------------
// disconnect
void __fastcall TForm11::ISQLDisconnect(TObject *Sender)
{
/*	TTreeNode *tNode = DBNODE->Selected;

	if (tNode->Parent)
	{
		ShowMessage("which do you want to disconnect?, maybe it's not DSN!");
		return;
	}

	tNode->DeleteChildren();
*/
}
//---------------------------------------------------------------------------
void __fastcall TForm11::Button3Click(TObject *Sender)
{
//	Form4->Database1->AliasName = DSN->Text;
//	Form4->Query2->DatabaseName = DSN->Text;
//	Form4->DataSource1->DataSet = Form4->Query2;
//	Form4->DBGrid1->DataSource = Form4->DataSource1;
	//Form4->Query2->UpdateObject = Form4->UpdateSQL1;
	//	ISQLConnect(this);
	Form4->Show();
	//Form12->Show();
}
//---------------------------------------------------------------------------
