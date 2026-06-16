#pragma once

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#define MENU_ID UINT_PTR
#define MENU HMENU

#define MENU_CALLBACK(name) static void name(MENU_ID caller)

struct MenuCallback
{
	MENU_ID id;
	void (*callback)(MENU_ID caller);
};

struct DropdownSelect
{
	MENU parent;
	MENU_ID* menus;
	int menuCount;
	void (*callback)(MENU_ID caller);
};

std::vector<MenuCallback> callbacks;
std::vector<DropdownSelect> dropdowns;

LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, void* userdata) {
	if (uMsg == WM_COMMAND) {
		UINT_PTR id = LOWORD(wParam);

		for (MenuCallback callback : callbacks)
		{
			if (callback.id == id)
			{
				callback.callback(id);
				break;
			}
		}

		for (DropdownSelect dropdown : dropdowns)
		{
			bool found = false;

			for (int i = 0; i < dropdown.menuCount; i++)
			{
				if (dropdown.menus[i] == id)
				{
					found = true;
					break;
				}
			}

			if (found)
			{
				for (int i = 0; i < dropdown.menuCount; i++)
				{
					CheckMenuItem(dropdown.parent, dropdown.menus[i], MF_BYCOMMAND | ((dropdown.menus[i] == id) ? MF_CHECKED : MF_UNCHECKED));
				}

				dropdown.callback(id);

				break;
			}
		}
	}

	auto originalProc = (WNDPROC)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	return CallWindowProc(originalProc, hwnd, uMsg, wParam, lParam);
}

MENU CreateMenuBar(SDL_Window* win)
{
	SDL_PropertiesID props = SDL_GetWindowProperties(win);

	HWND hWnd = (HWND)SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);

	if (!hWnd)
	{
		std::cerr << "Failed to initialize menu bar." << std::endl;
		return NULL;
	}

	MENU hMenuBar = CreateMenu();

	SetMenu(hWnd, hMenuBar);

	SetWindowLongPtr(hWnd, GWLP_USERDATA, GetWindowLongPtr(hWnd, GWLP_WNDPROC));
	SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)WndProc);

	DrawMenuBar(hWnd);

	return hMenuBar;
}

MENU AddMenu(MENU menuBar, const char* label)
{
	HMENU hMenu = CreatePopupMenu();
	AppendMenuA(menuBar, MF_POPUP, (UINT_PTR)hMenu, label);

	return hMenu;
}

void AddMenuItem(MENU menu, MENU_ID id, const char* label, void (*handler)(MENU_ID caller))
{
	AppendMenuA(menu, MF_STRING, id, label);

	callbacks.push_back({ id, handler });
}

void AddMenuItem(MENU menu, MENU_ID id, const char* label)
{
	AppendMenuA(menu, MF_STRING, id, label);
}

MENU AddSubMenu(MENU menu, const char* label)
{
	HMENU hMenu = CreatePopupMenu();
	AppendMenuA(menu, MF_POPUP, (UINT_PTR)hMenu, label);

	return hMenu;
}

void Alert(SDL_Window* win, const char* title, const char* message, void (*handler)(MENU_ID caller))
{
	SDL_PropertiesID props = SDL_GetWindowProperties(win);
	HWND hWnd = (HWND)SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);

	MessageBoxA(hWnd, message, title, MB_OK);

	handler(NULL);
}

bool GetMenuItemChecked(MENU menu, MENU_ID id)
{
	UINT state = GetMenuState(menu, id, MF_BYCOMMAND);

	return !!(state & MF_CHECKED);
}

void SetMenuItemChecked(MENU menu, MENU_ID id, bool checked)
{
	CheckMenuItem(menu, id, MF_BYCOMMAND | (checked ? MF_CHECKED : MF_UNCHECKED));
}

void CreateDropdownSelectHandler(DropdownSelect dropdown)
{
	dropdowns.push_back(dropdown);
}

MENU_ID GetDropdownSelected(DropdownSelect dropdown)
{
	for (int i = 0; i < dropdown.menuCount; i++)
	{
		if (GetMenuItemChecked(dropdown.parent, dropdown.menus[i]))
			return dropdown.menus[i];
	}

	return NULL;
}

void SetDropdownSelected(DropdownSelect dropdown, MENU_ID id)
{
	for (int i = 0; i < dropdown.menuCount; i++)
	{
		SetMenuItemChecked(dropdown.parent, dropdown.menus[i], dropdown.menus[i] == id);
	}
}


int GetDropdownSelectedIndex(DropdownSelect dropdown)
{
	for (int i = 0; i < dropdown.menuCount; i++)
	{
		if (GetMenuItemChecked(dropdown.parent, dropdown.menus[i]))
			return i;
	}

	return -1;
}

void SetDropdownSelectedIndex(DropdownSelect dropdown, MENU_ID idx)
{
	for (int i = 0; i < dropdown.menuCount; i++)
	{
		SetMenuItemChecked(dropdown.parent, dropdown.menus[i], i == idx);
	}
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