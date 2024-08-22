#ifndef _H_SND_COPY_MSG_INCLUDED__
#define _H_SND_COPY_MSG_INCLUDED__

#if defined(UNICODE) || defined(_UNICODE)
#define SndCopyDataToWnd SndCopyDataToWndW
#else
#define SndCopyDataToWnd SndCopyDataToWndA
#endif 


void SndCopyDataToWndA(LPCSTR msg, LPCSTR wndTitle="API Call Tracing");
void SndCopyDataToWndW(LPCWSTR msg, LPCWSTR wndTitle=L"API Call Tracing");

#endif // _H_SND_COPY_MSG_INCLUDED__