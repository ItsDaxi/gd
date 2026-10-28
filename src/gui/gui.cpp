#include "gui.hpp"
#include "menu.hpp"
#include "theme.hpp"
#include "core/hooks.hpp"
#include "core/log.hpp"
#include "hacks/config.hpp"
#include "hacks/registry.hpp"
#include "gd/gd.hpp"

#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_win32.h>

#include <mutex>
#include <vector>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace mh::gui {

static HWND g_hwnd = nullptr;
static WNDPROC g_origWndProc = nullptr;
static bool g_init = false;
static int* g_captureTarget = nullptr;
static bool g_keys[256]{};
static std::mutex g_queueMutex;
static std::vector<std::pair<void (*)(void*), void*>> g_queue;

HWND window() { return g_hwnd; }
bool initialized() { return g_init; }
void beginKeyCapture(int* target) { g_captureTarget = target; }
bool capturingKey() { return g_captureTarget != nullptr; }
bool keyDown(int vk) { return vk > 0 && vk < 256 && g_keys[vk]; }

void runOnMainThread(void (*fn)(void*), void* ctx) {
    std::lock_guard lock(g_queueMutex);
    g_queue.emplace_back(fn, ctx);
}

static bool isMouseMsg(UINT msg) {
    return (msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST) || msg == WM_NCMOUSEMOVE || msg == WM_MOUSEHOVER;
}

static bool isKeyMsg(UINT msg) {
    return msg == WM_KEYDOWN || msg == WM_KEYUP || msg == WM_SYSKEYDOWN || msg == WM_SYSKEYUP || msg == WM_CHAR;
}

static LRESULT CALLBACK hookedWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN || msg == WM_KEYUP || msg == WM_SYSKEYUP) {
        bool down = msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN;
        int vk = static_cast<int>(wp);
        if (vk > 0 && vk < 256) g_keys[vk] = down;
        bool repeat = (lp & (1 << 30)) != 0;
        if (down && !repeat) {
            if (g_captureTarget) {
                *g_captureTarget = vk == VK_ESCAPE ? 0 : vk;
                g_captureTarget = nullptr;
                config::markDirty();
                return 0;
            }
            bool typing = g_init && ImGui::GetIO().WantTextInput;
            if (!typing) {
                auto& ui = config::ui();
                if (vk == ui.toggleKey || (ui.altToggleKey && vk == ui.altToggleKey)) {
                    toggleMenu();
                    return 0;
                }
                if (dispatchKey(vk, true)) return 0;
            }
        } else if (!down) {
            dispatchKey(static_cast<int>(wp), false);
        }
    }

    if (g_init) {
        ImGui_ImplWin32_WndProcHandler(hwnd, msg, wp, lp);
        auto& io = ImGui::GetIO();
        if (isMouseMsg(msg) && io.WantCaptureMouse) return 0;
        if (isKeyMsg(msg) && io.WantCaptureKeyboard) return 0;
    }
    return CallWindowProcW(g_origWndProc, hwnd, msg, wp, lp);
}

static void loadFonts(float scale) {
    auto& io = ImGui::GetIO();
    io.Fonts->Clear();
    ImFontConfig cfg;
    cfg.OversampleH = 2;
    static const ImWchar ranges[] = {0x0020, 0x00FF, 0x0100, 0x017F, 0x2000, 0x206F, 0x25A0, 0x25FF, 0};
    const char* candidates[] = {"C:\\Windows\\Fonts\\segoeui.ttf", "C:\\Windows\\Fonts\\arial.ttf"};
    ImFont* font = nullptr;
    for (auto path : candidates) {
        if (GetFileAttributesA(path) == INVALID_FILE_ATTRIBUTES) continue;
        font = io.Fonts->AddFontFromFileTTF(path, 16.0f * scale, &cfg, ranges);
        if (font) break;
    }
    if (!font) {
        ImFontConfig def;
        def.SizePixels = 13.0f * scale;
        io.Fonts->AddFontDefault(&def);
    }
    // font grande per overlay di stato
    for (auto path : candidates) {
        if (GetFileAttributesA(path) == INVALID_FILE_ATTRIBUTES) continue;
        if (io.Fonts->AddFontFromFileTTF(path, 28.0f, &cfg, ranges)) break;
    }
    io.Fonts->Build();
}

static float g_fontScale = 0.f;

static void initImGui(HWND hwnd) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplOpenGL3_Init();
    g_fontScale = config::ui().scale;
    loadFonts(g_fontScale);
    applyTheme();
    g_origWndProc = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(hookedWndProc)));
    g_hwnd = hwnd;
    g_init = true;
    MH_LOG("ImGui inizializzato (hwnd %p)", hwnd);
}

static void shutdownImGui() {
    if (!g_init) return;
    if (g_hwnd && g_origWndProc && IsWindow(g_hwnd)) {
        SetWindowLongPtrW(g_hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(g_origWndProc));
    }
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    g_init = false;
    g_hwnd = nullptr;
    g_origWndProc = nullptr;
}

static void renderFrame() {
    HDC dc = wglGetCurrentDC();
    HWND hwnd = dc ? WindowFromDC(dc) : nullptr;
    if (!hwnd) return;
    // GD ricrea la finestra quando si cambia fullscreen/finestra: reinizializza
    if (g_init && hwnd != g_hwnd) shutdownImGui();
    if (!g_init) initImGui(hwnd);

    {
        std::vector<std::pair<void (*)(void*), void*>> q;
        {
            std::lock_guard lock(g_queueMutex);
            q.swap(g_queue);
        }
        for (auto& [fn, ctx] : q) fn(ctx);
    }

    if (g_fontScale != config::ui().scale) {
        g_fontScale = config::ui().scale;
        loadFonts(g_fontScale);
        ImGui_ImplOpenGL3_DestroyFontsTexture();
        ImGui_ImplOpenGL3_CreateFontsTexture();
        applyTheme();
    }

    runFrameCallbacks();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplWin32_NewFrame();
    {
        // GD nasconde il cursore durante il gioco: disegnalo noi quando il menu e' aperto
        CURSORINFO ci{sizeof(ci)};
        bool hidden = GetCursorInfo(&ci) && !(ci.flags & CURSOR_SHOWING);
        ImGui::GetIO().MouseDrawCursor = menuOpen() && hidden;
    }
    ImGui::NewFrame();
    drawOverlays();
    drawMenu();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    config::tick();
}

// ------------------------------------------------------------------ hook
static void (*o_swapBuffers)(void*) = nullptr;
static void h_swapBuffers(void* self) {
    renderFrame();
    o_swapBuffers(self);
}

static BOOL(WINAPI* o_wglSwapBuffers)(HDC) = nullptr;
static BOOL WINAPI h_wglSwapBuffers(HDC dc) {
    renderFrame();
    return o_wglSwapBuffers(dc);
}

void install() {
    if (!hooks::exported("libcocos2d.dll", "?swapBuffers@CCEGLView@cocos2d@@UEAAXXZ", h_swapBuffers, &o_swapBuffers)) {
        MH_WARN("swapBuffers di cocos non trovato, uso wglSwapBuffers");
        hooks::exported("opengl32.dll", "wglSwapBuffers", h_wglSwapBuffers, &o_wglSwapBuffers);
    }
}

} // namespace mh::gui
