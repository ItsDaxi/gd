// Proxy di XInput1_4.dll: Geometry Dash lo importa all'avvio, quindi mettendo
// questa DLL nella cartella del gioco viene caricata automaticamente.
// Inoltra tutte le funzioni alla vera XInput1_4.dll di System32 e carica MegaHackGD.dll.
#include <windows.h>

namespace {

HMODULE g_real = nullptr;

HMODULE real() {
    if (!g_real) {
        wchar_t path[MAX_PATH];
        GetSystemDirectoryW(path, MAX_PATH);
        lstrcatW(path, L"\\XInput1_4.dll");
        g_real = LoadLibraryW(path);
    }
    return g_real;
}

template <class F>
F fn(const char* name) {
    HMODULE m = real();
    return m ? reinterpret_cast<F>(GetProcAddress(m, name)) : nullptr;
}

template <class F>
F fnOrd(WORD ord) {
    HMODULE m = real();
    return m ? reinterpret_cast<F>(GetProcAddress(m, MAKEINTRESOURCEA(ord))) : nullptr;
}

DWORD WINAPI loadMod(LPVOID) {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    // sostituisce GeometryDash.exe con MegaHackGD.dll
    wchar_t* slash = path;
    for (wchar_t* p = path; *p; p++)
        if (*p == L'\\' || *p == L'/') slash = p + 1;
    *slash = 0;
    lstrcatW(path, L"MegaHackGD.dll");
    if (!LoadLibraryW(path)) {
        wchar_t msg[MAX_PATH + 128];
        wsprintfW(msg, L"Impossibile caricare %s (errore %lu)", path, GetLastError());
        MessageBoxW(nullptr, msg, L"Mega Hack GD loader", MB_ICONERROR);
    }
    return 0;
}

} // namespace

extern "C" {

DWORD WINAPI XInputGetState(DWORD idx, void* state) {
    using F = DWORD(WINAPI*)(DWORD, void*);
    static F f = fn<F>("XInputGetState");
    return f ? f(idx, state) : ERROR_DEVICE_NOT_CONNECTED;
}

DWORD WINAPI XInputSetState(DWORD idx, void* vib) {
    using F = DWORD(WINAPI*)(DWORD, void*);
    static F f = fn<F>("XInputSetState");
    return f ? f(idx, vib) : ERROR_DEVICE_NOT_CONNECTED;
}

DWORD WINAPI XInputGetCapabilities(DWORD idx, DWORD flags, void* caps) {
    using F = DWORD(WINAPI*)(DWORD, DWORD, void*);
    static F f = fn<F>("XInputGetCapabilities");
    return f ? f(idx, flags, caps) : ERROR_DEVICE_NOT_CONNECTED;
}

void WINAPI XInputEnable(BOOL enable) {
    using F = void(WINAPI*)(BOOL);
    static F f = fn<F>("XInputEnable");
    if (f) f(enable);
}

DWORD WINAPI XInputGetAudioDeviceIds(DWORD idx, LPWSTR render, UINT* renderCount, LPWSTR capture, UINT* captureCount) {
    using F = DWORD(WINAPI*)(DWORD, LPWSTR, UINT*, LPWSTR, UINT*);
    static F f = fn<F>("XInputGetAudioDeviceIds");
    return f ? f(idx, render, renderCount, capture, captureCount) : ERROR_DEVICE_NOT_CONNECTED;
}

DWORD WINAPI XInputGetBatteryInformation(DWORD idx, BYTE type, void* info) {
    using F = DWORD(WINAPI*)(DWORD, BYTE, void*);
    static F f = fn<F>("XInputGetBatteryInformation");
    return f ? f(idx, type, info) : ERROR_DEVICE_NOT_CONNECTED;
}

DWORD WINAPI XInputGetKeystroke(DWORD idx, DWORD reserved, void* ks) {
    using F = DWORD(WINAPI*)(DWORD, DWORD, void*);
    static F f = fn<F>("XInputGetKeystroke");
    return f ? f(idx, reserved, ks) : ERROR_DEVICE_NOT_CONNECTED;
}

// export nascosti (solo ordinali)
DWORD WINAPI XInputGetStateEx(DWORD idx, void* state) {
    using F = DWORD(WINAPI*)(DWORD, void*);
    static F f = fnOrd<F>(100);
    return f ? f(idx, state) : ERROR_DEVICE_NOT_CONNECTED;
}

DWORD WINAPI XInputWaitForGuideButton(DWORD idx, DWORD flag, void* unk) {
    using F = DWORD(WINAPI*)(DWORD, DWORD, void*);
    static F f = fnOrd<F>(101);
    return f ? f(idx, flag, unk) : ERROR_DEVICE_NOT_CONNECTED;
}

DWORD WINAPI XInputCancelGuideButtonWait(DWORD idx) {
    using F = DWORD(WINAPI*)(DWORD);
    static F f = fnOrd<F>(102);
    return f ? f(idx) : ERROR_DEVICE_NOT_CONNECTED;
}

DWORD WINAPI XInputPowerOffController(DWORD idx) {
    using F = DWORD(WINAPI*)(DWORD);
    static F f = fnOrd<F>(103);
    return f ? f(idx) : ERROR_DEVICE_NOT_CONNECTED;
}

DWORD WINAPI XInputGetBaseBusInformation(DWORD idx, void* info) {
    using F = DWORD(WINAPI*)(DWORD, void*);
    static F f = fnOrd<F>(104);
    return f ? f(idx, info) : ERROR_DEVICE_NOT_CONNECTED;
}

DWORD WINAPI XInputGetCapabilitiesEx(DWORD unk, DWORD idx, DWORD flags, void* caps) {
    using F = DWORD(WINAPI*)(DWORD, DWORD, DWORD, void*);
    static F f = fnOrd<F>(108);
    return f ? f(unk, idx, flags, caps) : ERROR_DEVICE_NOT_CONNECTED;
}

} // extern "C"

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(module);
        // non si puo' chiamare LoadLibrary dentro DllMain in sicurezza: usa un thread
        HANDLE t = CreateThread(nullptr, 0, loadMod, nullptr, 0, nullptr);
        if (t) CloseHandle(t);
    }
    return TRUE;
}
