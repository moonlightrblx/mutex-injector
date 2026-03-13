#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <string>
#include <vector>

std::vector<DWORD> GetPIDsByName(const std::wstring& procName) {
    std::vector<DWORD> pids;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return pids;

    PROCESSENTRY32W entry;
    entry.dwSize = sizeof(entry);

    if (Process32FirstW(snapshot, &entry)) {
        do {
            if (_wcsicmp(entry.szExeFile, procName.c_str()) == 0) {
                pids.push_back(entry.th32ProcessID);
            }
        } while (Process32NextW(snapshot, &entry));
    }

    CloseHandle(snapshot);
    return pids;
}

bool InjectDLL(DWORD pid, const std::wstring& dllPath) {
    HANDLE hMutex = CreateMutexA(NULL, FALSE, "Global\\ShxdowIsMyDaddyAndHeOwnsMe");
    if (!hMutex) {
        std::cerr << "Failed to verify injector.\n";
        // eh doesnt really matter atm but its nice knowing we can do this <3 
    }

    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProcess) {
        std::wcerr << L"Failed to open process " << pid << L". Error: " << GetLastError() << L"\n";
        if (hMutex) CloseHandle(hMutex);
        return false;
    }

    size_t size = (dllPath.size() + 1) * sizeof(wchar_t);
    LPVOID remoteMem = VirtualAllocEx(hProcess, nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remoteMem) {
        std::cerr << "Failed to allocate memory in target process.\n";
        CloseHandle(hProcess);
        if (hMutex) CloseHandle(hMutex);
        return false;
    }

    if (!WriteProcessMemory(hProcess, remoteMem, dllPath.c_str(), size, nullptr)) {
        std::cerr << "Failed to write DLL path to target process.\n";
        VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        if (hMutex) CloseHandle(hMutex);
        return false;
    }

    LPTHREAD_START_ROUTINE loadLibAddr = (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandle(L"kernel32.dll"), "LoadLibraryW");
    if (!loadLibAddr) {
        std::cerr << "Failed to get LoadLibraryW address.\n";
        VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        if (hMutex) CloseHandle(hMutex);
        return false;
    }

    HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0, loadLibAddr, remoteMem, 0, nullptr);
    if (!hThread) {
        std::cerr << "Failed to create remote thread.\n";
        VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        if (hMutex) CloseHandle(hMutex);
        return false;
    }

    WaitForSingleObject(hThread, INFINITE);

    VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
    CloseHandle(hThread);
    CloseHandle(hProcess);

    if (hMutex) CloseHandle(hMutex);

    std::cout << "Injected into PID " << pid << " successfully.\n";
    return true;
}

int wmain() {
    std::wstring dllPath = L"patcher.dll";
    std::wstring targetProc = L"Shxdow.exe";

    std::vector<DWORD> pids = GetPIDsByName(targetProc);

    if (pids.empty()) {
        std::wcout << L"No processes named " << targetProc << L" found.\n";
        system("pause > nul");
        return 0;
    }

    for (DWORD pid : pids) {
        std::wcout << L"Injecting into PID: " << pid << L"\n";
        if (!InjectDLL(pid, dllPath)) {
            std::wcerr << L"Failed to inject into PID: " << pid << L"\n";
            system("pause > nul");
        }
    }
    std::wcout << L"Successfully injected. created by cloud :3";
    system("pause > nul");
    return 0;
}
