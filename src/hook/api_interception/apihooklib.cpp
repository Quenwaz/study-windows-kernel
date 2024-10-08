/******************************************************************************
Module:  LastMsgBoxInfoLib.cpp
Notices: Copyright (c) 2008 Jeffrey Richter & Christophe Nasarre
******************************************************************************/


#include "common_header\CmnHdr.h"
#include <WindowsX.h>
#include <tchar.h>
#include <stdio.h>
#include "APIHook.h"

#define INTERCEPTIONAPI extern "C" __declspec(dllexport)
#include "apihooklib.h"
#include <StrSafe.h>
#include "sndcopymsg.h"


///////////////////////////////////////////////////////////////////////////////


// Prototypes for the hooked functions
typedef int (WINAPI *PFNMESSAGEBOXA)(HWND hWnd, PCSTR pszText, 
   PCSTR pszCaption, UINT uType);

typedef int (WINAPI *PFNMESSAGEBOXW)(HWND hWnd, PCWSTR pszText, 
   PCWSTR pszCaption, UINT uType);


// We need to reference these variables before we create them.
extern CAPIHook g_MessageBoxA;
extern CAPIHook g_MessageBoxW;


///////////////////////////////////////////////////////////////////////////////
void __SndCopyDataToWnd(bool bUnicode, PVOID msg, PVOID wndTitle)
{
   wchar_t szProcessPathname[MAX_PATH];
   GetModuleFileNameW(NULL, szProcessPathname, MAX_PATH);
   // Construct the string to send to the main dialog box
   wchar_t sz[2048];
   StringCchPrintfW(sz, _countof(sz), bUnicode 
      ? L"\r\nProcess: (%d) %s\r\n%s"
      : L"\r\nProcess: (%d) %s\r\n%S",
      GetCurrentProcessId(), szProcessPathname,msg);

   wchar_t title[32]={0};
   StringCchPrintfW(title, _countof(title), bUnicode 
      ? L"%s":L"%S", wndTitle);
   
   // Send the string to the main dialog box
   COPYDATASTRUCT cds = { 0, ((DWORD)wcslen(sz) + 1) * sizeof(wchar_t), sz };
   FORWARD_WM_COPYDATA(FindWindow(NULL, title), NULL, &cds, SendMessage);   
   // FORWARD_WM_COPYDATA(FindWindow(NULL, TEXT("Inject Library Tester")), 
   //    NULL, &cds, SendMessage);
}

void SndCopyDataToWndA(LPCSTR msg, LPCSTR wndTitle)
{
   __SndCopyDataToWnd(FALSE,(PVOID)msg,(PVOID)wndTitle);
}


void SndCopyDataToWndW(LPCWSTR msg, LPCWSTR wndTitle)
{
   __SndCopyDataToWnd(TRUE,(PVOID)msg,(PVOID)wndTitle);
}

// This function sends the MessageBox info to our main dialog box
void SendLastMsgBoxInfo(BOOL bUnicode, 
   PVOID pvCaption, PVOID pvText, int nResult) {

   // Convert the return value into a human-readable string
   PCWSTR pszResult = L"(Unknown)";
   switch (nResult) {
      case IDOK:       pszResult = L"Ok";        break;
      case IDCANCEL:   pszResult = L"Cancel";    break;
      case IDABORT:    pszResult = L"Abort";     break;
      case IDRETRY:    pszResult = L"Retry";     break;
      case IDIGNORE:   pszResult = L"Ignore";    break;
      case IDYES:      pszResult = L"Yes";       break;
      case IDNO:       pszResult = L"No";        break;
      case IDCLOSE:    pszResult = L"Close";     break;
      case IDHELP:     pszResult = L"Help";      break;
      case IDTRYAGAIN: pszResult = L"Try Again"; break;
      case IDCONTINUE: pszResult = L"Continue";  break;
   }

   // Construct the string to send to the main dialog box
   wchar_t sz[2048];
   StringCchPrintfW(sz, _countof(sz), bUnicode 
      ? L"Caption: %s\r\nMessage: %s\r\nResult: %s"
      : L"Caption: %S\r\nMessage: %S\r\nResult: %s",
      pvCaption, pvText, pszResult);

   __SndCopyDataToWnd(bUnicode,(PVOID)sz,(PVOID)L"Last MessageBox Info");
}


///////////////////////////////////////////////////////////////////////////////


// This is the MessageBoxW replacement function
int WINAPI Hook_MessageBoxW(HWND hWnd, PCWSTR pszText, LPCWSTR pszCaption, 
   UINT uType) {

   // Call the original MessageBoxW function
   int nResult = ((PFNMESSAGEBOXW)(PROC) g_MessageBoxW)
      (hWnd, pszText, pszCaption, uType);

   // Send the information to the main dialog box
   SendLastMsgBoxInfo(TRUE, (PVOID) pszCaption, (PVOID) pszText, nResult);

   // Return the result back to the caller
   return(nResult);
}


///////////////////////////////////////////////////////////////////////////////


// This is the MessageBoxA replacement function
int WINAPI Hook_MessageBoxA(HWND hWnd, PCSTR pszText, PCSTR pszCaption, 
   UINT uType) {

   // Call the original MessageBoxA function
   int nResult = ((PFNMESSAGEBOXA)(PROC) g_MessageBoxA)
      (hWnd, pszText, pszCaption, uType);

   // Send the information to the main dialog box
   SendLastMsgBoxInfo(FALSE, (PVOID) pszCaption, (PVOID) pszText, nResult);

   // Return the result back to the caller
   return(nResult);
}


///////////////////////////////////////////////////////////////////////////////


// Hook the MessageBoxA and MessageBoxW functions
CAPIHook g_MessageBoxA("User32.dll", "MessageBoxA", 
   (PROC) Hook_MessageBoxA);

CAPIHook g_MessageBoxW("User32.dll", "MessageBoxW", 
   (PROC) Hook_MessageBoxW);

HHOOK g_hhook = NULL;


///////////////////////////////////////////////////////////////////////////////


static LRESULT WINAPI GetMsgProc(int code, WPARAM wParam, LPARAM lParam) {
   return(CallNextHookEx(g_hhook, code, wParam, lParam));
}


///////////////////////////////////////////////////////////////////////////////


// Returns the HMODULE that contains the specified memory address
static HMODULE ModuleFromAddress(PVOID pv) {

   MEMORY_BASIC_INFORMATION mbi;
   return((VirtualQuery(pv, &mbi, sizeof(mbi)) != 0) 
      ? (HMODULE) mbi.AllocationBase : NULL);
}


///////////////////////////////////////////////////////////////////////////////


BOOL WINAPI HookAllAppsMsgBox(BOOL bInstall, DWORD dwThreadId) {

   BOOL bOk;

   if (bInstall) {

      chASSERT(g_hhook == NULL); // Illegal to install twice in a row

      // Install the Windows' hook
      g_hhook = SetWindowsHookEx(WH_GETMESSAGE, GetMsgProc, 
         ModuleFromAddress(HookAllAppsMsgBox), dwThreadId);

      bOk = (g_hhook != NULL);
   } else {

      chASSERT(g_hhook != NULL); // Can't uninstall if not installed
      bOk = UnhookWindowsHookEx(g_hhook);
      g_hhook = NULL;
   }

   return(bOk);
}


//////////////////////////////// End of File //////////////////////////////////