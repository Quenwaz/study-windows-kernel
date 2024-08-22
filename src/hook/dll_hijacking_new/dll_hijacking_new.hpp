#ifndef __DLL_HIJACKING_NEW_INCLUDED__
#define __DLL_HIJACKING_NEW_INCLUDED__

#ifdef DLL_HIJACKING_EXPORT
#define DLL_HIJACKING_API _declspec(dllexport)
#else
#define DLL_HIJACKING_API _declspec(dllimport)
#endif



DLL_HIJACKING_API double some_func_b(double a1,double a2);
#endif // __DLL_HIJACKING_NEW_INCLUDED__