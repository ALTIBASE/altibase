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
#include <stdlib.h>
#include <vcl.h>
#pragma hdrstop

#define WIN32
#include <sql.h>
#include <sqlext.h>

#include "sqlRun.h"
#include "sqlViewEditor.h"

#define SC_DRAG_RESIZEL  0xf001  // left resize 
#define SC_DRAG_RESIZER  0xf002  // right resize 
#define SC_DRAG_RESIZEU  0xf003  // upper resize 
#define SC_DRAG_RESIZEUL 0xf004  // upper-left resize 
#define SC_DRAG_RESIZEUR 0xf005  // upper-right resize 
#define SC_DRAG_RESIZED  0xf006  // down resize 
#define SC_DRAG_RESIZEDL 0xf007  // down-left resize 
#define SC_DRAG_RESIZEDR 0xf008  // down-right resize 
#define SC_DRAG_MOVE     0xf012  // move


//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TForm1 *Form1;

SQLHENV  env;
SQLHDBC  dbc;
SQLHSTMT stmt = NULL;
//---------------------------------------------------------------------------
__fastcall TForm1::TForm1(TComponent* Owner)
	: TForm(Owner)
{
}
//---------------------------------------------------------------------------
void freeDBHandle()
{
	if (stmt != NULL)
	{
		SQLFreeStmt(stmt, SQL_DROP);
	}

	SQLDisconnect(dbc);
	SQLFreeConnect(dbc);
	SQLFreeEnv(env);

}
void __fastcall TForm1::FormClose(TObject *Sender, TCloseAction &Action)
{
	Action = caFree;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
int __fastcall TForm1::dbConnect(AnsiString DSN)
{
	SQLCHAR USER[41];
    SQLCHAR PASSWD[41];

	
	// 연결을 위한 메모리 할당.
	if (SQLAllocEnv(&env) != SQL_SUCCESS)
	{
		Form6->Memo1->Lines->Add("[ErrorMessage] : SQLAllocEnv Error");
		return 0;
	}

	if (SQLAllocConnect(env, &dbc) != SQL_SUCCESS)
	{
		Form6->Memo1->Lines->Add("[ErrorMessage] : SQLAllocConnect Error");
		freeDBHandle();
		return 0;
	}

	// 진짜 연결해봅니다.
	if (SQLConnect(dbc, DSN.c_str(), SQL_NTS, "SYS", SQL_NTS, "MANAGER", SQL_NTS) != SQL_SUCCESS)
	{
		Form6->Memo1->Lines->Add("[ErrorMessage] : SQLConnect Error");
		return 0;
	}

	if (SQLAllocStmt(dbc, &stmt) != SQL_SUCCESS)
	{
    	Form6->Memo1->Lines->Add("[ErrorMessage] : SQLAllocStmt Error");
		return 0;
	}

    //Form6->Memo1->Lines->Add("[ErrorMessage] : SQLAllocStmt Error");
	return 1;
}
//---------------------------------------------------------------------------
int __fastcall TForm1::ExecNonSelect(AnsiString SQL, int Errtag)
{
	SQLCHAR errMsg[4096];
	SQLSMALLINT msgLen;
	SQLINTEGER errNo;
    char gMsg[40960], tmpMsg[4096];
    int i , k=0;

	if (SQLExecDirect(stmt, SQL.c_str(), SQL_NTS) != SQL_SUCCESS)
	{
		SQLError(env, dbc, stmt, NULL, &errNo, errMsg, 4095, &msgLen);
		sprintf(gMsg, "[ErrorMessage] : %s", errMsg);
        Form6->Memo1->Lines->Add("[SQL] : " + SQL);
		memset(tmpMsg, 0x00, sizeof(tmpMsg));
		for (i = 0; i < (int)strlen(gMsg); i++) {
			if (gMsg[i] == '\n') {
				Form6->Memo1->Lines->Add(tmpMsg);
				memset(tmpMsg, 0x00, sizeof(tmpMsg));
				k = 0;
				continue;
			}
			tmpMsg[k++] = gMsg[i];
		}
		//Form6->Memo1->Lines->Add(gMsg);
        Form6->Memo1->Lines->Add(tmpMsg);
		Form6->Memo1->Lines->Add(" ");
		return 0;
	}

	if (Errtag == 0) {
		return 1;
	}

	Form6->Memo1->Lines->Add("[SQL] : " + SQL);
	Form6->Memo1->Lines->Add("[ExecMessage] : Execute Successfully");
	Form6->Memo1->Lines->Add(" ");
	return 1;
}

//---------------------------------------------------------------------------
int __fastcall TForm1::ExecSelect(AnsiString SQL)
{

	int rc;
	int row;
	int maxLen[1024];
	SQLSMALLINT columnCount=0, nullable, dataType, scale, columnNameLength;
    int x, i;
    char columnName[255];
	void       **columnPtr;
	SQLINTEGER  *columnInd;
	unsigned long columnSize;

    SQLCHAR errMsg[4096];
	SQLSMALLINT msgLen;
	SQLINTEGER errNo;
	char gMsg[40960], tmpMsg[4096];
	int k=0;

	if (SQLExecDirect(stmt, "alter session set select_header_display = 1", SQL_NTS) != SQL_SUCCESS)
	{
		return 0;
	}

	if (SQLExecDirect(stmt, SQL.c_str(), SQL_NTS) != SQL_SUCCESS)
	{
        SQLError(env, dbc, stmt, NULL, &errNo, errMsg, 4095, &msgLen);
		sprintf(gMsg, "[ErrorMessage] : %s", errMsg);
		memset(tmpMsg, 0x00, sizeof(tmpMsg));
        k = 0;
		for (i = 0; i < (int)strlen(gMsg); i++)
		{
			if (gMsg[i] == '\n') {
				Form6->Memo1->Lines->Add(tmpMsg);
				memset(tmpMsg, 0x00, sizeof(tmpMsg));
				k = 0;
				continue;
			}
			tmpMsg[k++] = gMsg[i];
		}
        Form6->Memo1->Lines->Add(tmpMsg);
		Form6->Memo1->Lines->Add(" ");
		return 0;
	}


	SQLNumResultCols(stmt, &columnCount);
	columnPtr = (void**) malloc( sizeof(void*) * columnCount );
	columnInd = (SQLINTEGER*) malloc( sizeof(SQLINTEGER) * columnCount );
	if ( columnPtr == NULL )
	{
		free(columnInd);
		return 0;
	}

	//컬럼명 뿌리면서... Binding도 하고 메모리도 잡고..
	for ( i=0; i<columnCount; i++ )
	{
		SQLDescribeCol(stmt, i+1,
					   columnName, sizeof(columnName), &columnNameLength,
					   &dataType,
					   &columnSize,
					   &scale,
					   &nullable);

		columnPtr[i] = (char*) malloc( columnSize + 1 );
		SQLBindCol(stmt, i+1, SQL_C_CHAR, columnPtr[i], columnSize+1, &columnInd[i]);
		DataGrid->Cells[i][0] = columnName;
		maxLen[i] = strlen(columnName);
	}

	row = 1;
	DataGrid->RowCount = 2;
	DataGrid->ColCount = columnCount;

   // 데이타 fetch하면서 Grid에 셋팅한다. 문제는 gridSize 아 귀찮네..
	while (1)
	{
		 rc = SQLFetch(stmt);
		 if (rc == SQL_NO_DATA) {
			break;
		 }
		 else if (rc != SQL_SUCCESS) {
			NEXTFETCH->Enabled = false;

			SQLError(env, dbc, stmt, NULL, &errNo, errMsg, 4095, &msgLen);
			sprintf(gMsg, "[ErrorMessage] : %s", errMsg);
			k = 0;
			for (i = 0; i < (int)strlen(gMsg); i++)
			{
				if (gMsg[i] == '\n') {
					Form6->Memo1->Lines->Add(tmpMsg);
					memset(tmpMsg, 0x00, sizeof(tmpMsg));
					k = 0;
					continue;
				}
				tmpMsg[k++] = gMsg[i];
			}
            Form6->Memo1->Lines->Add(tmpMsg);
			Form6->Memo1->Lines->Add(" ");
 			return 0;
		 }

		 for ( i=0; i<columnCount; i++ )
		 {
			if ( columnInd[i] == SQL_NULL_DATA )
			{
				DataGrid->Cells[i][row] = "";
				continue;
			}
			DataGrid->Cells[i][row] = (char*)columnPtr[i];
			if (columnInd[i] > maxLen[i]) {
				maxLen[i] = columnInd[i];
				if (maxLen[i] >= 30) {
					maxLen[i] = 30;
				}
			}
		 }
		 row++;
		 if (row > 3000) {
			 break;
		 }
	}
	if (rc != SQL_SUCCESS)
	{
		NEXTFETCH->Enabled = false;
	}else {
        NEXTFETCH->Enabled = true;
   }
	
	// 사용한 메모리 해제 !!
	for ( i=0; i<columnCount; i++ )
	{
        free( columnPtr[i] );
	}
	free( columnPtr );
	free( columnInd );

	// Grid 크기 , 위치 조정하기
	if (row == 1)
	{
		DataGrid->RowCount = 2;
		for (i = 0; i < columnCount; i++)
		{
			DataGrid->Cells[i][1] = "";
		}
	}else {
		DataGrid->RowCount = row;
	}

	for (i = 0; i < columnCount; i++)
	{
		if (maxLen[i] < 10) {
            maxLen[i] = 10;
		}
		DataGrid->ColWidths[i] = maxLen[i] * 8;
	}

	return 1;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::NEXTFETCHClick(TObject *Sender)
{
	int rc;
	int row, row2;
	SQLSMALLINT columnCount=0, nullable, dataType, scale, columnNameLength;
	int x, i;
    char columnName[255];
	void       **columnPtr;
	SQLINTEGER  *columnInd;
	unsigned long columnSize;
	AnsiString _emsg;
    
    SQLCHAR errMsg[4096];
	SQLSMALLINT msgLen;
	SQLINTEGER errNo;
    char gMsg[40960];

	// 그럴일은 없지만 stmt가 NULL이라면 그냥 리턴한다.
	if (stmt == NULL)
	{
        return;    
	}

	SQLNumResultCols(stmt, &columnCount);
	columnPtr = (void**) malloc( sizeof(void*) * columnCount );
	columnInd = (SQLINTEGER*) malloc( sizeof(SQLINTEGER) * columnCount );
	if ( columnPtr == NULL )
	{
		free(columnInd);
		return  ;
	}

	//컬럼명 뿌리면서... Binding도 하고 메모리도 잡고..
	for ( i=0; i<columnCount; i++ )
	{
		SQLDescribeCol(stmt, i+1,
					   columnName, sizeof(columnName), &columnNameLength,
					   &dataType,
					   &columnSize,
					   &scale,
					   &nullable);

		columnPtr[i] = (char*) malloc( columnSize + 1 );
		SQLBindCol(stmt, i+1, SQL_C_CHAR, columnPtr[i], columnSize+1, &columnInd[i]);
	}

    row = 1;
    row2 = DataGrid->RowCount;
	// 데이타 fetch하면서 Grid에 셋팅한다. 문제는 gridSize 아 귀찮네..
	while (1)
	{
		 rc = SQLFetch(stmt);
		 if (rc == SQL_NO_DATA) {
            NEXTFETCH->Enabled = false;
			break;
		 }
		 else if (rc != SQL_SUCCESS) {
			NEXTFETCH->Enabled = false; 
			SQLError(env, dbc, stmt, NULL, &errNo, errMsg, 4095, &msgLen);
			sprintf(gMsg, "[ErrorMessage] : %s", errMsg);
			Form6->Memo1->Lines->Add(gMsg);
			Form6->Memo1->Lines->Add(" ");
			break;
		 }

		 for ( i=0; i<columnCount; i++ )
		 {
			if ( columnInd[i] == SQL_NULL_DATA )
			{
                DataGrid->Cells[i][row2] = "";
				continue;
			}
			DataGrid->Cells[i][row2] = (char*)columnPtr[i];
		 }
		 row++; row2++;
		 if (row > 3000) {
			 break;
		 }
	}
	
	
	// 사용한 메모리 해제 !!
	for ( i=0; i<columnCount; i++ )
	{
        free( columnPtr[i] );
	}
	free( columnPtr );
	free( columnInd );

	DataGrid->RowCount = row2;	
}
//---------------------------------------------------------------------------
void __fastcall TForm1::RichEdit1MouseMove(TObject *Sender, TShiftState Shift,
      int X, int Y)
{
	TControl *SenderControl = dynamic_cast<TControl *>(Sender);

	// 사이즈 조절하기.
	if ( (X < 4 && Y < 4) || (X > SenderControl->Width-4 && Y > SenderControl->Height-4))
        SenderControl->Cursor = crSizeNWSE; 
	else if((X < 4 && Y > SenderControl->Height-4) || (X > SenderControl->Width-4 && Y < 4))
        SenderControl->Cursor = crSizeNESW; 
	else if(X < 4 || X > SenderControl->Width-4)
		SenderControl->Cursor = crSizeWE;
    else if(Y < 4 || Y > SenderControl->Height-4) 
        SenderControl->Cursor = crSizeNS; 
    else 
		SenderControl->Cursor = crDefault;		
}
//---------------------------------------------------------------------------
void __fastcall TForm1::RichEdit1MouseDown(TObject *Sender, TMouseButton Button,
      TShiftState Shift, int X, int Y)
{
	TControl *SenderControl = dynamic_cast<TControl *>(Sender);
	int SysCommWparam;

	// 들어온 마우스포인터 위치에 따라
	if(X < 4 && Y < 4)
		SysCommWparam = SC_DRAG_RESIZEUL;
	else if(X > SenderControl->Width-4 && Y > SenderControl->Height-4)
		SysCommWparam = SC_DRAG_RESIZEDR;
	else if(X < 4 && Y > SenderControl->Height-4)
		SysCommWparam = SC_DRAG_RESIZEDL;
	else if(X > SenderControl->Width-4 && Y < 4)
		SysCommWparam = SC_DRAG_RESIZEUR;
	else if(X < 4)
		SysCommWparam = SC_DRAG_RESIZEL;
	else if(X > SenderControl->Width-4)
		SysCommWparam = SC_DRAG_RESIZER;
	else if(Y < 4)
		SysCommWparam = SC_DRAG_RESIZEU;
	else if(Y > SenderControl->Height-4)
		SysCommWparam = SC_DRAG_RESIZED;


	// 메시지처리
	ReleaseCapture();
	SendMessage(RichEdit1->Handle, WM_SYSCOMMAND, SysCommWparam, 0);	
}
//---------------------------------------------------------------------------

void __fastcall TForm1::Button1Click(TObject *Sender)
{
	int i, j;
	FILE *fp;

	// 저장할 파일대상 물색.
	if (!SaveDialog1->Execute()) {
		return;
	}

	// 파일 열기.
	fp = fopen(SaveDialog1->FileName.c_str(), "w+");
	if (fp == NULL) {
		ShowMessage("File Open Error [" + SaveDialog1->FileName + "]");
		return;
	}
	
    // 전체 Row를 훝어가면서 저장한다.
	for (i = 0; i < DataGrid->RowCount ; i++)
	{
		for (j = 0; j < DataGrid->ColCount ; j++)
		{
			fprintf(fp, "%s", DataGrid->Cells[j][i].c_str());

			// 콤마로 구분자로 한다.
			if ( (j+1) != DataGrid->ColCount) {
                fprintf(fp, ",");
			}
		}
		fprintf(fp, "\n");
		fflush(fp);
	}

	fclose(fp);
}
//---------------------------------------------------------------------------

