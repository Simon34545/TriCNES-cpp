#pragma once

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <dwmapi.h>

static HWND hWnd;
static HMENU hMenuBar;

#define MENU_ID UINT_PTR
#define MENU HMENU

#define MENU_CALLBACK(name) static void name(MENU_ID caller)

struct MenuCallback
{
	MENU_ID id;
	void (*callback)(MENU_ID caller);
};

std::vector<MenuCallback> callbacks;

LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, void* userdata) {
	if (uMsg == WM_COMMAND) {
		UINT_PTR id = LOWORD(wParam);

		for (MenuCallback callback : callbacks)
		{
			if (callback.id == id)
			{
				callback.callback(callback.id);
				break;
			}
		}
	}

	auto originalProc = (WNDPROC)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	return CallWindowProc(originalProc, hwnd, uMsg, wParam, lParam);
}

void InitGUI(SDL_Window* win)
{
	SDL_PropertiesID props = SDL_GetWindowProperties(win);

	hWnd = (HWND)SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);

	if (!hWnd)
	{
		std::cerr << "Failed to initialize Windows GUI." << std::endl;
		return;
	}

	hMenuBar = CreateMenu();

	SetMenu(hWnd, hMenuBar);

	SetWindowLongPtr(hWnd, GWLP_USERDATA, GetWindowLongPtr(hWnd, GWLP_WNDPROC));
	SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)WndProc);
}

MENU AddMenu(const char* label)
{
	HMENU hMenu = CreatePopupMenu();
	AppendMenuA(hMenuBar, MF_POPUP, (UINT_PTR)hMenu, label);

	return hMenu;
}

void AddMenuItem(MENU menu, MENU_ID id, const char* label, void (*handler)(MENU_ID caller))
{
	AppendMenuA(menu, MF_STRING, id, label);

	callbacks.push_back({ id, handler });
}

MENU AddSubMenu(MENU menu, const char* label)
{
	HMENU hMenu = CreatePopupMenu();
	AppendMenuA(menu, MF_POPUP, (UINT_PTR)hMenu, label);

	return hMenu;
}

void RefreshMenuBar()
{
	DrawMenuBar(hWnd);
}


void Alert(const char* title, const char* message, void (*handler)(MENU_ID caller))
{
	MessageBoxA(hWnd, message, title, MB_OK);

	handler(NULL);
}

#else

#define MENU_ID int
#define MENU int

void InitGUI(SDL_Window* win)
{

}

MENU AddMenu(const char* label)
{
	HMENU hMenu = 0;

	return hMenu;
}

void AddMenuItem(MENU menu, MENU_ID id, const char* label, void (*handler)())
{

}

void RefreshMenuBar()
{

}


void Alert(const char* title, const char* message, void (*handler)())
{

}

#endif