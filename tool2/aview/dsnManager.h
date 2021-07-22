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

#ifndef dsnManagerH
#define dsnManagerH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <ExtCtrls.hpp>
//---------------------------------------------------------------------------
class TForm1 : public TForm
{
__published:	// IDE-managed Components
	TListBox *DSNLIST;
	TPanel *Panel1;
	TPanel *Panel2;
	TPanel *Panel3;
	TPanel *Panel4;
	TPanel *Panel5;
	TPanel *Panel6;
	TPanel *Panel7;
	TComboBox *NLS;
	TEdit *PASSWD;
	TEdit *USER;
	TEdit *PORT;
	TEdit *SERVER;
	TEdit *DSN;
	TButton *Button1;
	TButton *Button2;
	TPanel *Panel10;
	TEdit *DataBase;
	TEdit *DLL;
	void __fastcall FormShow(TObject *Sender);
	void __fastcall DSNLISTClick(TObject *Sender);
	void __fastcall Button1Click(TObject *Sender);
	void __fastcall Button2Click(TObject *Sender);

private:	// User declarations
public:		// User declarations
	__fastcall TForm1(TComponent* Owner);
	void __fastcall TForm1::GetDsnList();
};
//---------------------------------------------------------------------------
extern PACKAGE TForm1 *Form1;
//---------------------------------------------------------------------------
#endif
