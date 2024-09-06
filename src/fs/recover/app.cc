#include <windows.h>
#include <iostream>
#include <vector>

enum{
    SECTOR_SIZE=512
};

// Function to read raw data from disk
bool ReadDiskSector(HANDLE hDisk, DWORD sectorNumber, BYTE* buffer) {
    LARGE_INTEGER sectorOffset;
    sectorOffset.QuadPart = SECTOR_SIZE * sectorNumber;

    DWORD bytesRead;
    SetFilePointerEx(hDisk, sectorOffset, NULL, FILE_BEGIN);
    return ReadFile(hDisk, buffer, SECTOR_SIZE, &bytesRead, NULL);
}

// Function to scan MFT records
void ScanMFT(HANDLE hDisk) {
    BYTE buffer[SECTOR_SIZE];
    DWORD sectorNumber = 0;

    while (ReadDiskSector(hDisk, sectorNumber, buffer)) {
        // Assuming this sector contains an MFT entry
        // Check the MFT entry header (file signature 'FILE')
        if (memcmp(buffer, "FILE", 4) == 0) {
            std::cout << "Found MFT entry at sector " << sectorNumber << std::endl;
            // Further processing to identify and recover deleted files
        }
        sectorNumber++;
    }
}

int main() {
    HANDLE hDisk = CreateFile(L"\\\\.\\F:", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                              NULL, OPEN_EXISTING, 0, NULL);
    if (hDisk == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to open disk" << std::endl;
        return 1;
    }

    ScanMFT(hDisk);

    CloseHandle(hDisk);
    return 0;
}
