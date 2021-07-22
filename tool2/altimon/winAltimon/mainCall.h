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

#ifndef mainCallFormH
#define mainCallFormH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <ExtCtrls.hpp>
#include <Grids.hpp>
//---------------------------------------------------------------------------
class TmainCallForm : public TForm
{
__published:	// IDE-managed Components
	TPanel *Panel3;
	TLabel *Label1;
	TComboBox *DSNLIST;
	TPanel *Panel1;
	TPanel *Panel2;
	TEdit *SleepTime;
	TPanel *Panel4;
	TButton *Button1;
	TButton *Button2;
	TButton *Button3;
	TButton *Button4;
	TButton *Button5;
	TButton *Button6;
	TStringGrid *EVENT;
	TButton *Button7;
	void __fastcall Button1Click(TObject *Sender);
	void __fastcall FormShow(TObject *Sender);
	void __fastcall Button2Click(TObject *Sender);
	void __fastcall Button3Click(TObject *Sender);
	void __fastcall Button4Click(TObject *Sender);
	void __fastcall Button5Click(TObject *Sender);
	void __fastcall Button6Click(TObject *Sender);
	void __fastcall Button7Click(TObject *Sender);
private:	// User declarations
public:		// User declarations
	__fastcall TmainCallForm(TComponent* Owner);
	void __fastcall TmainCallForm::GetDsnList();
	void __fastcall TmainCallForm::CopyToDown();
};
//---------------------------------------------------------------------------
extern PACKAGE TmainCallForm *mainCallForm;
//---------------------------------------------------------------------------
#endif
