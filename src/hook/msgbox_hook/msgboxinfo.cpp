/******************************************************************************
Module:  LastMsgBoxInfo.cpp
Notices: Copyright (c) 2008 Jeffrey Richter & Christophe Nasarre
******************************************************************************/


#include "common_header\CmnHdr.h"     /* See Appendix A. */
#include <windowsx.h>
#include <tchar.h>
#include "Resource.h"
#include "dllhook/api_interception/apihooklib.h"


///////////////////////////////////////////////////////////////////////////////


BOOL Dlg_OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam) {

   chSETDLGICONS(hWnd, IDI_LASTMSGBOXINFO);
   SetDlgItemText(hWnd, IDC_INFO, 
      TEXT("Waiting for a API interception to be dismissed"));
   return(TRUE);
}


///////////////////////////////////////////////////////////////////////////////


void Dlg_OnSize(HWND hWnd, UINT state, int cx, int cy) {

   SetWindowPos(GetDlgItem(hWnd, IDC_INFO), NULL, 
      0, 25, cx, cy, SWP_NOZORDER);

   RECT rcEditFilter, rcBtnClear;
   HWND hBtnClear = GetDlgItem(hWnd, IDC_BTN_CLEAR);
   GetClientRect(hBtnClear,&rcBtnClear);
   SetWindowPos(hBtnClear, NULL,  cx-rcBtnClear.right, 0, 0, 0, SWP_NOSIZE);
   HWND hEditFilter = GetDlgItem(hWnd, IDC_EDIT_FILTER);
   GetClientRect(hEditFilter,&rcEditFilter);
   SetWindowPos(hEditFilter, NULL,  0, 0, cx - rcBtnClear.right, 21, SWP_NOMOVE);
}


///////////////////////////////////////////////////////////////////////////////


void Dlg_OnCommand(HWND hWnd, int id, HWND hWndCtl, UINT codeNotify) {

   switch (id) {
      case IDCANCEL:
         EndDialog(hWnd, id);
         break;
      case IDC_BTN_CLEAR:
         Edit_SetText(GetDlgItem(hWnd,IDC_INFO), L"");
         break;
   }
}


///////////////////////////////////////////////////////////////////////////////


BOOL Dlg_OnCopyData(HWND hWnd, HWND hWndFrom, PCOPYDATASTRUCT pcds) {

   PCWSTR text = (PCWSTR) pcds->lpData;
   TCHAR szFilter[512] ={0};
   Edit_GetText(GetDlgItem(hWnd,IDC_EDIT_FILTER),szFilter,_countof(szFilter));
   if (lstrlen(szFilter) > 0 && _tcsstr(text,szFilter) == NULL){
      return(TRUE);
   }
   HWND hedit = GetDlgItem(hWnd,IDC_INFO);
   int textlen = Edit_GetTextLength(hedit);
   Edit_SetSel(hedit,textlen,textlen);
   Edit_ReplaceSel(hedit,text);
   SendMessageW(hedit, WM_VSCROLL, SB_BOTTOM,0);
   // Edit_Scroll(hedit, 0, 0);
   // SetDlgItemTextW(hWnd, IDC_INFO, (PCWSTR) pcds->lpData);
   return(TRUE);
}


///////////////////////////////////////////////////////////////////////////////


INT_PTR WINAPI Dlg_Proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

   switch (uMsg) {
      chHANDLE_DLGMSG(hWnd, WM_INITDIALOG, Dlg_OnInitDialog);
      chHANDLE_DLGMSG(hWnd, WM_SIZE,       Dlg_OnSize);
      chHANDLE_DLGMSG(hWnd, WM_COMMAND,    Dlg_OnCommand);
      chHANDLE_DLGMSG(hWnd, WM_COPYDATA,   Dlg_OnCopyData);
   }
   return(FALSE);
}


///////////////////////////////////////////////////////////////////////////////


int WINAPI _tWinMain(HINSTANCE hInstExe, HINSTANCE, PTSTR pszCmdLine, int) {

   DWORD dwThreadId = 0;
   HookAllAppsMsgBox(TRUE, dwThreadId);
   DialogBox(hInstExe, MAKEINTRESOURCE(IDD_LASTMSGBOXINFO), NULL, Dlg_Proc);
   HookAllAppsMsgBox(FALSE, 0);
   return(0);
}


//////////////////////////////// End of File //////////////////////////////////