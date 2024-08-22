/******************************************************************************
Module:  LastMsgBoxInfoLib.h
Notices: Copyright (c) 2008 Jeffrey Richter & Christophe Nasarre
******************************************************************************/


#ifndef INTERCEPTIONAPI 
#define INTERCEPTIONAPI extern "C" __declspec(dllimport)
#endif


///////////////////////////////////////////////////////////////////////////////


INTERCEPTIONAPI BOOL WINAPI HookAllAppsMsgBox(BOOL bInstall, 
   DWORD dwThreadId);


//////////////////////////////// End of File //////////////////////////////////