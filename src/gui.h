#pragma once

#define QTGUI

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#endif

#ifndef QTGUI
#define MENU_ID UINT_PTR
#define MENU HMENU
#define MENUBAR HMENU

struct Window
{
	SDL_Window* window;
};
#else
#include <QApplication>
#include <QMainWindow>
#include <QWindow>
#include <QWidget>
#include <QMenuBar>
#include <QAction>
#include <QVBoxLayout>
#include <QKeyEvent>
#include <QMessageBox>
#define MENU_ID unsigned int
#define MENU QMenu*
#define MENUBAR QMenuBar*

struct Window
{
	SDL_WindowID window;
	QMainWindow* qtWindow;
	QWidget* qtWidget;
};

std::vector<Window> windows;

struct MenuItem
{
	MENU_ID id;
	QAction* action;
	QMenu* parent;
};

std::vector<MenuItem> items;
#endif

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

#ifndef QTGUI

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

MENUBAR CreateMenuBar(SDL_Window* win)
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

void ResizeMenuWindow(SDL_Window* win, int w, int h)
{
	SDL_SetWindowSize(win, w, h);
}

MENU AddMenu(MENUBAR menuBar, const char* label)
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

#else

class KeyboardForwarder : public QObject {
protected:
	bool eventFilter(QObject* obj, QEvent* event) override {
		if (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease) {
			QKeyEvent* keyEvent = (QKeyEvent*)(event);
			quint32 nativeVKey;

			SDL_Scancode scancode = SDL_SCANCODE_UNKNOWN;
			switch (keyEvent->key()) {
			case Qt::Key_1:     scancode = SDL_SCANCODE_1; break;
			case Qt::Key_2:     scancode = SDL_SCANCODE_2; break;
			case Qt::Key_3:     scancode = SDL_SCANCODE_3; break;
			case Qt::Key_4:     scancode = SDL_SCANCODE_4; break;
			case Qt::Key_5:     scancode = SDL_SCANCODE_5; break;
			case Qt::Key_6:     scancode = SDL_SCANCODE_6; break;
			case Qt::Key_7:     scancode = SDL_SCANCODE_7; break;
			case Qt::Key_8:     scancode = SDL_SCANCODE_8; break;
			case Qt::Key_9:     scancode = SDL_SCANCODE_9; break;
			case Qt::Key_0:     scancode = SDL_SCANCODE_0; break;

			case Qt::Key_X:     scancode = SDL_SCANCODE_X; break;
			case Qt::Key_Z:     scancode = SDL_SCANCODE_Z; break;
			case Qt::Key_P:     scancode = SDL_SCANCODE_P; break;
			case Qt::Key_R:     scancode = SDL_SCANCODE_R; break;

			case Qt::Key_Up:    scancode = SDL_SCANCODE_UP; break;
			case Qt::Key_Down:  scancode = SDL_SCANCODE_DOWN; break;
			case Qt::Key_Left:  scancode = SDL_SCANCODE_LEFT; break;
			case Qt::Key_Right: scancode = SDL_SCANCODE_RIGHT; break;

			case Qt::Key_Shift:
#ifdef Q_OS_WIN
				nativeVKey = keyEvent->nativeVirtualKey();

				if (nativeVKey == VK_LSHIFT) {
					scancode = SDL_SCANCODE_LSHIFT;
				}
				else if (nativeVKey == VK_RSHIFT) {
					scancode = SDL_SCANCODE_RSHIFT;
				}
#else
				scancode = SDL_SCANCODE_RSHIFT;
#endif
				break;
			case Qt::Key_Control:
#ifdef Q_OS_WIN
				nativeVKey = keyEvent->nativeVirtualKey();

				if (nativeVKey == VK_LCONTROL) {
					scancode = SDL_SCANCODE_LCTRL;
				}
				else if (nativeVKey == VK_RCONTROL) {
					scancode = SDL_SCANCODE_RCTRL;
				}
#else
				scancode = SDL_SCANCODE_LCTRL;
#endif
				break;

			case Qt::Key_Return:scancode = SDL_SCANCODE_RETURN; break;
			}

			if (scancode != SDL_SCANCODE_UNKNOWN) {
				SDL_Event sdlEvent;
				SDL_zero(sdlEvent);

				sdlEvent.type = (event->type() == QEvent::KeyPress) ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
				sdlEvent.key.scancode = scancode;
				sdlEvent.key.key = SDL_GetKeyFromScancode(scancode, SDL_KMOD_NONE, true);
				sdlEvent.key.down = (event->type() == QEvent::KeyPress);

				SDL_PushEvent(&sdlEvent);
			}

			return true;
		}
		return QObject::eventFilter(obj, event);
	}
};

class CloseFilter : public QObject {
	SDL_WindowID id;
	QMainWindow* win;

public:
	CloseFilter(SDL_WindowID winID, QMainWindow* qtWin)
	{
		id = winID;
		win = qtWin;
	};

protected:
	bool eventFilter(QObject* obj, QEvent* event) override {
		if (event->type() == QEvent::Close) {
			SDL_Event quitEvent;
			SDL_zero(quitEvent);

			SDL_WindowEvent winEvent;
			SDL_zero(winEvent);
			winEvent.type = SDL_EVENT_WINDOW_CLOSE_REQUESTED;
			winEvent.windowID = id;

			quitEvent.window = winEvent;
			quitEvent.type = SDL_EVENT_WINDOW_CLOSE_REQUESTED;
			SDL_PushEvent(&quitEvent);

			win->hide();

			return true;
		}
		return QObject::eventFilter(obj, event);
	}
};

MENUBAR CreateMenuBar(SDL_Window* win)
{
	SDL_PropertiesID props = SDL_GetWindowProperties(win);

	const char* name = SDL_GetCurrentVideoDriver();

	WId windowID = NULL;

	if (SDL_strcmp(name, "windows") == 0)
	{
		printf("Attempt to load windows window ID\n");
		windowID = (WId)SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
	}
	else if (SDL_strcmp(name, "x11") == 0)
	{
		printf("Attempt to load x11 window ID\n");
		windowID = (WId)SDL_GetNumberProperty(props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, NULL);
	}
	else if(SDL_strcmp(name, "cocoa") == 0)
	{
		printf("Attempt to load cocoa window ID\n");
		windowID = (WId)SDL_GetPointerProperty(props, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, NULL);
	}
	
	if (windowID == NULL) throw std::runtime_error("Unsupported video driver: " + std::string(name));

	QWindow* qtWindow = QWindow::fromWinId(windowID);
	QWidget* qtWidget = QWidget::createWindowContainer(qtWindow);
	qtWidget->setFocusPolicy(Qt::StrongFocus);
	qtWindow->requestActivate();

	QMainWindow* qtMainWindow = new QMainWindow(NULL);
	QWidget* qtMainWidget = new QWidget(qtMainWindow);

	QVBoxLayout* layout = new QVBoxLayout(qtMainWidget);
	layout->addWidget(qtWidget);
	layout->setContentsMargins(0, 0, 0, 0);

	qtMainWindow->setCentralWidget(qtMainWidget);
	qtMainWindow->show();

	qtMainWindow->setWindowTitle(SDL_GetWindowTitle(win));

	CloseFilter* closeFilter = new CloseFilter(SDL_GetWindowID(win), qtMainWindow);
	qtMainWindow->installEventFilter(closeFilter);

	KeyboardForwarder* keyFilter = new KeyboardForwarder();
	qtMainWindow->installEventFilter(keyFilter);

	MENUBAR menuBar = qtMainWindow->menuBar();

	windows.push_back({ SDL_GetWindowID(win), qtMainWindow, qtMainWidget });

	return menuBar;
}

void ResizeMenuWindow(SDL_Window* win, int w, int h)
{
	for (Window window : windows)
	{
		if (window.window == SDL_GetWindowID(win))
		{
			window.qtWidget->setMinimumSize(w, h);
			window.qtWindow->adjustSize();
			window.qtWindow->setFixedSize(window.qtWindow->size());
		}
	}
}

MENU AddMenu(MENUBAR menuBar, const char* label)
{
	QMenu* menu = menuBar->addMenu(label);

	return menu;
}

bool GetMenuItemChecked(MENU menu, MENU_ID id)
{
	for (MenuItem item : items)
	{
		if (item.parent == menu && item.id == id)
		{
			return item.action->property("checked").toBool();
		}
	}

	return false;
}

void SetMenuItemChecked(MENU menu, MENU_ID id, bool checked)
{
	for (MenuItem item : items)
	{
		if (item.parent == menu && item.id == id)
		{
			item.action->blockSignals(true);
			item.action->setChecked(checked);
			item.action->blockSignals(false);

			item.action->setProperty("checked", checked);
		}
	}
}

void HandleDropdown(MENU_ID id)
{
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
				SetMenuItemChecked(dropdown.parent, dropdown.menus[i], dropdown.menus[i] == id);
			}

			dropdown.callback(id);

			break;
		}
	}
}

void AddMenuItem(MENU menu, MENU_ID id, const char* label, void (*handler)(MENU_ID caller))
{
	QAction* action = menu->addAction(label);
	action->setCheckable(true);

	items.push_back({ id, action, menu });

	QObject::connect(action, &QAction::triggered, [action, id, handler]() {
		action->blockSignals(true);
		action->setChecked(false);
		action->blockSignals(false);
		handler(id);
		HandleDropdown(id);
	});
}

void AddMenuItem(MENU menu, MENU_ID id, const char* label)
{
	QAction* action = menu->addAction(label);
	action->setCheckable(true);
	action->setProperty("checked", false);

	items.push_back({ id, action, menu });

	QObject::connect(action, &QAction::triggered, [action, id]() {
		action->blockSignals(true);
		action->setChecked(false);
		action->blockSignals(false);

		HandleDropdown(id);
	});
}

MENU AddSubMenu(MENU menu, const char* label)
{
	QMenu* subMenu = menu->addMenu(label);

	return subMenu;
}

void Alert(SDL_Window* win, const char* title, const char* message, void (*handler)(MENU_ID caller))
{
	for (Window window : windows)
	{
		if (window.window == SDL_GetWindowID(win))
		{
			QMessageBox::information(window.qtWidget, title, message);
		}
	}

	handler(NULL);
}

#endif

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