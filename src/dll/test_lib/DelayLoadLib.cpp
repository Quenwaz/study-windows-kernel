/******************************************************************************
Module:  DelayLoadLib.cpp
Notices: Copyright (c) 2008 Jeffrey Richter & Christophe Nasarre
******************************************************************************/


#pragma warning(push, 3)
#include <Windows.h>
#pragma warning(pop) 
#include <Windowsx.h>
#include <tchar.h>


///////////////////////////////////////////////////////////////////////////////


#define DELAYLOADLIBAPI extern "C" __declspec(dllexport)
#include "DelayLoadLib.h"


///////////////////////////////////////////////////////////////////////////////


int fnLib() {

   return(321);
}


///////////////////////////////////////////////////////////////////////////////


int fnLib2() {

   return(123);
}


//////////////////////////////// End of File //////////////////////////////////
