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

#ifndef sqlViewEditorH
#define sqlViewEditorH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <ComCtrls.hpp>
#include <ExtCtrls.hpp>
#include <Menus.hpp>
#include <Dialogs.hpp>
#include <Buttons.hpp>
//---------------------------------------------------------------------------
class TForm6 : public TForm
{
__published:	// IDE-managed Components
	TPanel *Panel1;
	TButton *Button1;
	TMemo *Memo1;
	TComboBox *DSNLIST;
	TLabel *Label1;
	TMainMenu *MainMenu1;
	TMenuItem *Menu1;
	TMenuItem *DSNreload1;
	TMenuItem *OpenFrom;
	TMenuItem *SaveFile1;
	TMenuItem *Exit1;
	TOpenDialog *OpenDialog1;
	TSaveDialog *SaveDialog1;
	TRichEdit *RichEdit2;
	TPopupMenu *PopupMenu1;
	TMenuItem *ClearMessage1;
	TMenuItem *SaveMessage1;
	TPageControl *PageControl1;
	TMenuItem *New1;
	TPopupMenu *PopupMenu2;
	TMenuItem *execute1;
	TMenuItem *Save1;
	TMenuItem *Save2;
	TMenuItem *Newfile1;
	TMenuItem *Copy1;
	TMenuItem *Paste1;
	TMenuItem *CloseFile1;
	TMenuItem *CloseFile2;
	void __fastcall FormShow(TObject *Sender);
	void __fastcall Button1Click(TObject *Sender);
	void __fastcall Memo1DblClick(TObject *Sender);
	void __fastcall Memo1MouseDown(TObject *Sender, TMouseButton Button,
          TShiftState Shift, int X, int Y);
	void __fastcall Memo1MouseMove(TObject *Sender, TShiftState Shift, int X,
          int Y);
	void __fastcall Exit1Click(TObject *Sender);
	void __fastcall DSNreload1Click(TObject *Sender);
	void __fastcall OpenFromClick(TObject *Sender);
	void __fastcall SaveFile1Click(TObject *Sender);
	void __fastcall ClearMessage1Click(TObject *Sender);
	void __fastcall SaveMessage1Click(TObject *Sender);
	void __fastcall New1Click(TObject *Sender);
	void __fastcall execute1Click(TObject *Sender);
	void __fastcall Newfile1Click(TObject *Sender);
	void __fastcall Save1Click(TObject *Sender);
	void __fastcall Save2Click(TObject *Sender);
	void __fastcall Copy1Click(TObject *Sender);
	void __fastcall Paste1Click(TObject *Sender);
	void __fastcall CloseFile1Click(TObject *Sender);
	void __fastcall CloseFile2Click(TObject *Sender);
private:	// User declarations
public:		// User declarations
	__fastcall TForm6(TComponent* Owner);
	void __fastcall TForm6::GetDsnList();
	void __fastcall TForm6::MyKeyDown(TObject *, WORD &, TShiftState);
};
//---------------------------------------------------------------------------
extern PACKAGE TForm6 *Form6;
//---------------------------------------------------------------------------
#endif
