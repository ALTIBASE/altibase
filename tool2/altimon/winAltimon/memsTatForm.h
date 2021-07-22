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

#ifndef memsTatFormH
#define memsTatFormH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <ADODB.hpp>
#include <DB.hpp>
#include <DBGrids.hpp>
#include <ExtCtrls.hpp>
#include <Grids.hpp>
#include <Dialogs.hpp>
//---------------------------------------------------------------------------
class TMemStat : public TForm
{
__published:	// IDE-managed Components
	TPanel *Panel1;
	TButton *Button1;
	TCheckBox *CheckBox1;
	TDBGrid *DBGrid1;
	TTimer *Timer1;
	TADOConnection *ADOConnection1;
	TADOConnection *ADOConnection2;
	TTimer *Timer2;
	TADOQuery *ADOQuery1;
	TDataSource *DataSource1;
	TPanel *DSN;
	TPanel *Panel3;
	TLabel *Label1;
	TLabel *Label2;
	TEdit *total_alloc;
	TEdit *total_used;
	TPanel *Panel2;
	TLabel *Label3;
	TEdit *Edit1;
	TButton *Button2;
	TButton *Button3;
	TCheckBox *CheckBox2;
	TButton *Button4;
	TSaveDialog *SaveDialog1;
	TButton *Button5;
	void __fastcall Button1Click(TObject *Sender);
	void __fastcall Timer2Timer(TObject *Sender);
	void __fastcall CheckBox1Click(TObject *Sender);
	void __fastcall Button2Click(TObject *Sender);
	void __fastcall Button3Click(TObject *Sender);
	void __fastcall Button4Click(TObject *Sender);
	void __fastcall FormClose(TObject *Sender, TCloseAction &Action);
	void __fastcall Button5Click(TObject *Sender);
private:	// User declarations
public:		// User declarations
	__fastcall TMemStat(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TMemStat *MemStat;
//---------------------------------------------------------------------------
#endif
