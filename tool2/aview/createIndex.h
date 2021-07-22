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

#ifndef createIndexH
#define createIndexH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <ExtCtrls.hpp>
#include <Grids.hpp>
#include <CheckLst.hpp>
//---------------------------------------------------------------------------
class TForm7 : public TForm
{
__published:	// IDE-managed Components
	TPanel *Panel1;
	TStringGrid *ColGrid;
	TPanel *Panel2;
	TButton *Button1;
	TButton *Button2;
	TStringGrid *IndexGrid;
	TButton *Button3;
	TEdit *IndexName;
	TLabel *Label1;
	TCheckBox *CheckBox1;
	TEdit *TARGET;
	TLabel *Label2;
	TEdit *SERVERNAME;
	TEdit *USERS;
	TCheckListBox *ColGrid2;
	TCheckBox *CheckBox2;
	void __fastcall Button1Click(TObject *Sender);
	void __fastcall Button2Click(TObject *Sender);
	void __fastcall Button3Click(TObject *Sender);
private:	// User declarations
public:		// User declarations
	__fastcall TForm7(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TForm7 *Form7;
//---------------------------------------------------------------------------
#endif
