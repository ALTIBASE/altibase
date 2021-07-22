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

#include "addUser.h"
#include "mainForm1.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TForm2 *Form2;
//---------------------------------------------------------------------------
__fastcall TForm2::TForm2(TComponent* Owner)
	: TForm(Owner)
{
}
//---------------------------------------------------------------------------
void dbErrMsg1(SQLHENV _env, SQLHDBC _dbc, SQLHSTMT _stmt)
{
    SQLCHAR errMsg[4096];
	SQLSMALLINT msgLen;
	SQLINTEGER errNo;
	char gMsg[8192];

	memset(errMsg, 0x00, sizeof(errMsg));
	SQLError(_env, _dbc, _stmt, NULL, &errNo, errMsg, 4095, &msgLen);
	wsprintf(gMsg, "MainFrame] ErrNo=[%d]:ErrMsg=%s", errNo, errMsg);
	ShowMessage(gMsg);
}

void __fastcall TForm2::Button1Click(TObject *Sender)
{
	SQLHENV env;
	SQLHDBC dbc;
	SQLHSTMT stmt;
	char USER[41];
	char PASSWD[41];
    char query[1024];

	if (UID->Text.Length() == 0)
	{
		ShowMessage("Input User ID. ");
		UID->SetFocus();
		return;
	}
    if (PWD->Text.Length() == 0)
	{
		ShowMessage("Input User Password. ");
		PWD->SetFocus();
		return;
	}


	// 연결을 위한 메모리 할당.
	if (SQLAllocEnv(&env) != SQL_SUCCESS)
	{
		ShowMessage("SQLAllocEnv Fail");
		SQLDisconnect(dbc);
		SQLFreeConnect(dbc);
		SQLFreeEnv(env);
		return;
	}

	if (SQLAllocConnect(env, &dbc) != SQL_SUCCESS)
	{
		ShowMessage("SQLAllocConnect Fail");
		SQLDisconnect(dbc);
		SQLFreeConnect(dbc);
		SQLFreeEnv(env);
		return;
	}

	// 접속정보를 가져옵니당..
	if (!Form5->getDsnInfo(Form5->SERVERNAME->Caption, "User",     USER))
	{
		ShowMessage("Can't Get User");
		return;
	}
	if (!Form5->getDsnInfo(Form5->SERVERNAME->Caption, "Password", PASSWD))
	{
		ShowMessage("Can't Get Password");
		SQLDisconnect(dbc);
		SQLFreeConnect(dbc);
		SQLFreeEnv(env);
		return;
	}

	// 진짜 연결해봅니다.
	if (SQLConnect(dbc, Form5->SERVERNAME->Caption.c_str(), SQL_NTS, USER, SQL_NTS, PASSWD, SQL_NTS) != SQL_SUCCESS)
	{
		dbErrMsg1(env, dbc, SQL_NULL_HSTMT);
		SQLDisconnect(dbc);
		SQLFreeConnect(dbc);
		SQLFreeEnv(env);
		return;
	}

    if (SQLAllocStmt(dbc, &stmt) != SQL_SUCCESS)
	{
		ShowMessage("SQLAllocStmt Fail");
		return;
	}
	sprintf(query, "create user %s identified by %s", UID->Text.UpperCase().c_str(), PWD->Text.UpperCase().c_str());
	if (SQLExecDirect(stmt, query, SQL_NTS) != SQL_SUCCESS)
	{
		dbErrMsg1(env, dbc, stmt);
		SQLFreeStmt(stmt, SQL_DROP);
		SQLDisconnect(dbc);
		SQLFreeConnect(dbc);
		SQLFreeEnv(env);
		return;
	}

	if (TBSNAME->Text.Length() > 0)
	{
		sprintf(query, "alter user %s default tablespace %s", UID->Text.UpperCase().c_str(), TBSNAME->Text.UpperCase().c_str());
		if (SQLExecDirect(stmt, query, SQL_NTS) != SQL_SUCCESS)
		{
			dbErrMsg1(env, dbc, stmt);
			SQLFreeStmt(stmt, SQL_DROP);
			SQLDisconnect(dbc);
			SQLFreeConnect(dbc);
			SQLFreeEnv(env);
			return;
		}
	}


	SQLFreeStmt(stmt, SQL_DROP);
    SQLDisconnect(dbc);
	SQLFreeConnect(dbc);
	SQLFreeEnv(env);

	ShowMessage("Create User Success!!");

	// 문제는 mainForm에 추가를 해야 하는디...
	// 그냥 뒤에 붙힌다.
	// 다시 읽어들인다.. 에공..
	if (Form5->DBNODE->Selected)
	{
		// 다시 읽어들이기라면 그냥 아래 함수로 끝..
		//Form5->Connect1Click(this);

		// 제일 앞에다 집어쳐넣기..
		TTreeNode *tNode = Form5->DBNODE->Selected;
		TTreeNode *rootNode = NULL;

		while (tNode->Parent)
			tNode = tNode->Parent;

        // 이런 애매모호성 코드가 많아진다.. 짜증나넹.		
		rootNode = tNode;
		tNode = Form5->DBNODE->Items->Insert(rootNode->getFirstChild(), UID->Text.UpperCase());
		Form5->DBNODE->Items->AddChild(tNode, "Tables");
		Form5->DBNODE->Items->AddChild(tNode, "Procedures");
		Form5->DBNODE->Items->AddChild(tNode, "Views");
		Form5->DBNODE->Items->AddChild(tNode, "Triggers");
	}
}
//---------------------------------------------------------------------------
