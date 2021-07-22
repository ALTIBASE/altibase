/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 
#include <windows.h>
#include <tchar.h>
#include <stdio.h>

#define ODBC_PATH "Software\\ODBC\\ODBCINST.INI"
#define ODBC_IDENTIFIER "ALTIBASE_ODBC_CM511"
#define ODBC_DRIVER "altiodbc.dll"
#define ALTICORE_LIB "alticore.dll"  /* bug-30807 alticore for pkg */
#define CPTIMEOUT "60"


BOOL install();
void copyFile(char* aFileName);

BOOL uninstall();


BOOL addDSN(BOOL aIsSystemDSN,
            LPTSTR aDsnName,
            LPTSTR aServer,
            LPTSTR aPort,
            LPTSTR aUser,
            LPTSTR aPassword,
            LPTSTR aDataBase,
            LPTSTR aNlsUse);

BOOL deleteDSN(BOOL aIsSystemDSN,
               LPTSTR aDsnName);

void printUsage();

int main(int argc, char* argv[])
{
    if (argc <= 1)
    {
        printUsage();
        return -1;
    }

    if(0 == strcmp(argv[1], "-i"))
    {
        install();
        copyFile(ODBC_DRIVER);
        copyFile(ALTICORE_LIB); /* bug-30807 alticore for pkg */
    }
    else if(0 == strcmp(argv[1], "-u"))
    {
        uninstall();
    }
    else
    {
        printUsage();
    }

    return 0;
}


void printUsage()
{
    printf("Usage: odbcsetup  [-i] [-u] \n");
    printf(" [-i] : install odbc driver \n");
    printf(" [-u] : uninstall odbc driver \n");
}


void copyFile(char* aFileName)
{
    TCHAR sExistFileName[MAX_PATH];
    TCHAR sNewFileName[MAX_PATH];
    memset(sExistFileName, 0x00, MAX_PATH);
    memset(sNewFileName, 0x00, MAX_PATH);

    _stprintf(sExistFileName, "%s\\Altibase\\%s", getenv("APPDATA"), aFileName);
    _stprintf(sNewFileName, "%s\\system32\\%s", getenv("SystemRoot"), aFileName);

    CopyFile(sExistFileName, sNewFileName, FALSE);
}


BOOL install()
{
    HKEY sKey;
    DWORD sDisp;
    TCHAR sBuf[MAX_PATH];


    /* open registry key */
    _stprintf( sBuf, "%s\\%s", ODBC_PATH, "ODBC Drivers");
    if( 0 != RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                           sBuf,
                           0,
                           KEY_WRITE,
                           &sKey ))
    {
        return FALSE;
    }

    /* set registry value */
    memset( sBuf, 0x00, MAX_PATH);
    _stprintf( sBuf, "Installed");
    if( 0 != RegSetValueEx(sKey, ODBC_IDENTIFIER, 0, REG_SZ, (LPBYTE) sBuf,
                           (DWORD) (_tcsclen(sBuf)+1)*sizeof(TCHAR)))
    {
        RegCloseKey(sKey);
        return FALSE;
    }

    /* close registry key */
    RegCloseKey(sKey);



    /* create registry key */
    memset( sBuf, 0x00, MAX_PATH);
    _stprintf( sBuf, "%s\\%s", ODBC_PATH, ODBC_IDENTIFIER);
    if( 0 != RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                            sBuf,
                            0,
                            NULL,
                            REG_OPTION_NON_VOLATILE,
                            KEY_WRITE,
                            NULL,
                            &sKey,
                            &sDisp))
    {
        return FALSE;
    }

    memset( sBuf, 0x00, MAX_PATH);
    _stprintf( sBuf, "%s\\system32\\%s", getenv("SystemRoot"), ODBC_DRIVER);

    /* set registry value(Driver) */
    if( 0 != RegSetValueEx(sKey, "Driver", 0, REG_SZ, (LPBYTE) sBuf,
                           (DWORD) (_tcsclen(sBuf)+1)*sizeof(TCHAR)))
    {
        RegCloseKey(sKey);
        return FALSE;
    }

    /* set registry value(Setup) */
    if( 0 != RegSetValueEx(sKey, "Setup", 0, REG_SZ, (LPBYTE) sBuf,
                           (DWORD) (_tcsclen(sBuf)+1)*sizeof(TCHAR)))
    {
        RegCloseKey(sKey);
        return FALSE;
    }

    /* set registry value(CPTimeout) */
    if( 0 != RegSetValueEx(sKey, "CPTimeout", 0, REG_SZ, (LPBYTE) CPTIMEOUT,
                           (DWORD) (_tcsclen(CPTIMEOUT)+1)*sizeof(TCHAR)))
    {
        RegCloseKey(sKey);
        return FALSE;
    }

    /* close registry key */
    RegCloseKey(sKey);

    return TRUE;
}

BOOL uninstall()
{
    HKEY sKey;
    TCHAR sBuf[MAX_PATH];


    /* delete registry key */
    _stprintf( sBuf, "%s\\%s", ODBC_PATH, ODBC_IDENTIFIER);
    if( 0 != RegDeleteKey(HKEY_LOCAL_MACHINE, sBuf))
    {
        return FALSE;
    }


    /* open registry key */
    memset( sBuf, 0x00, MAX_PATH);
    _stprintf( sBuf, "%s\\%s", ODBC_PATH, "ODBC Drivers");
    if( 0 != RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                           sBuf,
                           0,
                           KEY_WRITE,
                           &sKey ))
    {
        return FALSE;
    }

    /* delete registry value */
    if( 0 != RegDeleteValue(sKey, ODBC_IDENTIFIER))
    {
        return FALSE;
    }

    /* close registry key */
    RegCloseKey(sKey);

    return TRUE;
}




BOOL addDSN(BOOL   aIsSystemDSN,
            LPTSTR aDsnName,
            LPTSTR aServer,
            LPTSTR aPort,
            LPTSTR aUser,
            LPTSTR aPassword,
            LPTSTR aDataBase,
            LPTSTR aNlsUse)
{
    HKEY sRootKey;
    HKEY sKey;
    DWORD sDisp;
    TCHAR sBuf[MAX_PATH];

    /*System DSN */
    if( TRUE == aIsSystemDSN)
    {
        sRootKey = HKEY_LOCAL_MACHINE;
    }
    /*User DSN */
    else
    {
        sRootKey = HKEY_CURRENT_USER;
    }

    /* create registry key */
    _stprintf( sBuf, "%s\\%s", ODBC_PATH, aDsnName);
    if( 0 != RegCreateKeyEx(sRootKey,
                            sBuf,
                            0,
                            NULL,
                            REG_OPTION_NON_VOLATILE,
                            KEY_WRITE,
                            NULL,
                            &sKey,
                            &sDisp))
    {
        return FALSE;
    }

    /* set registry value(Server) */
    if( 0 != RegSetValueEx(sKey, "Server", 0, REG_SZ, (LPBYTE) aServer,
                           (DWORD) (_tcsclen(aServer)+1)*sizeof(TCHAR)))
    {
        RegCloseKey(sKey);
        return FALSE;
    }

    /* set registry value(Port) */
    if( 0 != RegSetValueEx(sKey, "Port", 0, REG_SZ, (LPBYTE) aPort,
                           (DWORD) (_tcsclen(aPort)+1)*sizeof(TCHAR)))
    {
        RegCloseKey(sKey);
        return FALSE;
    }

    /* set registry value(User) */
    if( 0 != RegSetValueEx(sKey, "User", 0, REG_SZ, (LPBYTE) aUser,
                           (DWORD) (_tcsclen(aUser)+1)*sizeof(TCHAR)))
    {
        RegCloseKey(sKey);
        return FALSE;
    }

    /* set registry value(Password) */
    if( 0 != RegSetValueEx(sKey, "Password", 0, REG_SZ, (LPBYTE) aPassword,
                           (DWORD) (_tcsclen(aPassword)+1)*sizeof(TCHAR)))
    {
        RegCloseKey(sKey);
        return FALSE;
    }

    /* set registry value(DataBase) */
    if( 0 != RegSetValueEx(sKey, "DataBase", 0, REG_SZ, (LPBYTE) aDataBase,
                           (DWORD) (_tcsclen(aDataBase)+1)*sizeof(TCHAR)))
    {
        RegCloseKey(sKey);
        return FALSE;
    }

    /* set registry value(NLS_USE) */
    if( 0 != RegSetValueEx(sKey, "NLS_USE", 0, REG_SZ, (LPBYTE) aNlsUse,
                           (DWORD) (_tcsclen(aNlsUse)+1)*sizeof(TCHAR)))
    {
        RegCloseKey(sKey);
        return FALSE;
    }

    /* set registry value(Driver) */
    memset( sBuf, 0x00, MAX_PATH);
    _stprintf( sBuf, "%s\\system32\\%s", getenv("SystemRoot"), ODBC_DRIVER);
    if( 0 != RegSetValueEx(sKey, "Driver", 0, REG_SZ, (LPBYTE) sBuf,
                           (DWORD) (_tcsclen(sBuf)+1)*sizeof(TCHAR)))
    {
        RegCloseKey(sKey);
        return FALSE;
    }

    /* close registry key */
    RegCloseKey(sKey);


    /* open registry key */
    memset( sBuf, 0x00, MAX_PATH);
    _stprintf( sBuf, "%s\\%s", ODBC_PATH, "ODBC Data Sources");
    if( 0 != RegOpenKeyEx( sRootKey,
                           sBuf,
                           0,
                           KEY_WRITE,
                           &sKey ))
    {
        return FALSE;
    }

    /* set registry value */
    memset( sBuf, 0x00, MAX_PATH);
    _stprintf( sBuf, ODBC_IDENTIFIER);
    if( 0 != RegSetValueEx(sKey, aDsnName, 0, REG_SZ, (LPBYTE) sBuf,
                           (DWORD) (_tcsclen(sBuf)+1)*sizeof(TCHAR)))
    {
        RegCloseKey(sKey);
        return FALSE;
    }

    /* close registry key */
    RegCloseKey(sKey);

    return TRUE;
}

BOOL deleteDSN(BOOL   aIsSystemDSN,
               LPTSTR aDsnName)
{
    HKEY sRootKey;
    HKEY sKey;
    TCHAR sBuf[MAX_PATH];

    /* System DSN */
    if( TRUE == aIsSystemDSN)
    {
        sRootKey = HKEY_LOCAL_MACHINE;
    }
    /* User DSN */
    else
    {
        sRootKey = HKEY_CURRENT_USER;
    }

    /* delete registry key */
    _stprintf( sBuf, "%s\\%s", ODBC_PATH, aDsnName);
    if( 0 != RegDeleteKey(sRootKey, sBuf))
    {
        return FALSE;
    }


    /* open registry key */
    memset( sBuf, 0x00, MAX_PATH);
    _stprintf( sBuf, "%s\\%s", ODBC_PATH, "ODBC Data Sources");
    if( 0 != RegOpenKeyEx( sRootKey,
                           sBuf,
                           0,
                           KEY_WRITE,
                           &sKey ))
    {
        return FALSE;
    }

    /* delete registry value */
    if( 0 != RegDeleteValue(sKey, aDsnName))
    {
        return FALSE;
    }

    /* close registry key */
    RegCloseKey(sKey);

    return TRUE;
}
