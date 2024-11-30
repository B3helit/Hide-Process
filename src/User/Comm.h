#pragma once
#include "Windows.h"
constexpr ULONG hideCode = CTL_CODE(FILE_DEVICE_UNKNOWN, 0x914, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);


struct data_transfer {
	ULONG PID;
   // PVOID buffer;
};
class driver_manager {
    HANDLE m_driver_handle = nullptr;
public:
    ULONG procID;
    driver_manager(const char* driver_name) {
        m_driver_handle = CreateFileA(driver_name, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    }


    driver_manager(const char* driver_name, DWORD target_process_id) {
        m_driver_handle = CreateFileA(driver_name, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        procID = target_process_id;
    }

    void closeHandle() {
        CloseHandle(m_driver_handle);
    }

    void _KRNL_CALL() {
        data_transfer io_info;
        io_info.PID = procID;
        DeviceIoControl(m_driver_handle, hideCode, &io_info, sizeof(io_info), &io_info, sizeof(io_info), nullptr, nullptr);
    }
};