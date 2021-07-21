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
#include <dbghelp.h>
#include <shellapi.h>
#include <shlobj.h>
#include "iduWinCallstack.h"
/* ------------------------------------------------
 *  Data type define
 * ----------------------------------------------*/

#if _MSC_VER < 1300
typedef unsigned __int64 DWORD64, *PDWORD64;
#if defined(_WIN64)
typedef unsigned __int64 SIZE_T, *PSIZE_T;
#else
typedef unsigned long SIZE_T, *PSIZE_T;
#endif
#endif  // _MSC_VER < 1300


#if defined(_M_IX86)
#define GET_CURRENT_CONTEXT(c, contextFlags)    \
  do {                                          \
    memset(&c, 0, sizeof(CONTEXT));             \
    c.ContextFlags = contextFlags;              \
    __asm    call x                             \
      __asm x: pop eax                          \
      __asm    mov c.Eip, eax                   \
      __asm    mov c.Ebp, ebp                   \
      __asm    mov c.Esp, esp                   \
      } while(0);

#else

// The following is defined for x86 (XP and higher), x64 and IA64:
#define GET_CURRENT_CONTEXT(c, contextFlags)    \
  do {                                          \
    memset(&c, 0, sizeof(CONTEXT));             \
    c.ContextFlags = contextFlags;              \
    RtlCaptureContext(&c);                      \
  } while(0);
#endif


// Some missing defines (for VC5/6):
#ifndef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)
#endif  

typedef enum StackWalkOptions
  {
    // No addition info will be retrived 
    // (only the address is available)
    RetrieveNone = 0,
    
    // Try to get the symbol-name
    RetrieveSymbol = 1,
    
    // Try to get the line for this symbol
    RetrieveLine = 2,
    
    // Try to retrieve the module-infos
    RetrieveModuleInfo = 4,
    
    // Also retrieve the version for the DLL/EXE
    RetrieveFileVersion = 8,
    
    // Contains all the abouve
    RetrieveVerbose = 0xF,
    
    // Generate a "good" symbol-search-path
    SymBuildPath = 0x10,
    
    // Also use the public Microsoft-Symbol-Server
    SymUseSymSrv = 0x20,
    
    // Contains all the abouve "Sym"-options
    SymAll = 0x30,
    
    // Contains all options (default)
    OptionsAll = 0x3F
  } StackWalkOptions;


/* ------------------------------------------------
 *  Predefined Functions
 * ----------------------------------------------*/

// secure-CRT_functions are only available starting with VC8
#if _MSC_VER < 1400
#define strcpy_s strcpy
#define strcat_s(dst, len, src) strcat(dst, src)
#define _snprintf_s _snprintf
#define _tcscat_s _tcscat
#endif



/* ------------------------------------------------
 *  DBGHELP.DLL internal defines
 * ----------------------------------------------*/

// If VC7 and later, then use the shipped 'dbghelp.h'-file
#if _MSC_VER >= 1300
#include <dbghelp.h>
#else
// inline the important dbghelp.h-declarations...
typedef enum {
  SymNone = 0,
  SymCoff,
  SymCv,
  SymPdb,
  SymExport,
  SymDeferred,
  SymSym,
  SymDia,
  SymVirtual,
  NumSymTypes
} SYM_TYPE;
typedef struct _IMAGEHLP_LINE64 {
  DWORD                       SizeOfStruct;           // set to sizeof(IMAGEHLP_LINE64)
  PVOID                       Key;                    // internal
  DWORD                       LineNumber;             // line number in file
  PCHAR                       FileName;               // full filename
  DWORD64                     Address;                // first instruction of line
} IMAGEHLP_LINE64, *PIMAGEHLP_LINE64;
typedef struct _IMAGEHLP_MODULE64 {
  DWORD                       SizeOfStruct;           // set to sizeof(IMAGEHLP_MODULE64)
  DWORD64                     BaseOfImage;            // base load address of module
  DWORD                       ImageSize;              // virtual size of the loaded module
  DWORD                       TimeDateStamp;          // date/time stamp from pe header
  DWORD                       CheckSum;               // checksum from the pe header
  DWORD                       NumSyms;                // number of symbols in the symbol table
  SYM_TYPE                    SymType;                // type of symbols loaded
  CHAR                        ModuleName[32];         // module name
  CHAR                        ImageName[256];         // image name
  CHAR                        LoadedImageName[256];   // symbol file name
} IMAGEHLP_MODULE64, *PIMAGEHLP_MODULE64;
typedef struct _IMAGEHLP_SYMBOL64 {
  DWORD                       SizeOfStruct;           // set to sizeof(IMAGEHLP_SYMBOL64)
  DWORD64                     Address;                // virtual address including dll base address
  DWORD                       Size;                   // estimated size of symbol, can be zero
  DWORD                       Flags;                  // info about the symbols, see the SYMF defines
  DWORD                       MaxNameLength;          // maximum size of symbol name in 'Name'
  CHAR                        Name[1];                // symbol name (null terminated string)
} IMAGEHLP_SYMBOL64, *PIMAGEHLP_SYMBOL64;
typedef enum {
  AddrMode1616,
  AddrMode1632,
  AddrModeReal,
  AddrModeFlat
} ADDRESS_MODE;
typedef struct _tagADDRESS64 {
  DWORD64       Offset;
  WORD          Segment;
  ADDRESS_MODE  Mode;
} ADDRESS64, *LPADDRESS64;
typedef struct _KDHELP64 {
  DWORD64   Thread;
  DWORD   ThCallbackStack;
  DWORD   ThCallbackBStore;
  DWORD   NextCallback;
  DWORD   FramePointer;
  DWORD64   KiCallUserMode;
  DWORD64   KeUserCallbackDispatcher;
  DWORD64   SystemRangeStart;
  DWORD64  Reserved[8];
} KDHELP64, *PKDHELP64;
typedef struct _tagSTACKFRAME64 {
  ADDRESS64   AddrPC;               // program counter
  ADDRESS64   AddrReturn;           // return address
  ADDRESS64   AddrFrame;            // frame pointer
  ADDRESS64   AddrStack;            // stack pointer
  ADDRESS64   AddrBStore;           // backing store pointer
  PVOID       FuncTableEntry;       // pointer to pdata/fpo or NULL
  DWORD64     Params[4];            // possible arguments to the function
  BOOL        Far;                  // WOW far call
  BOOL        Virtual;              // is this a virtual frame?
  DWORD64     Reserved[3];
  KDHELP64    KdHelp;
} STACKFRAME64, *LPSTACKFRAME64;
typedef
BOOL
(__stdcall *PREAD_PROCESS_MEMORY_ROUTINE64)(
                                            HANDLE      hProcess,
                                            DWORD64     qwBaseAddress,
                                            PVOID       lpBuffer,
                                            DWORD       nSize,
                                            LPDWORD     lpNumberOfBytesRead
                                            );
typedef
PVOID
(__stdcall *PFUNCTION_TABLE_ACCESS_ROUTINE64)(
                                              HANDLE  hProcess,
                                              DWORD64 AddrBase
                                              );
typedef
DWORD64
(__stdcall *PGET_MODULE_BASE_ROUTINE64)(
                                        HANDLE  hProcess,
                                        DWORD64 Address
                                        );
typedef
DWORD64
(__stdcall *PTRANSLATE_ADDRESS_ROUTINE64)(
                                          HANDLE    hProcess,
                                          HANDLE    hThread,
                                          LPADDRESS64 lpaddr
                                          );
#define SYMOPT_CASE_INSENSITIVE         0x00000001
#define SYMOPT_UNDNAME                  0x00000002
#define SYMOPT_DEFERRED_LOADS           0x00000004
#define SYMOPT_NO_CPP                   0x00000008
#define SYMOPT_LOAD_LINES               0x00000010
#define SYMOPT_OMAP_FIND_NEAREST        0x00000020
#define SYMOPT_LOAD_ANYTHING            0x00000040
#define SYMOPT_IGNORE_CVREC             0x00000080
#define SYMOPT_NO_UNQUALIFIED_LOADS     0x00000100
#define SYMOPT_FAIL_CRITICAL_ERRORS     0x00000200
#define SYMOPT_EXACT_SYMBOLS            0x00000400
#define SYMOPT_ALLOW_ABSOLUTE_SYMBOLS   0x00000800
#define SYMOPT_IGNORE_NT_SYMPATH        0x00001000
#define SYMOPT_INCLUDE_32BIT_MODULES    0x00002000
#define SYMOPT_PUBLICS_ONLY             0x00004000
#define SYMOPT_NO_PUBLICS               0x00008000
#define SYMOPT_AUTO_PUBLICS             0x00010000
#define SYMOPT_NO_IMAGE_SEARCH          0x00020000
#define SYMOPT_SECURE                   0x00040000
#define SYMOPT_DEBUG                    0x80000000
#define UNDNAME_COMPLETE                 (0x0000)  // Enable full undecoration
#define UNDNAME_NAME_ONLY                (0x1000)  // Crack only the name for primary declaration;
#endif  // _MSC_VER < 1300

typedef struct IMAGEHLP_MODULE64_V2 {
  DWORD    SizeOfStruct;           // set to sizeof(IMAGEHLP_MODULE64)
  DWORD64  BaseOfImage;            // base load address of module
  DWORD    ImageSize;              // virtual size of the loaded module
  DWORD    TimeDateStamp;          // date/time stamp from pe header
  DWORD    CheckSum;               // checksum from the pe header
  DWORD    NumSyms;                // number of symbols in the symbol table
  SYM_TYPE SymType;                // type of symbols loaded
  CHAR     ModuleName[32];         // module name
  CHAR     ImageName[256];         // image name
  CHAR     LoadedImageName[256];   // symbol file name
};
typedef BOOL (__stdcall *tSGMI)( IN HANDLE hProcess, IN DWORD64 dwAddr, OUT IMAGEHLP_MODULE64_V2 *ModuleInfo );
typedef DWORD64 (__stdcall *tSGMB)( IN HANDLE hProcess, IN DWORD64 dwAddr );
typedef PVOID (__stdcall *tSFTA)( HANDLE hProcess, DWORD64 AddrBase );

typedef BOOL (__stdcall *tSI)( IN HANDLE hProcess, IN PSTR UserSearchPath, IN BOOL fInvadeProcess );
typedef DWORD64 (__stdcall *tSLM)( IN HANDLE hProcess, IN HANDLE hFile,
                                   IN PSTR ImageName, IN PSTR ModuleName, IN DWORD64 BaseOfDll, IN DWORD SizeOfDll );

typedef BOOL (__stdcall *tSW)( 
                              DWORD MachineType, 
                              HANDLE hProcess,
                              HANDLE hThread, 
                              LPSTACKFRAME64 StackFrame, 
                              PVOID ContextRecord,
                              PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemoryRoutine,
                              PFUNCTION_TABLE_ACCESS_ROUTINE64 FunctionTableAccessRoutine,
                              PGET_MODULE_BASE_ROUTINE64 GetModuleBaseRoutine,
                              PTRANSLATE_ADDRESS_ROUTINE64 TranslateAddress );

typedef BOOL (WINAPI* MINIDUMPWRITEDUMP)( HANDLE hProcess,
                                          DWORD dwPid,
                                          HANDLE hFile,
                                          MINIDUMP_TYPE dumpType,
                                          CONST PMINIDUMP_EXCEPTION_INFORMATION exceptionParam,
                                          CONST PMINIDUMP_USER_STREAM_INFORMATION userStreamParam,
                                          CONST PMINIDUMP_CALLBACK_INFORMATION callbackParam );

/* ------------------------------------------------
 *  Class define
 * ----------------------------------------------*/

static void gDefaultAddrCallback(void *aArg)
{
  printf("Arress=%p\n", (char *)aArg);
}

static void gDefaultErrCallback(void *aArg)
{
  printf("Error=%s\n", (char *)aArg);
}

static BOOL   m_modulesLoaded = FALSE;
static LPSTR  m_szSymPath;
static int    m_options = RetrieveNone;
static tSFTA  pSFTA;
static tSGMB  pSGMB;
static tSI    pSI;
static tSLM   pSLM;
static tSW    pSW; 
static HMODULE m_hDbhHelp;
static CRITICAL_SECTION g_CS;
static iduWin32CallStackCallback gFuncAddrCallback = gDefaultAddrCallback;
static iduWin32ErrorMsgCallback  gFuncErrMsgCallback = gDefaultErrCallback;

// **************************************** ToolHelp32 ************************
#define MAX_MODULE_NAME32 255
#define TH32CS_SNAPMODULE   0x00000008
#pragma pack( push, 8 )
typedef struct tagMODULEENTRY32
{
  DWORD   dwSize;
  DWORD   th32ModuleID;       // This module
  DWORD   th32ProcessID;      // owning process
  DWORD   GlblcntUsage;       // Global usage count on the module
  DWORD   ProccntUsage;       // Module usage count in th32ProcessID's context
  BYTE  * modBaseAddr;        // Base address of module in th32ProcessID's context
  DWORD   modBaseSize;        // Size in bytes of module starting at modBaseAddr
  HMODULE hModule;            // The hModule of this module in th32ProcessID's context
  char    szModule[MAX_MODULE_NAME32 + 1];
  char    szExePath[MAX_PATH];
} MODULEENTRY32;
typedef MODULEENTRY32 *  PMODULEENTRY32;
typedef MODULEENTRY32 *  LPMODULEENTRY32;
#pragma pack( pop )


static BOOL loadDbgDLL(LPCSTR szSymPath)
{
  // Dynamically load the Entry-Points for dbghelp.dll:
  // First try to load the newsest one from
  TCHAR szTemp[4096];
  // But before wqe do this, we first check if the ".local" file exists
  if (GetModuleFileName(NULL, szTemp, 4096) > 0)
    {
      if (GetEnvironmentVariable(_T("ProgramFiles"), szTemp, 4096) > 0)
        {
          _tcscat_s(szTemp, _T("\\Debugging Tools for Windows\\dbghelp.dll"));
          // now check if the file exists:
          if (GetFileAttributes(szTemp) != INVALID_FILE_ATTRIBUTES)
            {
              m_hDbhHelp = LoadLibrary(szTemp);
            }
        }
      // Still not found? Then try to load the 64-Bit version:
      if ( (m_hDbhHelp == NULL) && (GetEnvironmentVariable(_T("ProgramFiles"), szTemp, 4096) > 0) )
        {
          _tcscat_s(szTemp, _T("\\Debugging Tools for Windows 64-Bit\\dbghelp.dll"));
          if (GetFileAttributes(szTemp) != INVALID_FILE_ATTRIBUTES)
            {
              m_hDbhHelp = LoadLibrary(szTemp);
            }
        }
    }
  if (m_hDbhHelp == NULL)  // if not already loaded, try to load a default-one
    m_hDbhHelp = LoadLibrary( _T("dbghelp.dll") );
  if (m_hDbhHelp == NULL)
    {
      (*gFuncErrMsgCallback)("[Error] Can't Load dbghelp.dll");
      return FALSE;
    }
  pSI = (tSI) GetProcAddress(m_hDbhHelp, "SymInitialize" );
  pSW = (tSW) GetProcAddress(m_hDbhHelp, "StackWalk64" );
  pSFTA = (tSFTA) GetProcAddress(m_hDbhHelp, "SymFunctionTableAccess64" );
  pSGMB = (tSGMB) GetProcAddress(m_hDbhHelp, "SymGetModuleBase64" );
  pSLM = (tSLM) GetProcAddress(m_hDbhHelp, "SymLoadModule64" );

  if ( pSFTA == NULL || 
       pSGMB == NULL ||
       pSI   == NULL ||
       pSW   == NULL ||
       pSLM  == NULL )
    {
      (*gFuncErrMsgCallback)("[Error] Can't Load Function Pointer in dbghelp.dll");
      FreeLibrary(m_hDbhHelp);
      m_hDbhHelp = NULL;
      return FALSE;
    }

  // SymInitialize
  if (pSI(GetCurrentProcess(), (CHAR *)szSymPath, FALSE) == FALSE)
    {
      (*gFuncErrMsgCallback)("[Error] in pSI(GetCurrentProcess())");
      return FALSE;
    }

  return TRUE;
}

static DWORD LoadInfoModule(HANDLE hProcess, LPCSTR img, LPCSTR mod, DWORD64 baseAddr, DWORD size)
{
  DWORD result = ERROR_SUCCESS;
  if (pSLM(hProcess, 0, (CHAR *)img, (CHAR *)mod, baseAddr, size) == 0)
    {
      (*gFuncErrMsgCallback)("[Error] LoadInfoModule() in pSLM(hProcess, 0, (CHAR...");
      result = GetLastError();
    }
  return result;
}

static BOOL GetModuleListTH32(HANDLE hProcess, DWORD pid)
{
  // CreateToolhelp32Snapshot()
  typedef HANDLE (__stdcall *tCT32S)(DWORD dwFlags, DWORD th32ProcessID);
  // Module32First()
  typedef BOOL (__stdcall *tM32F)(HANDLE hSnapshot, LPMODULEENTRY32 lpme);
  // Module32Next()
  typedef BOOL (__stdcall *tM32N)(HANDLE hSnapshot, LPMODULEENTRY32 lpme);

  // try both dlls...
  const TCHAR *dllname[] = { _T("kernel32.dll"), _T("tlhelp32.dll") };
  HINSTANCE hToolhelp = NULL;
  tCT32S pCT32S = NULL;
  tM32F pM32F = NULL;
  tM32N pM32N = NULL;

  HANDLE hSnap;
  MODULEENTRY32 me;
  me.dwSize = sizeof(me);
  BOOL keepGoing;
  size_t i;
  TCHAR szBinName[4096];
  // But before wqe do this, we first check if the ".local" file exists
  if (GetModuleFileName(NULL, szBinName, 4096) <= 0)
    {
      (*gFuncErrMsgCallback)("[Error] in GetModuleFileName()");
      return false;
    }

  for (i = 0; i<(sizeof(dllname) / sizeof(dllname[0])); i++ )
    {
      hToolhelp = LoadLibrary( dllname[i] );
      if (hToolhelp == NULL)
        continue;
      pCT32S = (tCT32S) GetProcAddress(hToolhelp, "CreateToolhelp32Snapshot");
      pM32F = (tM32F) GetProcAddress(hToolhelp, "Module32First");
      pM32N = (tM32N) GetProcAddress(hToolhelp, "Module32Next");
      if ( (pCT32S != NULL) && (pM32F != NULL) && (pM32N != NULL) )
        break; // found the functions!
      FreeLibrary(hToolhelp);
      hToolhelp = NULL;
    }

  if (hToolhelp == NULL)
    {
      (*gFuncErrMsgCallback)("[Error] hToolhelp is NULL");
      return FALSE;
    }

  hSnap = pCT32S( TH32CS_SNAPMODULE, pid );
  if (hSnap == (HANDLE) -1)
    {
      (*gFuncErrMsgCallback)("[Error] pCT32S( TH32CS_SNAPMODULE, pid ) failed");
      return FALSE;
    }

  keepGoing = !!pM32F( hSnap, &me );
  int cnt = 0;
  while (keepGoing)
    {
      if (strcmp(me.szExePath, szBinName) == 0)
        {
          LoadInfoModule(hProcess, me.szExePath, me.szModule, (DWORD64) me.modBaseAddr, me.modBaseSize);
          (*gFuncErrMsgCallback)(me.szExePath);
        }
      cnt++;
      keepGoing = !!pM32N( hSnap, &me );
    }
  CloseHandle(hSnap);
  FreeLibrary(hToolhelp);
  if (cnt <= 0)
    return FALSE;
  return TRUE;
} 

/* ------------------------------------------------
 *  StackWalker
 * ----------------------------------------------*/

static BOOL LoadModules()
{
  if (m_modulesLoaded != FALSE)
    return TRUE;

  // Build the sym-path:
  char *szSymPath = NULL;
  if ( (m_options & SymBuildPath) != 0)
    {
      const size_t nSymPathLen = 1024 * 128;
      szSymPath = (char*) malloc(nSymPathLen);
      if (szSymPath == NULL)
        {
          SetLastError(ERROR_NOT_ENOUGH_MEMORY);
          return FALSE;
        }
      szSymPath[0] = 0;
      // Now first add the (optional) provided sympath:
      if (m_szSymPath != NULL)
        {
          strcat_s(szSymPath, nSymPathLen, m_szSymPath);
          strcat_s(szSymPath, nSymPathLen, ";");
        }

      strcat_s(szSymPath, nSymPathLen, ".;");

      const size_t nTempLen = 1024;
      char szTemp[nTempLen];
      // Now add the current directory:
      if (GetCurrentDirectoryA(nTempLen, szTemp) > 0)
        {
          szTemp[nTempLen-1] = 0;
          strcat_s(szSymPath, nSymPathLen, szTemp);
          strcat_s(szSymPath, nSymPathLen, ";");
        }

      // Now add the path for the main-module:
      if (GetModuleFileNameA(NULL, szTemp, nTempLen) > 0)
        {
          szTemp[nTempLen-1] = 0;
          for (char *p = (szTemp+strlen(szTemp)-1); p >= szTemp; --p)
            {
              // locate the rightmost path separator
              if ( (*p == '\\') || (*p == '/') || (*p == ':') )
                {
                  *p = 0;
                  break;
                }
            }  // for (search for path separator...)
          if (strlen(szTemp) > 0)
            {
              strcat_s(szSymPath, nSymPathLen, szTemp);
              strcat_s(szSymPath, nSymPathLen, ";");
            }
        }
      if (GetEnvironmentVariableA("_NT_SYMBOL_PATH", szTemp, nTempLen) > 0)
        {
          szTemp[nTempLen-1] = 0;
          strcat_s(szSymPath, nSymPathLen, szTemp);
          strcat_s(szSymPath, nSymPathLen, ";");
        }
      if (GetEnvironmentVariableA("_NT_ALTERNATE_SYMBOL_PATH", szTemp, nTempLen) > 0)
        {
          szTemp[nTempLen-1] = 0;
          strcat_s(szSymPath, nSymPathLen, szTemp);
          strcat_s(szSymPath, nSymPathLen, ";");
        }
      if (GetEnvironmentVariableA("SYSTEMROOT", szTemp, nTempLen) > 0)
        {
          szTemp[nTempLen-1] = 0;
          strcat_s(szSymPath, nSymPathLen, szTemp);
          strcat_s(szSymPath, nSymPathLen, ";");
          // also add the "system32"-directory:
          strcat_s(szTemp, nTempLen, "\\system32");
          strcat_s(szSymPath, nSymPathLen, szTemp);
          strcat_s(szSymPath, nSymPathLen, ";");
        }

      if ( (m_options & SymBuildPath) != 0)
        {
          if (GetEnvironmentVariableA("SYSTEMDRIVE", szTemp, nTempLen) > 0)
            {
              szTemp[nTempLen-1] = 0;
              strcat_s(szSymPath, nSymPathLen, "SRV*");
              strcat_s(szSymPath, nSymPathLen, szTemp);
              strcat_s(szSymPath, nSymPathLen, "\\websymbols");
              strcat_s(szSymPath, nSymPathLen, "*http://msdl.microsoft.com/download/symbols;");
            }
          else
            strcat_s(szSymPath, nSymPathLen, "SRV*c:\\websymbols*http://msdl.microsoft.com/download/symbols;");
        }
    }

  // First Init the whole stuff...
  BOOL bRet = loadDbgDLL(szSymPath);

  (*gFuncErrMsgCallback)("Path Detail=>");
  (*gFuncErrMsgCallback)(szSymPath);

  if (szSymPath != NULL) free(szSymPath); szSymPath = NULL;
  if (bRet == FALSE)
    {
      (*gFuncErrMsgCallback)("loadDbgDLL() failed.");
      SetLastError(ERROR_DLL_INIT_FAILED);
      return FALSE;
    }

  bRet = GetModuleListTH32(GetCurrentProcess(), GetCurrentProcessId());
  if (bRet != FALSE)
    {
      m_modulesLoaded = TRUE;
    }
  else
    {
      (*gFuncErrMsgCallback)("GetModuleListTH32() failed");
    }
  return bRet;
}


static BOOL ShowCallstack(const CONTEXT *context)
{
  CONTEXT c;;
  int frameNum;

  if (m_hDbhHelp == NULL)
    {
      SetLastError(ERROR_DLL_INIT_FAILED);
      return FALSE;
    }

  if (context == NULL)
    {
      GET_CURRENT_CONTEXT(c, CONTEXT_FULL);
    }
  else
    {
      c = *context;
    }

  // init STACKFRAME for first call
  STACKFRAME64 s; // in/out stackframe
  memset(&s, 0, sizeof(s));
  DWORD imageType;
#ifdef _M_IX86
  // normally, call ImageNtHeader() and use machine info from PE header
  imageType = IMAGE_FILE_MACHINE_I386;
  s.AddrPC.Offset = c.Eip;
  s.AddrPC.Mode = AddrModeFlat;
  s.AddrFrame.Offset = c.Ebp;
  s.AddrFrame.Mode = AddrModeFlat;
  s.AddrStack.Offset = c.Esp;
  s.AddrStack.Mode = AddrModeFlat;
#elif _M_X64
  imageType = IMAGE_FILE_MACHINE_AMD64;
  s.AddrPC.Offset = c.Rip;
  s.AddrPC.Mode = AddrModeFlat;
  s.AddrFrame.Offset = c.Rsp;
  s.AddrFrame.Mode = AddrModeFlat;
  s.AddrStack.Offset = c.Rsp;
  s.AddrStack.Mode = AddrModeFlat;
#elif _M_IA64
  imageType = IMAGE_FILE_MACHINE_IA64;
  s.AddrPC.Offset = c.StIIP;
  s.AddrPC.Mode = AddrModeFlat;
  s.AddrFrame.Offset = c.IntSp;
  s.AddrFrame.Mode = AddrModeFlat;
  s.AddrBStore.Offset = c.RsBSP;
  s.AddrBStore.Mode = AddrModeFlat;
  s.AddrStack.Offset = c.IntSp;
  s.AddrStack.Mode = AddrModeFlat;
#else
#error "Platform not supported!"
#endif

  for (frameNum = 0; ; ++frameNum )
    {
      if ( ! pSW(imageType, GetCurrentProcess(), GetCurrentThread(), &s, &c, NULL, pSFTA, pSGMB, NULL) )
        { 
          break;
        }

      if (s.AddrPC.Offset == s.AddrReturn.Offset || s.AddrReturn.Offset == 0) 
        {
          break;
        }

      //printf("Address=%p\n", s.AddrPC.Offset);
      (*gFuncAddrCallback)( (void *)(s.AddrPC.Offset));

    } // for ( frameNum )

  return TRUE;
}

/* ------------------------------------------------
 *   Interfaces
 * ----------------------------------------------*/

int iduInitializeWINDOWSCallStack(iduWin32CallStackCallback aFuncArg,
                                iduWin32ErrorMsgCallback  aFuncErr)
{
  InitializeCriticalSection(&g_CS);

  gFuncAddrCallback   = aFuncArg;
  gFuncErrMsgCallback = aFuncErr;

  if (m_modulesLoaded == FALSE)
    {
      LoadModules();  // ignore the result...
    }


  return 0;
}

int iduDestroyWINDOWSCallStack()
{
  return 0;
}

int iduCallStackWINDOWS(const CONTEXT *aContext)
{
  int ret;

  EnterCriticalSection(&g_CS);

  ret = ShowCallstack(aContext);
  LeaveCriticalSection(&g_CS);

  return (ret == TRUE ? 0 : 1);
}


/***********************************************************************
 * Description : Windows Minidump 기록
 *
 *   aExceptionPtr [IN] - 예외의 종류, ASSERT일 경우 NULL이 넘어온다.
 *   aDumpFile     [IN] - MiniDump기록 할 File의 Handle이다.
 *                        본 파일에서 idf 등을 만들 수 없으므로,
 *                        미리 만들어서 넘겨받는다.
 **********************************************************************/
void iduWriteMiniDump( void*         aExceptionPtr,
                       HANDLE        aDumpFile )
{
    HMODULE           sHDll = NULL;
    MINIDUMPWRITEDUMP sWriteMiniDumpPtr = NULL;
    MINIDUMP_EXCEPTION_INFORMATION sExpParam;

    sHDll = ::LoadLibrary("DbgHelp.dll");

    if( sHDll != NULL )
    {
        sWriteMiniDumpPtr = (MINIDUMPWRITEDUMP)::GetProcAddress( sHDll,
                                                                 "MiniDumpWriteDump" );

        if( sWriteMiniDumpPtr != NULL )
        {
            sExpParam.ThreadId          = GetCurrentThreadId();
            sExpParam.ExceptionPointers = (LPEXCEPTION_POINTERS) aExceptionPtr;
            sExpParam.ClientPointers    = ( aExceptionPtr != NULL ) ? TRUE : FALSE ;

            sWriteMiniDumpPtr( GetCurrentProcess(),
                               GetCurrentProcessId(),
                               (HANDLE)aDumpFile,
#ifdef DEBUG
                               MiniDumpWithFullMemory,
#else /* release */
                               MiniDumpWithDataSegs,
#endif
                               ( aExceptionPtr != NULL ) ? &sExpParam : NULL,
                               NULL,
                               NULL );
        }
    }
}
