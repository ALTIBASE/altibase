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

#ifndef snapShot123H
#define snapShot123H
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
//---------------------------------------------------------------------------
class TSnapShotForm : public TForm
{
__published:	// IDE-managed Components
	TTimer *Timer1;
	TADOQuery *ADOQuery1;
	TDataSource *DataSource1;
	TADOConnection *ADOConnection1;
	TPanel *DSN;
	TGroupBox *GroupBox1;
	TPanel *Panel1;
	TLabel *Label1;
	TLabel *Label2;
	TLabel *Label3;
	TEdit *SSCOUNT;
	TEdit *WCOUNT;
	TEdit *LCOUNT;
	TPanel *Panel2;
	TCheckBox *CheckBox1;
	TButton *Button1;
	TDBGrid *DBGrid1;
	TGroupBox *GroupBox2;
	TPanel *Panel3;
	TLabel *Label4;
	TLabel *Label5;
	TEdit *TOTMEM;
	TEdit *TOTUSE;
	TGroupBox *GroupBox3;
	TPanel *Panel4;
	TLabel *Label6;
	TLabel *Label7;
	TEdit *MEMALLOC;
	TEdit *MEMFREE;
	TLabel *Label8;
	TEdit *INDEXUSE;
	TGroupBox *GroupBox4;
	TPanel *Panel5;
	TLabel *Label9;
	TLabel *Label10;
	TEdit *OLDLOG;
	TEdit *CURLOG;
	TGroupBox *GroupBox5;
	TPanel *Panel6;
	TLabel *Label11;
	TComboBox *ComboBox1;
	TButton *Button2;
	TPanel *Panel7;
	void __fastcall Button1Click(TObject *Sender);
	void __fastcall CheckBox1Click(TObject *Sender);
	void __fastcall Timer1Timer(TObject *Sender);
	void __fastcall FormClose(TObject *Sender, TCloseAction &Action);
	void __fastcall GroupBox1DblClick(TObject *Sender);
	void __fastcall GroupBox2DblClick(TObject *Sender);
	void __fastcall GroupBox3DblClick(TObject *Sender);
	void __fastcall GroupBox5DblClick(TObject *Sender);
	void __fastcall Button2Click(TObject *Sender);
private:	// User declarations
public:		// User declarations
	__fastcall TSnapShotForm(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TSnapShotForm *SnapShotForm;
//---------------------------------------------------------------------------
#endif
