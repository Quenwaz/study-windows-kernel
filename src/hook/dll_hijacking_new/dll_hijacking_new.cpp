
#include "dll_hijacking_new.hpp"
#include "common_header/CmnHdr.h"  
#include <cstdio>
#include <tchar.h>
#include <Windows.h>
#include <ImageHlp.h>
#pragma comment(lib, "ImageHlp")



DLL_HIJACKING_API double some_func_a(double a1, int a2, int a3)
{
    fprintf(stderr, "dll_hijacking_new::some_func_a\n");

    return a1 * a2 * a3;
}



double some_tst(double a1, int a2, int a3)
{
    fprintf(stderr, "dll_hijacking_new::some_tst\n");

    return a1 + a2 + a3;
}

DLL_HIJACKING_API double some_func_b(double a1,double a2)
{

    fprintf(stderr, "dll_hijacking_new::some_func_b\n");
    return a1/ a2;
}

typedef double(*SomeFunA)(double, double,double);
typedef double(*OrignalFun)(double);

BOOL WINAPI DllMain(HINSTANCE hInstDll, DWORD fdwReason, PVOID fImpLoad)
{
    if(fdwReason == DLL_PROCESS_ATTACH){

        FARPROC pOrgFn = NULL;
        HMODULE hmodule = LoadLibrary(L"dll_hijacking_org.dll");
        if (hmodule != NULL){
            OrignalFun orgfun = (OrignalFun)GetProcAddress(hmodule, "?some_func_c@@YANN@Z");
            if (orgfun != NULL){
                fprintf(stderr, "dll_hijacking_org::some_func_c\n");
                orgfun(1);
            }else{
                fprintf(stderr, "GetLastError()=%d\n", GetLastError());
            }
            pOrgFn = GetProcAddress(hmodule, "?some_func_a@@YANNHH@Z");
            SomeFunA orgfun1 = (SomeFunA)pOrgFn;
            PROC pNewFn = (PROC)&some_tst;
            WriteProcessMemory(GetCurrentProcess(), orgfun1,&pNewFn, sizeof(pNewFn),0);
            DWORD lasterr = GetLastError();
            FreeLibrary(hmodule);
            // fprintf(stderr, "orgfun1(1,2,3)=%f\n",orgfun1(1,2,3));

        }

        // char szBuf[MAX_PATH * 100] = { 0 };

        // PBYTE pb = NULL;
        // MEMORY_BASIC_INFORMATION mbi;
        // while (VirtualQuery(pb, &mbi, sizeof(mbi)) == sizeof(mbi)) {

        //     int nLen;
        //     char szModName[MAX_PATH];

        //     if (mbi.State == MEM_FREE)
        //         mbi.AllocationBase = mbi.BaseAddress;

        //     if ((mbi.AllocationBase == hInstDll) ||
        //         (mbi.AllocationBase != mbi.BaseAddress) ||
        //         (mbi.AllocationBase == NULL)) {
        //         // Do not add the module name to the list
        //         // if any of the following is true:
        //         // 1. If this region contains this DLL
        //         // 2. If this block is NOT the beginning of a region
        //         // 3. If the address is NULL
        //         nLen = 0;
        //     } else {
        //         nLen = GetModuleFileNameA((HINSTANCE) mbi.AllocationBase, 
        //         szModName, _countof(szModName));
        //     }

        //     if (nLen > 0) {
        //         PSTR name = strrchr(szModName, '\\') + 1;
        //         if (lstrcmpiA(name, "dynamic_link_library.exe") == 0)
        //         {
        //             ULONG ulSize;
        //             PIMAGE_IMPORT_DESCRIPTOR pImportDesc = (PIMAGE_IMPORT_DESCRIPTOR) ImageDirectoryEntryToData(
        //                 (HINSTANCE) mbi.AllocationBase, TRUE, IMAGE_DIRECTORY_ENTRY_IMPORT, &ulSize);

        //             for (; pImportDesc->Name; pImportDesc++) {
        //                 PSTR pszModName = (PSTR) ((PBYTE) (HINSTANCE) mbi.AllocationBase + pImportDesc->Name);
        //                 if (lstrcmpiA(pszModName, "dll_hijacking_org.dll") == 0) {
        //                     PIMAGE_THUNK_DATA pThunk = (PIMAGE_THUNK_DATA) 
        //                     ((PBYTE) (HINSTANCE) mbi.AllocationBase + pImportDesc->FirstThunk);

        //                     // Replace current function address with new function address
        //                     for (; pThunk->u1.Function; pThunk++) {

        //                         // Get the address of the function address
        //                         PROC* ppfn = (PROC*) &pThunk->u1.Function;

        //                         if (*ppfn == pOrgFn)
        //                         {

        //                             chMB("Found Function");
        //                         }
        //                     }
        //                 }

        //             }

        //         }

        //         wsprintfA(strchr(szBuf, 0), "\n%p-%s", 
        //         mbi.AllocationBase, szModName);
        //     }
            
        //     pb += mbi.RegionSize;
        // }

        // // NOTE: Normally, you should not display a message box in DllMain
        // // due to the loader lock described in Chapter 20. However, to keep
        // // this sample application simple, I am violating this rule.
        // chMB(&szBuf[1]);


    }
    return (TRUE);
}

