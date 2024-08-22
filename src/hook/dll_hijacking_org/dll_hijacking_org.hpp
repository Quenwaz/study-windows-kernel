#ifndef __DLL_HIJACKING_ORG_INCLUDED__
#define __DLL_HIJACKING_ORG_INCLUDED__

#ifdef DLL_HIJACKING_EXPORT
#define DLL_HIJACKING_API _declspec(dllexport)
#else
#define DLL_HIJACKING_API _declspec(dllimport)
#endif

DLL_HIJACKING_API double some_func_a(double a1, int a2, int a3);
#endif // __DLL_HIJACKING_ORG_INCLUDED__
