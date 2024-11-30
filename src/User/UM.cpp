// UM.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "Comm.h"
#include <tlhelp32.h> 


int findProcId(const char* procname) {

    HANDLE hSnapshot;
    PROCESSENTRY32 pe;
    int pid = 0;
    BOOL hResult;
    
    // snapshot of all processes in the system
    hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (INVALID_HANDLE_VALUE == hSnapshot) return 0;

    // initializing size: needed for using Process32First
    pe.dwSize = sizeof(PROCESSENTRY32);

    // info about first process encountered in a system snapshot
    hResult = Process32First(hSnapshot, &pe);

    // retrieve information about the processes
    // and exit if unsuccessful
    while (hResult) {
        // if we find the process: return process ID
        if (strcmp(procname, pe.szExeFile) == 0) {
            pid = pe.th32ProcessID;
            break;
        }
        hResult = Process32Next(hSnapshot, &pe);
    }

    // closes an open handle (CreateToolhelp32Snapshot)  
    CloseHandle(hSnapshot);
    return pid;
}

int main(int argc, char* argv[])
{
    int y = 0;
    char str[22];
    char bytes[] = {0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16};

    if (!argv[1]) {
        printf("Please specify the process's name by passing the arguments pre app run");
        return -1;
    }
    while ((*(argv[1] + bytes[y]))!=NULL) {
        str[y] = *(argv[1]+bytes[y]);
        y++;
    }

    ULONG process_id = findProcId(str);
    printf("%d\n", process_id);

    driver_manager* dm = new driver_manager("\\\\.\\HideProcess", process_id);
    dm->procID = process_id;
    dm->_KRNL_CALL();
    if(dm->procID)
        std::cout << "Operation Done\n";
    else {
        std::cout << "Error occured\n";
        return -1;
    }
    dm->closeHandle();
    return 0;
}