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

#include "delimeter.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TForm3 *Form3;
//---------------------------------------------------------------------------
__fastcall TForm3::TForm3(TComponent* Owner)
	: TForm(Owner)
{
}
//---------------------------------------------------------------------------
void __fastcall TForm3::FormShow(TObject *Sender)
{
	FILE *fp;
	char a[255], b[255];

	fp = fopen("delimeter.conf", "r");
	if (fp == NULL) {
		COLS->Text = "^^##";
		ROWS->Text = "##^^";
		return;
	}
	memset(a, 0x00, sizeof(a));
	memset(b, 0x00, sizeof(b));
	fscanf(fp ,"%s %s", a, b);
	fclose(fp);

	COLS->Text = a;
	ROWS->Text = b;

}
//---------------------------------------------------------------------------
void __fastcall TForm3::Button1Click(TObject *Sender)
{
	FILE *fp;

	fp = fopen("delimeter.conf", "w+");
	if (fp == NULL) {
		ShowMessage("File Create Error!");
		return;
	}

	fprintf(fp ,"%s %s", COLS->Text.c_str(), ROWS->Text.c_str());
	fclose(fp);

	ShowMessage("Setting Success!!");
	Form3->Close();
}
//---------------------------------------------------------------------------
