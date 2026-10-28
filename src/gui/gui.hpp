#pragma once
#include <windows.h>

namespace mh::gui {

// Installa l'hook di rendering (CCEGLView::swapBuffers / wglSwapBuffers)
void install();

HWND window();
bool initialized();

// Prossimo tasto premuto (per assegnare keybind); -1 = nessuno in attesa
void beginKeyCapture(int* target);
bool capturingKey();

// Stato input (aggiornato dalla WndProc)
bool keyDown(int vk);

// Esegue una funzione sul thread principale al prossimo frame
void runOnMainThread(void (*fn)(void*), void* ctx);

} // namespace mh::gui
