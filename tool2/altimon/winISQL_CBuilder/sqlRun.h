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

#ifndef sqlRunH
#define sqlRunH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <ADODB.hpp>
#include <DB.hpp>
#include <DBGrids.hpp>
#include <Grids.hpp>
#include <ExtCtrls.hpp>
#include <ComCtrls.hpp>
#include <Dialogs.hpp>
//---------------------------------------------------------------------------
class TForm1 : public TForm
{
__published:	// IDE-managed Components
	TPanel *DSN;
	TStringGrid *DataGrid;
	TPanel *Panel1;
	TButton *NEXTFETCH;
	TRichEdit *RichEdit1;
	TButton *Button1;
	TSaveDialog *SaveDialog1;
	void __fastcall FormClose(TObject *Sender, TCloseAction &Action);
	void __fastcall NEXTFETCHClick(TObject *Sender);
	void __fastcall RichEdit1MouseMove(TObject *Sender, TShiftState Shift, int X,
          int Y);
	void __fastcall RichEdit1MouseDown(TObject *Sender, TMouseButton Button,
          TShiftState Shift, int X, int Y);
	void __fastcall Button1Click(TObject *Sender);
private:	// User declarations
public:		// User declarations
	__fastcall TForm1(TComponent* Owner);
	int _fastcall TForm1::getDsnInfo(AnsiString , char , char );
	int __fastcall TForm1::dbConnect(AnsiString );
	int __fastcall TForm1::ExecNonSelect(AnsiString, int );
	int __fastcall TForm1::ExecSelect(AnsiString );
};
//---------------------------------------------------------------------------
extern PACKAGE TForm1 *Form1;
//---------------------------------------------------------------------------
#endif
