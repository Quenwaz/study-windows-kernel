// common.h
#pragma once
#include <windows.h>
#include <iostream>
#include <string>

const wchar_t* PIPE_NAME = L"\\\\.\\pipe\\my_named_pipe";
const wchar_t* MUTEX_NAME = L"Global\\MyMutex";


int server_logic()
{
    HANDLE hPipe = CreateNamedPipe(
        PIPE_NAME,
        PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        PIPE_UNLIMITED_INSTANCES,
        0,
        0,
        0,
        NULL);

    if (hPipe == INVALID_HANDLE_VALUE) {
        std::cerr << "CreateNamedPipe failed. Error: " << GetLastError() << std::endl;
        return 1;
    }

    std::cout << "Writer process started. Enter messages (type 'exit' to quit):" << std::endl;

    std::string message;
    while (true) {
        std::getline(std::cin, message);
        if (message == "exit") break;

        if (ConnectNamedPipe(hPipe, NULL) != FALSE || 
            GetLastError() == ERROR_PIPE_CONNECTED) {
            DWORD bytesWritten;
            if (WriteFile(hPipe, message.c_str(), message.length() + 1, &bytesWritten, NULL) == FALSE) {
                std::cerr << "WriteFile failed. Error: " << GetLastError() << std::endl;
            }

            std::string buffer(512, 0);
            DWORD bytesRead = 0;
            if (ReadFile(hPipe, buffer.data(), buffer.length(), &bytesRead, NULL) != FALSE) {
                std::cout << "Received: " << buffer.c_str() << std::endl;
            }


            DisconnectNamedPipe(hPipe);
        }else{
            DWORD error = GetLastError();
            if (error == ERROR_PIPE_CONNECTED){
                std::cerr << "There is a process on other end of the pipe." <<std::endl;
            }
        }
    }

    CloseHandle(hPipe);
    return 0;
}



int client_logic() {
    HANDLE hMutex = CreateMutex(NULL, FALSE, MUTEX_NAME);
    if (hMutex == NULL) {
        std::cerr << "CreateMutex failed. Error: " << GetLastError() << std::endl;
        return 1;
    }

    std::cout << "Reader process started." << std::endl;

    while (true) {
        if (!WaitNamedPipe(PIPE_NAME,NMPWAIT_WAIT_FOREVER)){
            std::cerr << "WaitNamedPipe failed. Error: " << GetLastError() << std::endl;
            // ERROR_FILE_NOT_FOUND
            break;
        }
        std::cout << "pipe available\n";
        
        HANDLE hPipe = CreateFile(
            PIPE_NAME,
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            0,
            NULL);

        if (hPipe != INVALID_HANDLE_VALUE) {
            std::string buffer(256,0);
            DWORD bytesRead;

            WaitForSingleObject(hMutex, INFINITE);
            if (ReadFile(hPipe, buffer.data(), buffer.length(), &bytesRead, NULL) != FALSE) {
                std::cout << "Received: " << buffer << std::endl;

                DWORD bytesWritten;
                if (WriteFile(hPipe, buffer.c_str(), buffer.length() + 1, &bytesWritten, NULL) == FALSE) {
                    std::cerr << "WriteFile failed. Error: " << GetLastError() << std::endl;
                }

            } else {
                std::cerr << "ReadFile failed. Error: " << GetLastError() << std::endl;
                break;
            }

            ReleaseMutex(hMutex);
            CloseHandle(hPipe);
        } else {

            DWORD error = GetLastError();
            if (error == ERROR_PIPE_BUSY) {
                std::cout << "Pipe busy. Waiting..." << std::endl;
            } else {
                std::cerr << "Could not open pipe. Error: " << error << std::endl;
                break;
            }
        }
    }

    CloseHandle(hMutex);
    return 0;
}

int main(int argc, char const *argv[])
{
    if (argc > 1) {
        std::string arg = argv[1];
        if (arg == "worker") {
            return client_logic();
        } else {
            std::cerr << "Unknown argument: " << arg << std::endl;
        }
    }
    std::string cmd = "start ";
    cmd += strrchr(argv[0], '\\') + 1;
    const auto num_of_worker = 2;
    for(size_t i = 0;i < num_of_worker; ++i){
        std::system((cmd + " worker").c_str());
    }
    server_logic();
    return 0;
}


