#include <iostream>
#include <windows.h>
#include <hidsdi.h>
#include <hidusage.h>
#include <stdio.h>
_declspec(dllimport) double some_func_b(double a1, double a2);
_declspec(dllimport) double some_func_a(double a1, int a2, int a3);


typedef DWORD(*TalentGetDongleType)(int, BYTE*, BYTE*, int*);
typedef int(*TalentIsValidLicence)(INT,const char*, DWORD &,DWORD,int);
int main()
{
    std::clog << "PID=" << GetCurrentProcessId() << std::endl;
    getchar();

    // PVOID address = &some_func_a;
    // std::clog << "some_func_a(1,2,3)=" << some_func_a(1,6,3) << std::endl;
    // some_func_b(1,2);
    std::clog << "Hello Dynamic link library\n";

    HMODULE hmodule = LoadLibrary("hid.dll");
    if (hmodule != NULL){
        TalentGetDongleType fn = (TalentGetDongleType)GetProcAddress(hmodule, "Talent_GetDongleType");
        if (fn != NULL){
            BYTE bsLic[89] = {0};
            INT dongleType = 0;

            fn(0,bsLic,bsLic,&dongleType);
            std::clog << "TalentGetDongleType\n";
        }

        TalentIsValidLicence fn1 = (TalentIsValidLicence)GetProcAddress(hmodule, "Talent_IsValidLicence");
        if (fn1 != NULL){
            DWORD dwLicenseThisTimestamp= 0;
            fn1(0,"vxQc7NMxAEg1iG8/t1c6IiCl+NpD9RfG8En/YCWhm0oTMNo4N6XOaoBThVTPtDICk5OP9bAQsG30JTfTjk6Uyg==",dwLicenseThisTimestamp,0,1);
            std::clog << "Talent_IsValidLicence\n";
        }
    }
    getchar();
    return 0;
}