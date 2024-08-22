/******************************************************************************
Module:  InjLib.cpp
Notices: Copyright (c) 2008 Jeffrey Richter & Christophe Nasarre
******************************************************************************/


#include "CmnHdr.h"     /* See Appendix A. */
#include <windowsx.h>
#include <stdio.h>
#include <tchar.h>
#include <malloc.h>        // For alloca
#include <TlHelp32.h>
#include "Resource.h"
#include <StrSafe.h>



///////////////////////////////////////////////////////////////////////////////


#ifdef UNICODE
   #define InjectLib InjectLibW
   #define EjectLib  EjectLibW
#else
   #define InjectLib InjectLibA
   #define EjectLib  EjectLibA
#endif   // !UNICODE


///////////////////////////////////////////////////////////////////////////////


BOOL WINAPI InjectLibW(DWORD dwProcessId, PCWSTR pszLibFile) {

   BOOL bOk = FALSE; // Assume that the function fails
   HANDLE hProcess = NULL, hThread = NULL;
   PWSTR pszLibFileRemote = NULL;

   __try {
      // Get a handle for the target process.
      hProcess = OpenProcess(
         PROCESS_QUERY_INFORMATION |   // Required by Alpha
         PROCESS_CREATE_THREAD     |   // For CreateRemoteThread
         PROCESS_VM_OPERATION      |   // For VirtualAllocEx/VirtualFreeEx
         PROCESS_VM_WRITE,             // For WriteProcessMemory
         FALSE, dwProcessId);
      if (hProcess == NULL) __leave;

      // Calculate the number of bytes needed for the DLL's pathname
      int cch = 1 + lstrlenW(pszLibFile);
      int cb  = cch * sizeof(wchar_t);

      // Allocate space in the remote process for the pathname
      pszLibFileRemote = (PWSTR) 
         VirtualAllocEx(hProcess, NULL, cb, MEM_COMMIT, PAGE_READWRITE);
      if (pszLibFileRemote == NULL) __leave;

      // Copy the DLL's pathname to the remote process' address space
      if (!WriteProcessMemory(hProcess, pszLibFileRemote, 
         (PVOID) pszLibFile, cb, NULL)) __leave;

      // Get the real address of LoadLibraryW in Kernel32.dll
      PTHREAD_START_ROUTINE pfnThreadRtn = (PTHREAD_START_ROUTINE)
         GetProcAddress(GetModuleHandle(TEXT("Kernel32")), "LoadLibraryW");
      if (pfnThreadRtn == NULL) __leave;

      // Create a remote thread that calls LoadLibraryW(DLLPathname)
      hThread = CreateRemoteThread(hProcess, NULL, 0, 
         pfnThreadRtn, pszLibFileRemote, 0, NULL);
      if (hThread == NULL) __leave;

      // Wait for the remote thread to terminate
      WaitForSingleObject(hThread, INFINITE);

      bOk = TRUE; // Everything executed successfully
   }
   __finally { // Now, we can clean everything up

      // Free the remote memory that contained the DLL's pathname
      if (pszLibFileRemote != NULL) 
         VirtualFreeEx(hProcess, pszLibFileRemote, 0, MEM_RELEASE);

      if (hThread  != NULL) 
         CloseHandle(hThread);

      if (hProcess != NULL) 
         CloseHandle(hProcess);
   }

   return(bOk);
}


///////////////////////////////////////////////////////////////////////////////


BOOL WINAPI InjectLibA(DWORD dwProcessId, PCSTR pszLibFile) {

   // Allocate a (stack) buffer for the Unicode version of the pathname
   SIZE_T cchSize = lstrlenA(pszLibFile) + 1;
   PWSTR pszLibFileW = (PWSTR) 
      _alloca(cchSize * sizeof(wchar_t));

   // Convert the ANSI pathname to its Unicode equivalent
   StringCchPrintfW(pszLibFileW, cchSize, L"%S", pszLibFile);

   // Call the Unicode version of the function to actually do the work.
   return(InjectLibW(dwProcessId, pszLibFileW));
}


///////////////////////////////////////////////////////////////////////////////


BOOL WINAPI EjectLibW(DWORD dwProcessId, PCWSTR pszLibFile) {

   BOOL bOk = FALSE; // Assume that the function fails
   HANDLE hthSnapshot = NULL;
   HANDLE hProcess = NULL, hThread = NULL;

   __try {
      // Grab a new snapshot of the process
      hthSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, dwProcessId);
      if (hthSnapshot == INVALID_HANDLE_VALUE) __leave;

      // Get the HMODULE of the desired library
      MODULEENTRY32W me = { sizeof(me) };
      BOOL bFound = FALSE;
      BOOL bMoreMods = Module32FirstW(hthSnapshot, &me);
      for (; bMoreMods; bMoreMods = Module32NextW(hthSnapshot, &me)) {
         bFound = (_wcsicmp(me.szModule,  pszLibFile) == 0) || 
                  (_wcsicmp(me.szExePath, pszLibFile) == 0);
         if (bFound) break;
      }
      if (!bFound) __leave;

      // Get a handle for the target process.
      hProcess = OpenProcess(
         PROCESS_QUERY_INFORMATION |   
         PROCESS_CREATE_THREAD     | 
         PROCESS_VM_OPERATION,  // For CreateRemoteThread
         FALSE, dwProcessId);
      if (hProcess == NULL) __leave;

      // Get the real address of FreeLibrary in Kernel32.dll
      PTHREAD_START_ROUTINE pfnThreadRtn = (PTHREAD_START_ROUTINE)
         GetProcAddress(GetModuleHandle(TEXT("Kernel32")), "FreeLibrary");
      if (pfnThreadRtn == NULL) __leave;

      // Create a remote thread that calls FreeLibrary()
      hThread = CreateRemoteThread(hProcess, NULL, 0, 
         pfnThreadRtn, me.modBaseAddr, 0, NULL);
      if (hThread == NULL) __leave;

      // Wait for the remote thread to terminate
      WaitForSingleObject(hThread, INFINITE);

      bOk = TRUE; // Everything executed successfully
   }
   __finally { // Now we can clean everything up

      if (hthSnapshot != NULL) 
         CloseHandle(hthSnapshot);

      if (hThread     != NULL) 
         CloseHandle(hThread);

      if (hProcess    != NULL) 
         CloseHandle(hProcess);
   }

   return(bOk);
}


///////////////////////////////////////////////////////////////////////////////


BOOL WINAPI EjectLibA(DWORD dwProcessId, PCSTR pszLibFile) {

   // Allocate a (stack) buffer for the Unicode version of the pathname
   SIZE_T cchSize = lstrlenA(pszLibFile) + 1;
   PWSTR pszLibFileW = (PWSTR) 
      _alloca(cchSize * sizeof(wchar_t));

   // Convert the ANSI pathname to its Unicode equivalent
      StringCchPrintfW(pszLibFileW, cchSize, L"%S", pszLibFile);

   // Call the Unicode version of the function to actually do the work.
   return(EjectLibW(dwProcessId, pszLibFileW));
}


///////////////////////////////////////////////////////////////////////////////


BOOL Dlg_OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam) {

   chSETDLGICONS(hWnd, IDI_INJLIB);
   return(TRUE);
}


///////////////////////////////////////////////////////////////////////////////


void Dlg_OnCommand(HWND hWnd, int id, HWND hWndCtl, UINT codeNotify) {
   
   switch (id) {
      case IDCANCEL:
         EndDialog(hWnd, id);
         break;

      case IDC_INJECT:
      {
         TCHAR szLibFile[MAX_PATH];
         // GetModuleFileName(NULL, szLibFile, _countof(szLibFile));
         // PTSTR pFilename = _tcsrchr(szLibFile, TEXT('\\')) + 1;
         // _tcscpy_s(pFilename, _countof(szLibFile) - (pFilename - szLibFile),
         //     TEXT("imgwalk.dll"));
         Edit_GetText(GetDlgItem(hWnd,IDC_EDIT_DLLPATH), szLibFile, _countof(szLibFile));
         if (lstrcmpi(szLibFile,_T("")) == 0){
            chMB("Dll Path incorrect.");
            break;
         }

         DWORD dwProcessId = GetDlgItemInt(hWnd, IDC_PROCESSID, NULL, FALSE);
         if (dwProcessId == 0){
            chMB("Process id incorrect.");
            break;
         }
         // if (dwProcessId == 0) {
         //    // A process ID of 0 causes everything to take place in the 
         //    // local process; this makes things easier for debugging.
         //    dwProcessId = GetCurrentProcessId();
         // }
         if (InjectLib(dwProcessId, szLibFile)) {
            // chVERIFY(EjectLib(dwProcessId, szLibFile));
            chMB("DLL Injection/Ejection successful.");
         } else {
            chMB("DLL Injection/Ejection failed.");
         }
      }
         break;
      case IDC_BROWSEFILE:
      {
         OPENFILENAME opfn;  
         TCHAR strFilename[MAX_PATH] = {0};
         ZeroMemory(&opfn, sizeof(OPENFILENAME));  
         opfn.lStructSize = sizeof(OPENFILENAME);
         opfn.lpstrFilter = L"Dynamic link library\0*.dll\0All files\0*.*\0";  
         //默认过滤器索引设为1  
         opfn.nFilterIndex = 1;  
         opfn.lpstrFile = strFilename;  
         opfn.nMaxFile = _countof(strFilename);  
         //设置标志位，检查目录或文件是否存在  
         opfn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;  
         //opfn.lpstrInitialDir = NULL;  
         if(GetOpenFileName(&opfn))  
         {  
            HWND hEdt = GetDlgItem(hWnd, IDC_EDIT_DLLPATH);  
            Edit_SetText(hEdt,strFilename);
            // SendMessage(hEdt, WM_SETTEXT, NULL, (LPARAM)strFilename);  
         } 
      }
 
         break;
   }
}

BOOL Dlg_OnCopyData(HWND hWnd, HWND hWndFrom, PCOPYDATASTRUCT pcds) {

   // Some hooked process sent us some message box info, display it
   SetDlgItemTextW(hWnd, IDC_INFO, (PCWSTR) pcds->lpData);
   return(TRUE);
}



///////////////////////////////////////////////////////////////////////////////


INT_PTR WINAPI Dlg_Proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

   switch (uMsg) {
      chHANDLE_DLGMSG(hWnd, WM_INITDIALOG, Dlg_OnInitDialog);
      chHANDLE_DLGMSG(hWnd, WM_COMMAND,    Dlg_OnCommand);
      chHANDLE_DLGMSG(hWnd, WM_COPYDATA,   Dlg_OnCopyData);
   }
   return(FALSE);
}


///////////////////////////////////////////////////////////////////////////////


int WINAPI _tWinMain(HINSTANCE hInstExe, HINSTANCE, PTSTR pszCmdLine, int) {

   DialogBox(hInstExe, MAKEINTRESOURCE(IDD_INJLIB), NULL, Dlg_Proc);
   return(0);
}


//////////////////////////////// End of File //////////////////////////////////