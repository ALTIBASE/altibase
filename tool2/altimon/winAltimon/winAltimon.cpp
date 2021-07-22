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
USEFORM("mainCall.cpp", mainCallForm);
USEFORM("sessionMgr.cpp", sessionMgrForm);
USEFORM("lockMgr.cpp", lockMgrForm);
USEFORM("sqlViewForm.cpp", SQLVIEW);
USEFORM("tablespaceForm.cpp", tblForm);
USEFORM("memsTatForm.cpp", MemStat);
USEFORM("memtblForm1.cpp", memtblMgrForm);
USEFORM("replMgrForm1.cpp", replMgr);
USEFORM("snapShot123.cpp", SnapShotForm);
USEFORM("disktblForm1.cpp", diskTblMgrForm);
//---------------------------------------------------------------------------
WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	try
	{
		Application->Initialize();
		Application->CreateForm(__classid(TmainCallForm), &mainCallForm);
		Application->CreateForm(__classid(TsessionMgrForm), &sessionMgrForm);
		Application->CreateForm(__classid(TlockMgrForm), &lockMgrForm);
		Application->CreateForm(__classid(TSQLVIEW), &SQLVIEW);
		Application->CreateForm(__classid(TtblForm), &tblForm);
		Application->CreateForm(__classid(TMemStat), &MemStat);
		Application->CreateForm(__classid(TmemtblMgrForm), &memtblMgrForm);
		Application->CreateForm(__classid(TreplMgr), &replMgr);
		Application->CreateForm(__classid(TSnapShotForm), &SnapShotForm);
		Application->CreateForm(__classid(TdiskTblMgrForm), &diskTblMgrForm);
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
