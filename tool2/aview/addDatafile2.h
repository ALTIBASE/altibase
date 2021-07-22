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

#ifndef addDatafileH
#define addDatafileH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <ExtCtrls.hpp>
//---------------------------------------------------------------------------
class TForm10 : public TForm
{
__published:	// IDE-managed Components
	TLabel *Label1;
	TLabel *Label2;
	TLabel *Label3;
	TPanel *Panel1;
	TPanel *Panel2;
	TPanel *Panel3;
	TPanel *Panel4;
	TPanel *Panel5;
	TPanel *Panel6;
	TEdit *SPACENAME;
	TEdit *DATAFILE;
	TEdit *INITSIZE;
	TEdit *MAXSIZE;
	TEdit *EXTENDSIZE;
	TCheckBox *AUTOEXTEND;
	TButton *Button1;
	TButton *Button2;
	TListBox *ListBox1;
	void __fastcall Button1Click(TObject *Sender);
	void __fastcall Button2Click(TObject *Sender);
	void __fastcall ListBox1DblClick(TObject *Sender);
private:	// User declarations
public:		// User declarations
	__fastcall TForm10(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TForm10 *Form10;
//---------------------------------------------------------------------------
#endif
