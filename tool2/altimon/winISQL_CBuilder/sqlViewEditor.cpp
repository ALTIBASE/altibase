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

#include "sqlViewEditor.h"
#include "sqlRun.h"


// 몇가지 필요한 키값을 정의한다.
#define SPACE_KEY  32
#define ENTER_KEY  13
#define PG_DOWN    34
#define PG_UP      33
#define LEFT_KEY   37
#define RIGHT_KEY  39
#define UP_KEY     38
#define DOWN_KEY   40
#define HOME_KEY   36
#define END_KEY    35
#define CTRL_KEY   17
#define BACKSPACE  8
#define DEL_KEY    46


// 에러메시지 박스의 컨트롤 크기조절을 위한 Message정의
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
TForm6 *Form6;

int X = 1, Y = 1;


// 예약어 등록을 위한 단어 등록. 별로 뾰족한 방법을 모르겠다.
int WORD_COUNT = 59;
char *WORD_CHK[] = {"CREATE",     "TABLE",      "CHAR",        "INTEGER",     "NUMERIC",
					"BLOB",       "CLOB",       "DOUBLE",      "VARCHAR",     "VARCHAR2",
					"DATE",       "PRIMARY",    "INDEX",       "ALTER",       "DROP",
					"AUTOEXTEND", "KEY",        ";",           "TABLESPACE",  "ADD",
					"DATAFILE",   "EXTENT",     "NEXT",        "INIT",        "MAX",
					"SIZE",       "SELECT",     "FROM",        "WHERE",       "INSERT",
					"DELETE",     "UPDATE",     "SET",         "INTO",        "VALUES",
					"SET",        "OUTER",      "JOIN",        "LEFT" ,       "LIMIT",
					"ORDER BY",   "GROUP BY",   "PROCEDURE",   "VIEW",        "DROP",
					"TRIGGER",    "REPLACE",    "RENAME",      "BEGIN",       "END",
					"CONSTRAINT", "SMALLINT",   "DEFAULT",     "ASC",         "DESC",
					"FUNCTION",   "RETURN",     "REFERENCING", "FOR",        
					""   
				   };

//---------------------------------------------------------------------------
__fastcall TForm6::TForm6(TComponent* Owner)
	: TForm(Owner)
{
}
//---------------------------------------------------------------------------
// 프로시져, 트리거, 함수인지를 체크한다.
int findProcedure(AnsiString SQL)
{
	int pos1, pos2, pos3, pos4;

	// 규칙 : create문이 있으면서 아래 3개단어중에 하나가 껴있으면..
	pos1 = SQL.Trim().UpperCase().AnsiPos("CREATE");
	pos2 = SQL.Trim().UpperCase().AnsiPos("PROCEDURE");
    pos3 = SQL.Trim().UpperCase().AnsiPos("FUNCTION");
    pos4 = SQL.Trim().UpperCase().AnsiPos("TRIGGER");

	if (pos1 && ( pos2 || pos3 || pos4 ) )
	{
		return 1;
	}

    return 0;

}
//---------------------------------------------------------------------------
// ODBC.INI에서 DSN list를 가져온다.
void __fastcall TForm6::GetDsnList()
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
// 폼이 열릴때 일단,
// ODBC정보를 가져오고, 새로운 TabSheet 및 RichEdit를 추가한다.
void __fastcall TForm6::FormShow(TObject *Sender)
{

	// 시작시 DSN정보들을 가져온다.
	GetDsnList();
	DSNLIST->ItemIndex = 0;

	// RichEdit의 편집크기가 매우 작아 10M로 증대시킨다.
    New1Click(this);

}
//---------------------------------------------------------------------------
// 실행버튼을 눌렀을 때의 처리
void __fastcall TForm6::Button1Click(TObject *Sender)
{
	AnsiString tmp;
	int RowCount;
	int i, j, exec_count=0, stops=0;
	int chkProc;

	// 지금 눌린게 어떤 richEdit인지를 알아내야만 한다.
	// Active한 page는 한순간에 하나니까 그걸 기준으로 알아낸다.
	TRichEdit *SenderControl = dynamic_cast<TRichEdit *>(PageControl1->ActivePage->FindChildControl("Rich_" + PageControl1->ActivePage->Name)) ;


	// 선택된 접속정보가 없으면 에러처리
	if (DSNLIST->Text.Length() == 0)
	{
		ShowMessage("Select ODBC DSN to Execute !!");
		return;    
	}

	// 모든 SQL문을 숨겨진 RichEdit2에서 복사해서 이를 이용한다.
	RichEdit2->Clear();
	if (SenderControl->SelText.Length() > 0)
	{
		RichEdit2->Text = SenderControl->SelText;
	}else{
        RichEdit2->Text = SenderControl->Text;
	}

	// 오직 세미콜론만이 구분자 역할을 한다.
	tmp = "";
	RowCount = RichEdit2->Lines->Count;
	for (i = 0; i < RowCount; i++)
	{
        // 주석처리나 읽은 문자열의 길이=0 이면 무시한다.
		if (RichEdit2->Lines->Strings[i].Trim().SubString(1, 2) == "--" || RichEdit2->Lines->Strings[i].Trim().Length() == 0 ||
			RichEdit2->Lines->Strings[i].Trim().SubString(1, 1) == "\n")
		{
			continue;
		}

		// SQL문을 만들어간다.
		tmp = tmp + RichEdit2->Lines->Strings[i] + " ";

		// 세미콜론이면 실행수행한다.
		if (tmp.AnsiPos(";") != 0)
		{
			// 만일 프로시져로 판단이 되면  쭈욱 읽어들인다 "/" 나올때까지.
			chkProc = findProcedure(tmp);
			if (chkProc == 1)
			{
				for (j = i+1; j < RowCount; j++) {
					tmp = tmp + RichEdit2->Lines->Strings[j] + " ";
					if (tmp.AnsiPos("/") != 0) {
						break ;
					}
				}
                i = j ;
                tmp.Delete(tmp.AnsiPos("/"), 1);
			}


			// 새로운 폼을 만들어 Execute창에서 수행한다.
			// 향후에는 반드시 성능을 위해 쓰레드로 해야 한다.
			TForm1 *f1 = new TForm1(Application);
            f1->RichEdit1->Clear();
			for (j=stops; j <=i; j++)
			{
				if (RichEdit2->Lines->Strings[j].Trim().Length() == 0 ||
					RichEdit2->Lines->Strings[j].Trim().SubString(1, 1) == "\n")
				{
					continue;
				}
				f1->RichEdit1->Lines->Add(RichEdit2->Lines->Strings[j]);
			}

            // 새로운 폼의 DSN정보를 넘겨준다. 나중에 써먹을일이 있을꺼 같다.
			f1->DSN->Caption = DSNLIST->Text;
			if (!f1->dbConnect(f1->DSN->Caption))
				return;
     
			// 일단 select를 위한 헤더제거
			f1->ExecNonSelect("alter session set select_header_display = 1", 0);
			if (tmp.TrimLeft().UpperCase().SubString(1, 6) == "SELECT") {
				if (f1->ExecSelect(tmp))
					f1->Show();
				else
                    f1->Close();
			}else {
				f1->ExecNonSelect(tmp, 1);
				f1->Close();
			}

			stops = i+1;
            tmp = "";
			exec_count++;
		}

	}

	// 수행한건수가 없으면 세미콜론이 없을 가능성이 100%임으로..
	if (exec_count == 0)
	{
		ShowMessage("Statement need a charater ';' ");
		return;
	}
}
//---------------------------------------------------------------------------
void __fastcall TForm6::Memo1DblClick(TObject *Sender)
{
    Memo1->Clear();	
}
//---------------------------------------------------------------------------
// 메모박스에서 마우스 움직임에 따라 사이즈 변경.
void __fastcall TForm6::Memo1MouseDown(TObject *Sender, TMouseButton Button,
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
	SendMessage(Memo1->Handle, WM_SYSCOMMAND, SysCommWparam, 0);
	
	
}
//---------------------------------------------------------------------------
// 메모 박스에서 마우스 움직임에 따라 포인터 모양새 변경.
void __fastcall TForm6::Memo1MouseMove(TObject *Sender, TShiftState Shift,
      int X, int Y)
{

	TControl *SenderControl = dynamic_cast<TControl *>(Sender);

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
// 하이라이팅을 위해서 RichEdit에서 글자가 들어오면 처리한다.
void __fastcall TForm6::MyKeyDown(TObject *Sender, WORD &Key, TShiftState Shift)
{
	TRichEdit *SenderControl = dynamic_cast<TRichEdit *>(Sender);
    int k, len;
	int x, y;
	AnsiString tmp, tmp2;


	// 현재의 커서위치를 저장해 놓는다.
	TPoint oldPos   = SenderControl->CaretPos;

	// Default색깔은 검정색이다.
	TColor oldColor = clBlack;

	// 현재의 위치에서 x, y를 판단한다.
	x = SenderControl->CaretPos.x;
	y = SenderControl->CaretPos.y;


    // 특수키이면 무시한다.
	if (Key == DEL_KEY || Key == BACKSPACE || Key == CTRL_KEY
		|| Shift.Contains(ssCtrl)
		|| SenderControl->SelText.Length() > 0)
	{
		return;
	}


	// 등록된 단어리스트에서
	for (k = 0; k < WORD_COUNT ; k++)
	{
		// 등록된 단어의 길이
		len = strlen(WORD_CHK[k]);

		// 현재 짤린 위치가 0보다 작으면.
		if ( (x-len+1) < 0)
		{
			 continue;
		}

		// 단어 캡쳐
		tmp  = SenderControl->Lines->Strings[y].SubString( (x-len+1), len).UpperCase().Trim();
		if (Key != SPACE_KEY && Key != ENTER_KEY)
		{
			tmp = tmp + Key;
		}

		// 가장 왼쪽의 앞 한글자.
		tmp2 = SenderControl->Lines->Strings[y].SubString( (x-len), 1).UpperCase();

		// 길이가 0보다 큰데.. 왼쪽끝이 Space가 아니라면
		if ( (x-len) > 0 && tmp2 != " ")
		{
			continue;
		}
		
		// 맞는 등록된 단어라면 하이라트이처리한다.
		if (memcmp( tmp.c_str(), WORD_CHK[k], strlen(WORD_CHK[k]) ) == 0)
		{
			// 위치설정.
			SenderControl->CaretPos = TPoint((x-len), y);
			SenderControl->SelLength = strlen(WORD_CHK[k]);
			// 속성변경.
			SenderControl->SelAttributes->Color = clRed;
			if (tmp == ";")
			{
				SenderControl->SelAttributes->Style.Contains(fsBold);
			}
			break;
		}
	}

    // 원래대로 포지션을 찾아간다.
	SenderControl->CaretPos = oldPos;
	SenderControl->SelAttributes->Color = oldColor;

}

//---------------------------------------------------------------------------
void __fastcall TForm6::Exit1Click(TObject *Sender)
{
    Form6->Close();	
}
//---------------------------------------------------------------------------
// DSN 다시 읽기를 누르면 
void __fastcall TForm6::DSNreload1Click(TObject *Sender)
{
	DSNLIST->Clear();
	GetDsnList();
    DSNLIST->ItemIndex = 0;
}
//---------------------------------------------------------------------------
// 파일에서 읽어들일때.
void __fastcall TForm6::OpenFromClick(TObject *Sender)
{
	int i, j, k, start  ;
	AnsiString tmp, tmp2;
	char check21[1024];


    if (!OpenDialog1->Execute())
	{
		return;
	}


	// 파일에서 읽었으니 새로운 TabSheet, RichEdit를 만들어낸다.
	New1Click(this);
	TRichEdit *SenderControl = dynamic_cast<TRichEdit *>(PageControl1->ActivePage->FindChildControl("Rich_" + PageControl1->ActivePage->Name)) ;

	// 하이라이팅을 위해서는 숨겨진창에서 분석한후에
    // 이를 원본에 Copy한다.
	RichEdit2->Clear();
	RichEdit2->Lines->LoadFromFile(OpenDialog1->FileName);

	PageControl1->ActivePage->Caption = OpenDialog1->FileName;
	
	// 하이라이팅
	for (k=0; k < RichEdit2->Lines->Count; k++)
	{
		tmp = RichEdit2->Lines->Strings[k];
		start = 1;
		for (j = 1; j <= tmp.Length(); j++)
		{

			if (tmp.SubString(j , 1) == " " || tmp.SubString(j, 1) == "\n" || tmp.SubString(j, 1) == ";")
			{
				tmp2 = tmp.SubString(start, j - start).Trim().UpperCase();
				if (tmp2.Length() == 0)
				{
					start = j + 1;
					continue;
				}

				for (i = 0; i < WORD_COUNT; i++)
				{
					// 맞는 등록된 단어라면 하이라트 처리한다.
					if (memcmp( tmp2.c_str() , WORD_CHK[i], tmp2.Length() ) == 0)
					{
						// 위치설정.
						RichEdit2->CaretPos = TPoint((start-1), k);
						RichEdit2->SelLength = strlen(WORD_CHK[i]);
						// 속성변경.
						RichEdit2->SelAttributes->Color = clRed;
						break;
					}
				}
				start = j + 1;
				continue;
			}
		}
	}


	// 원래대로 포지션을 찾아간다.
	RichEdit2->CaretPos = TPoint(0, 0);
	RichEdit2->SelAttributes->Color = clBlack;


	//RichEdit2->PlainText = true;
	RichEdit2->Lines->SaveToFile(OpenDialog1->FileName);
	SenderControl->Lines->LoadFromFile(OpenDialog1->FileName);

}
//---------------------------------------------------------------------------
// 파일저장하기
void __fastcall TForm6::SaveFile1Click(TObject *Sender)
{
	TRichEdit *SenderControl = dynamic_cast<TRichEdit *>(PageControl1->ActivePage->FindChildControl("Rich_" + PageControl1->ActivePage->Name)) ;

	// untitle이 아니면 기존 파일명에 저장을 하고..
	if (PageControl1->ActivePage->Caption != "untitle")
	{
		SenderControl->PlainText = true;
		SenderControl->Lines->SaveToFile(PageControl1->ActivePage->Caption);
		SenderControl->PlainText = false;
		return;
	}

	// 새로운거면 새로운 파일명에..
	if (!SaveDialog1->Execute())
	{
		return;
	}

    // 하이라이팅 정보때문에 반드시 PlainText속성을 변경해야 한다.
	PageControl1->ActivePage->Caption = SaveDialog1->FileName;
	SenderControl->PlainText = true;
	SenderControl->Lines->SaveToFile(SaveDialog1->FileName);
	SenderControl->PlainText = false;
}
//---------------------------------------------------------------------------
void __fastcall TForm6::ClearMessage1Click(TObject *Sender)
{
    Memo1->Clear();	
}
//---------------------------------------------------------------------------

void __fastcall TForm6::SaveMessage1Click(TObject *Sender)
{
	if (!SaveDialog1->Execute()) {
		return;
	}

	Memo1->Lines->SaveToFile(SaveDialog1->FileName);
}
//---------------------------------------------------------------------------
// 핵심인데.. 메모리 누수가 없는지 걱정이다. 
void __fastcall TForm6::New1Click(TObject *Sender)
{
	static int row = 1;
	char Names123[100];
	
	RichEdit2->Clear();

	// RichEdit를 만들어낸다.
	TRichEdit *rch = new TRichEdit(this);
	rch->Align = alClient;
	rch->BevelInner = bvLowered;
	rch->BevelKind  = bkTile;
	rch->BevelOuter = bvLowered;
	rch->ScrollBars = ssBoth;
	rch->HideScrollBars = false;
	rch->Ctl3D = false;
    rch->PopupMenu = PopupMenu2;
	rch->OnKeyDown = MyKeyDown;
    rch->Text = "";
	
	// 새로운 TabSheet를 만들어낸다.
	TTabSheet *tab = new TTabSheet(this);
	tab->Parent = Form6;
	tab->PageControl = PageControl1 ;
	tab->InsertControl(rch);
	tab->Caption = "untitle";

	// FindComponent를 쉽게 하기 위해서 이름을 일관성있게 만든다.
	sprintf(Names123, "tab_%d", row);
	tab->Name = Names123;

    sprintf(Names123, "Rich_tab_%d", row);
	rch->Name = Names123;
	rch->Clear();

	// 부모들을 지정해준다.
	rch->Parent = tab;

	// 포커싱 처리
	PageControl1->ActivePage = tab;
	rch->SetFocus();

    // RichEdit의 reading제약으로 10M로 늘린다.
	SendMessage(rch->Handle, EM_EXLIMITTEXT, 0, (1024*1024*10));

	row++;
	//TabSheet1->Caption = "untitle";
	//RichEdit1->SetFocus();
}
//---------------------------------------------------------------------------

void __fastcall TForm6::execute1Click(TObject *Sender)
{
    Button1Click(this);	
}
//---------------------------------------------------------------------------

void __fastcall TForm6::Newfile1Click(TObject *Sender)
{
	New1Click(this);
}
//---------------------------------------------------------------------------

void __fastcall TForm6::Save1Click(TObject *Sender)
{
    OpenFromClick(this);	
}
//---------------------------------------------------------------------------

void __fastcall TForm6::Save2Click(TObject *Sender)
{
	SaveFile1Click(this);	
}
//---------------------------------------------------------------------------
// 복사하기
void __fastcall TForm6::Copy1Click(TObject *Sender)
{
	TRichEdit *SenderControl = dynamic_cast<TRichEdit *>(PageControl1->ActivePage->FindChildControl("Rich_" + PageControl1->ActivePage->Name)) ;
	SenderControl->CopyToClipboard();
}
//---------------------------------------------------------------------------
// 붙혀넣기
void __fastcall TForm6::Paste1Click(TObject *Sender)
{
	TRichEdit *SenderControl = dynamic_cast<TRichEdit *>(PageControl1->ActivePage->FindChildControl("Rich_" + PageControl1->ActivePage->Name)) ;
	SenderControl->PasteFromClipboard();
}
//---------------------------------------------------------------------------
// 열린 instance 닫기.
void __fastcall TForm6::CloseFile1Click(TObject *Sender)
{

    // 페이지 갯수가 0개이면 아래 코드는 에러가 날테니 막는다.
	if (PageControl1->PageCount == 0)
	{
        return;    
	}

    // 현재 포커싱된 놈만 삭제한다.
	TRichEdit *SenderControl = dynamic_cast<TRichEdit *>(PageControl1->ActivePage->FindChildControl("Rich_" + PageControl1->ActivePage->Name)) ;

	SenderControl->Free() ;
    PageControl1->ActivePage->Free();

}
//---------------------------------------------------------------------------
void __fastcall TForm6::CloseFile2Click(TObject *Sender)
{
	CloseFile1Click(this);
}
//---------------------------------------------------------------------------

