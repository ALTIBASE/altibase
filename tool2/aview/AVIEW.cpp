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

#include <vcl.h>
#pragma hdrstop
//---------------------------------------------------------------------------
USEFORM("mainForm1.cpp", Form5);
USEFORM("dsnManager.cpp", Form1);
USEFORM("addUser.cpp", Form2);
USEFORM("delimeter.cpp", Form3);
USEFORM("resource.cpp", Form6);
USEFORM("createIndex.cpp", Form7);
USEFORM("iloaderStatus.cpp", Form8);
USEFORM("addDatafile2.cpp", Form10);
USEFORM("createTbs.cpp", Form9);
USEFORM("winISQL.cpp", Form4);
USEFORM("winISQL_dsnManager.cpp", Form11);
//---------------------------------------------------------------------------
WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	try
	{
		Application->Initialize();
		Application->CreateForm(__classid(TForm5), &Form5);
		Application->CreateForm(__classid(TForm1), &Form1);
		Application->CreateForm(__classid(TForm2), &Form2);
		Application->CreateForm(__classid(TForm3), &Form3);
		Application->CreateForm(__classid(TForm6), &Form6);
		Application->CreateForm(__classid(TForm7), &Form7);
		Application->CreateForm(__classid(TForm8), &Form8);
		Application->CreateForm(__classid(TForm10), &Form10);
		Application->CreateForm(__classid(TForm9), &Form9);
		Application->CreateForm(__classid(TForm4), &Form4);
		Application->CreateForm(__classid(TForm11), &Form11);
		Application->Run();
	}
	catch (Exception &exception)
	{
		Application->ShowException(&exception);
	}
	catch (...)
	{
		try
		{
			throw Exception("");
		}
		catch (Exception &exception)
		{
			Application->ShowException(&exception);
		}
	}
	return 0;
}
//---------------------------------------------------------------------------
