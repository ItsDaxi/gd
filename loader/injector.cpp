// Injector alternativo: avvia/trova GeometryDash.exe e inietta MegaHackGD.dll.
// Uso: MegaHackInjector.exe [percorso\MegaHackGD.dll]
#include <windows.h>
#include <tlhelp32.h>

#include <cstdio>
#include <string>

static DWORD findProcess(const wchar_t* name) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    PROCESSENTRY32W pe{};
    pe.dwSize = sizeof(pe);
    DWORD pid = 0;
    if (Process32FirstW(snap, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, name) == 0) {
                pid = pe.th32ProcessID;
                break;
            }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return pid;
}

int wmain(int argc, wchar_t** argv) {
    std::wstring dll;
    if (argc > 1) {
        dll = argv[1];
    } else {
        wchar_t self[MAX_PATH];
        GetModuleFileNameW(nullptr, self, MAX_PATH);
        dll = self;
        dll = dll.substr(0, dll.find_last_of(L"\\/") + 1) + L"MegaHackGD.dll";
    }
    wchar_t full[MAX_PATH];
    GetFullPathNameW(dll.c_str(), MAX_PATH, full, nullptr);
    if (GetFileAttributesW(full) == INVALID_FILE_ATTRIBUTES) {
        wprintf(L"DLL non trovata: %s\n", full);
        return 1;
    }

    wprintf(L"In attesa di GeometryDash.exe...\n");
    DWORD pid = 0;
    while (!(pid = findProcess(L"GeometryDash.exe"))) Sleep(250);
    Sleep(1500); // lascia inizializzare il gioco

    HANDLE proc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!proc) {
        wprintf(L"OpenProcess fallito (%lu). Prova come amministratore.\n", GetLastError());
        return 1;
    }
    size_t bytes = (wcslen(full) + 1) * sizeof(wchar_t);
    void* remote = VirtualAllocEx(proc, nullptr, bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remote || !WriteProcessMemory(proc, remote, full, bytes, nullptr)) {
        wprintf(L"Scrittura memoria fallita (%lu)\n", GetLastError());
        return 1;
    }
    auto loadLib = reinterpret_cast<LPTHREAD_START_ROUTINE>(
        GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW"));
    HANDLE th = CreateRemoteThread(proc, nullptr, 0, loadLib, remote, 0, nullptr);
    if (!th) {
        wprintf(L"CreateRemoteThread fallito (%lu)\n", GetLastError());
        return 1;
    }
    WaitForSingleObject(th, 10000);
    DWORD code = 0;
    GetExitCodeThread(th, &code);
    CloseHandle(th);
    VirtualFreeEx(proc, remote, 0, MEM_RELEASE);
    CloseHandle(proc);
    wprintf(code ? L"Iniettato! Premi Tab nel gioco.\n" : L"LoadLibrary fallito nel processo remoto.\n");
    return code ? 0 : 1;
}
