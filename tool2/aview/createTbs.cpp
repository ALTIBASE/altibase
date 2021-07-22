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
#define WIN32
#include <sql.h>
#include <sqlext.h>

#include "createTbs.h"
#include "mainForm1.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TForm9 *Form9;


SQLHENV env9 = NULL;
SQLHDBC dbc9 = NULL;
SQLHSTMT stmt9 = NULL;

//---------------------------------------------------------------------------
void dbErrMsg9(SQLHENV _env, SQLHDBC _dbc, SQLHSTMT _stmt)
{
    SQLCHAR errMsg[4096];
	SQLSMALLINT msgLen;
	SQLINTEGER errNo;
	char gMsg[8192];

	memset(errMsg, 0x00, sizeof(errMsg));
	SQLError(_env, _dbc, _stmt, NULL, &errNo, errMsg, 4095, &msgLen);
	sprintf(gMsg, "MainFrame] ErrNo=[%d]:ErrMsg=%s", errNo, errMsg);
	ShowMessage(gMsg);
}
//---------------------------------------------------------------------------
// DB handle Free
void freeDBHandle9()
{

	SQLDisconnect(dbc9);
	SQLFreeConnect(dbc9);
	SQLFreeEnv(env9);

	env9 = NULL;
	dbc9 = NULL;
	
	return;
}
//---------------------------------------------------------------------------
// DB Connect
int dbConnect9(AnsiString DSN)
{
	SQLCHAR USER[41];
    SQLCHAR PASSWD[41];

	if (dbc9 != NULL)
	{
		ShowMessage("Already Connected !!");
		return 0;    
	}
	// 연결을 위한 메모리 할당.
	if (SQLAllocEnv(&env9) != SQL_SUCCESS)
	{
		ShowMessage("SQLAllocEnv Fail");
		return 0;
	}

	if (SQLAllocConnect(env9, &dbc9) != SQL_SUCCESS)
	{
		ShowMessage("SQLAllocConnect Fail");
		freeDBHandle9();
		return 0;
	}

	// 접속정보를 가져옵니당..
	if (!Form5->getDsnInfo(DSN, "User",     USER))
	{
		ShowMessage("Can't Get User");
		return 0;
	}
	if (!Form5->getDsnInfo(DSN, "Password", PASSWD))
	{
		ShowMessage("Can't Get Password");
		return 0;
	}

	// 진짜 연결해봅니다.
	if (SQLConnect(dbc9, DSN.c_str(), SQL_NTS, USER, SQL_NTS, PASSWD, SQL_NTS) != SQL_SUCCESS)
	{
		dbErrMsg9(env9, dbc9, SQL_NULL_HSTMT);
		freeDBHandle9();
		return 0;
	}

	return 1;
}

//---------------------------------------------------------------------------
__fastcall TForm9::TForm9(TComponent* Owner)
	: TForm(Owner)
{
}
//---------------------------------------------------------------------------
void __fastcall TForm9::Button1Click(TObject *Sender)
{
	char query[1024];
	char ONOFF[20], temp[1024];
	int i;
	
    // input check
	if (SPACENAME->Text.Length() == 0) {
		ShowMessage("Input SpaceName");
		SPACENAME->SetFocus();
		return;
	}
	if (DATAFILE->Text.Length() == 0) {
	    ShowMessage("Input Datafile");
		DATAFILE->SetFocus();
		return;
	}
	if (atoi(INITSIZE->Text.c_str()) <= 0) {
        ShowMessage("Check initsize value");
		INITSIZE->SetFocus();
		return;
	}
	if (atoi(MAXSIZE->Text.c_str()) <= 0) {
        ShowMessage("Check maxSize value");
		MAXSIZE->SetFocus();
		return;
	}
	if (atoi(EXTENDSIZE->Text.c_str()) <= 32) {
        ShowMessage("Check extendSize value (minimum 32k)");
		EXTENDSIZE->SetFocus();
		return;
	}

	if (AUTOEXTEND->Checked == true) {
		sprintf(ONOFF, "ON");
	}else
        sprintf(ONOFF, "OFF");

	// connect db
	if (!dbConnect9(Form5->SERVERNAME->Caption)) {
        return;
	}

    // alloc statement
	if (SQLAllocStmt(dbc9, &stmt9) != SQL_SUCCESS)
	{
		ShowMessage("SQLAllocStmt Fail");
		return ;
	}

	// datafile이 여러개일수도 있고
	// autoextend on/off에 따라서는 nextsize, maxsize가 필요없기 때문에 .
	if (ListBox1->Items->Count == 0 ) {
		sprintf(query, "create tablespace %s datafile '%s' size %sM autoextend %s ",
					SPACENAME->Text.c_str(),
					DATAFILE->Text.c_str(),
					INITSIZE->Text.c_str(),
					ONOFF );

		 // autoextend 여부에 따라.
		 sprintf(temp, " NEXT %sK MAXSIZE %sM ",
					EXTENDSIZE->Text.c_str(),
					MAXSIZE->Text.c_str());
		 if (memcmp(ONOFF, "ON", 2) == 0) {
            strcat(query, temp); 
		 }
	}else {
		 sprintf(query, "create tablespace %s datafile %s ",
						SPACENAME->Text.c_str(),
						ListBox1->Items->Strings[0].c_str());
		 for (i = 1; i < ListBox1->Items->Count; i++) {
			sprintf(temp, ",  %s ", ListBox1->Items->Strings[i].c_str());
			strcat(query, temp);
		 }
	}


	if (SQLExecDirect(stmt9, query, SQL_NTS) != SQL_SUCCESS)
	{
		dbErrMsg9(env9, dbc9, stmt9);
		SQLFreeStmt(stmt9, SQL_DROP);
		freeDBHandle9();
		return ;
	}

    SQLFreeStmt(stmt9, SQL_DROP);
	freeDBHandle9();
    //ShowMessage(query);
	ShowMessage("CreateTablespace Success");
}
//---------------------------------------------------------------------------
void __fastcall TForm9::Button2Click(TObject *Sender)
{
	char temp[1024];
	char temp2[512];
	char ONOFF[20];

	if (DATAFILE->Text.Length() == 0) {
	   ShowMessage("Input Datafile Name");
	   DATAFILE->SetFocus();
	   return;
	}
	if (INITSIZE->Text.Length() == 0) {
	   ShowMessage("Input InitSize");
	   INITSIZE->SetFocus();
	   return;
	}
	if (EXTENDSIZE->Text.Length() == 0) {
	   ShowMessage("Input ExtendSize");
	   EXTENDSIZE->SetFocus();
	   return;
	}
	if (MAXSIZE->Text.Length() == 0) {
	   ShowMessage("Input MaxSize");
	   MAXSIZE->SetFocus();
	   return;
	}

	if (AUTOEXTEND->Checked == true) {
		sprintf(ONOFF, "ON");
	}else
		sprintf(ONOFF, "OFF");

	sprintf(temp, " '%s' size %sM autoextend %s ", DATAFILE->Text.c_str(), INITSIZE->Text.c_str(), ONOFF);
	if (memcmp(ONOFF, "ON" , 2) == 0) {
		sprintf(temp2, " NEXT %sK MAXSIZE %sM ", EXTENDSIZE->Text.c_str(), MAXSIZE->Text.c_str());
        strcat(temp, temp2);
	}
    ListBox1->Items->Add(temp);
}
//---------------------------------------------------------------------------

void __fastcall TForm9::ListBox1DblClick(TObject *Sender)
{
    ListBox1->Items->Delete(ListBox1->ItemIndex);	
}
//---------------------------------------------------------------------------

