#include "windowing.h"

WCHAR CheatPassword[] = L"XYZZY";

DifficultyConfigItem DifficultyConfigTable[] = {
	{ BEGINNER_MINES, BEGINEER_HEIGHT, BEGINNER_WIDTH },
	{ INTERMIDIATE_MINES, INTERMIDIATE_HEIGHT, INTERMIDIATE_WIDTH },
	{ EXPERT_MINES, EXPERT_HEIGHT, EXPERT_WIDTH }
};

HelpEntry customFieldHelpData[] = {
	{ ID_DIALOG_CUSTOM_FIELD_HEIGHT, 1000 },
	{ ID_DIALOG_CUSTOM_FIELD_WIDTH, 1001 },
	{ ID_DIALOG_CUSTOM_FIELD_MINES, 1002 },
	{ ID_DIALOG_CUSTOM_FIELD_HEIGHT_LABEL, 1000 },
	{ ID_DIALOG_CUSTOM_FIELD_WIDTH_LABEL, 1001 },
	{ ID_DIALOG_CUSTOM_FIELD_MINES_LABEL, 1002 },
	{ 0, 0 }
};

HelpEntry winnersHelpData[] = {
	{ ID_DIALOG_WINNERS_BTN_RESET_SCORES, 1003 },
	{ 709, 1004 },
	{ 710, 1004 },
	{ 701, 1004 },
	{ 703, 1004 },
	{ 705, 1004 },
	{ 702, 1004 },
	{ 704, 1004 },
	{ 706, 1004 }
};

DWORD CheatPasswordIndex;

BOOL HasMouseCapture;
BOOL Is3x3Click;
BOOL IgnoreSingleClick;
BOOL IsMenuOpen;

// Number of pixels in the window
int WindowWidthInPixels;
int WindowHeightInPixels;
int ScreenHeightInPixels;
int WindowHeightIncludingMenu;
int MenuBarHeightInPixels;

// .data: 01005AA0
WCHAR ClassName[32];

HMENU hMenu;

int yBottom = 0;
int xRight = 0;

HWND hWnd;
HICON hIcon;


HINSTANCE hModule;
// This variable is used only in WinMain
// To check if the window is minimized use "StateFlags" with 
//		STATE_WINDOW_MINIMIZED or STATE_WINDOW_MINIMIZED2
BOOL Minimized;


BOOL HandleLeftClick(DWORD dwLocation) {
	MSG msg;
	RECT rect;

	// Copy point into struct
	msg.pt.y = HIWORD(dwLocation);
	msg.pt.x = LOWORD(dwLocation);

	// The rect represents the game board
	rect.left = (xRight - 24) / 2;
	rect.right = rect.left + 24;
	rect.top = 16;
	rect.bottom = 40;

	// Check if the click is in the range of the board
	if (!PtInRect(&rect, msg.pt)) {
		return FALSE;
	}

	// Capture mouse events outside the window 
	//   - to capture MOUSEMOVE and BOTTONUP events 
	SetCapture(hWnd);
	DisplaySmile(SMILE_CLICKED);


	// Convert window relative points to screen relative points
	MapWindowPoints(
		hWnd, // hWndFrom: Game window
		NULL, // hWndTo: Desktop screen
		(LPPOINT)&rect, // lpPoints - 2 points: (left, top), (right, bottom)
		2 // cPoints
	);

	int ebx = 0;

	while (TRUE) {
		// Wait for a message of this kind
		// Handle WM_MOUSEMOVE, WM_BUTTONUP events
		while (!PeekMessageW(&msg, hWnd, WM_MOUSEMOVE, WM_XBUTTONDBLCLK, TRUE)) {
		}

		switch (msg.message) {
		case WM_MOUSEMOVE:
			if (PtInRect(&rect, msg.pt) && ebx != 0) {
				DisplaySmile(GlobalSmileId);
				ebx = 0;
			}
			else if (ebx == 0) {
				ebx++;
				DisplaySmile(SMILE_NORMAL);
			}

			break;

		case WM_LBUTTONUP:
			if (ebx != 0 && PtInRect(&rect, msg.pt)) {
				GlobalSmileId = SMILE_NORMAL;
				DisplaySmile(SMILE_NORMAL);
				InitializeNewGame();
			}

			ReleaseCapture();
			return TRUE;
		default:
			if (ebx == 0) {
				ebx++;
				DisplaySmile(SMILE_CLICKED);
			}
			break;
		}
	}
}


void InitializeCheckedMenuItems() {
	// Set Difficulty Level Checkbox
	SetMenuItemState(ID_MENUITEM_BEGINNER, Difficulty_InitFile == DIFFICULTY_BEGINNER);
	SetMenuItemState(ID_MENUITEM_INTERMEDIATE, Difficulty_InitFile == DIFFICULTY_INTERMEDIATE);
	SetMenuItemState(ID_MENUITEM_EXPERT, Difficulty_InitFile == DIFFICULTY_EXPERT);
	SetMenuItemState(ID_MENUITEM_CUSTOM, Difficulty_InitFile == DIFFICULTY_CUSTOM);

	// Set Another Flags
	SetMenuItemState(ID_MENUITEM_COLOR, Color_InitFile);
	SetMenuItemState(ID_MENUITEM_MARKS, Mark_InitFile);
	SetMenuItemState(ID_MENUITEM_SOUND, Sound_InitFile);
}

INT_PTR CALLBACK CustomFieldDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_HELP:
	{
		HELPINFO* pHelpInfo = (HELPINFO*)lParam;
		WinHelpW((HWND)pHelpInfo->hItemHandle, winmineHelp, HELP_WM_HELP, (ULONG_PTR)&winnersHelpData);
		return FALSE;
	}
	case WM_CONTEXTMENU:
	{
		WinHelpW((HWND)wParam, winmineHelp, HELP_CONTEXTMENU, (ULONG_PTR)&winnersHelpData);
		return FALSE;
	}
	case WM_INITDIALOG:
	{
		SetDlgItemInt(hDlg, ID_DIALOG_CUSTOM_FIELD_HEIGHT, Height_InitFile, FALSE);
		SetDlgItemInt(hDlg, ID_DIALOG_CUSTOM_FIELD_WIDTH, Width_InitFile, FALSE);
		SetDlgItemInt(hDlg, ID_DIALOG_CUSTOM_FIELD_MINES, Mines_InitFile, FALSE);
		return TRUE;
	}
	case WM_COMMAND:
	{
		switch (LOWORD(wParam)) {
		case 1:
		case 100:
		{
			Height_InitFile = GetDlgIntOfRange(hDlg, ID_DIALOG_CUSTOM_FIELD_HEIGHT, 9, 24);
			Width_InitFile = GetDlgIntOfRange(hDlg, ID_DIALOG_CUSTOM_FIELD_WIDTH, 9, 30);
			int maxMines = min((Height_InitFile - 1) * (Width_InitFile - 1), 999);
			Mines_InitFile = GetDlgIntOfRange(hDlg, ID_DIALOG_CUSTOM_FIELD_MINES, 10, maxMines);
		}
		break;
		case 2:
			break;
		default:
			return FALSE;
		}

		EndDialog(hDlg, TRUE);
		return TRUE;
	}
	}

	return FALSE;
}


void SaveWinnerNameDialogBox() {
	DialogBoxParamW(hModule, (LPCWSTR)ID_DIALOG_SAVE_NAME, hWnd, SaveWinnerNameDialogProc, 0);
	NeedToSaveConfigToRegistry = TRUE;
}


INT_PTR CALLBACK SaveWinnerNameDialogProc(HWND hDialog, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	WCHAR winMsg[128];

	if (uMsg == WM_INITDIALOG) {
		// Load you have the fastest time message
		LoadResourceString(Difficulty_InitFile + 9, winMsg, _countof(winMsg));

		// Show the message on the 
		SetDlgItemTextW(hDialog, ID_DIALOG_SAVE_NAME_CONTROL_MSG, winMsg);

		SendMessageW(GetDlgItem(hDialog, ID_DIALOG_SAVE_NAME),
			EM_SETLIMITTEXT, 32, 0);

		PWSTR pWinnerNamePtr = NULL;

		switch (Difficulty_InitFile) {
		case DIFFICULTY_BEGINNER:
			pWinnerNamePtr = Name1_InitFile;
			break;
		case DIFFICULTY_INTERMEDIATE:
			pWinnerNamePtr = Name2_InitFile;
			break;
		default:
			pWinnerNamePtr = Name3_InitFile;
			break;
		}

		SetDlgItemTextW(hDialog, ID_DIALOG_SAVE_NAME_CONTROL_NAME, pWinnerNamePtr);
		return TRUE;

	}
	else if (uMsg == WM_COMMAND && wParam > 0 &&
		(wParam <= 2 || wParam == 100 || wParam == 109)) {

		PWSTR pWinnerNamePtr = NULL;

		switch (Difficulty_InitFile) {
		case DIFFICULTY_BEGINNER:
			pWinnerNamePtr = Name1_InitFile;
			break;
		case DIFFICULTY_INTERMEDIATE:
			pWinnerNamePtr = Name2_InitFile;
			break;
		default:
			pWinnerNamePtr = Name3_InitFile;
			break;
		}

		GetDlgItemTextW(hDialog, ID_DIALOG_SAVE_NAME_CONTROL_NAME, pWinnerNamePtr, 32);
		EndDialog(hDialog, 1);
		return TRUE;
	}

	return FALSE;
}


INT_PTR CALLBACK WinnersDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_HELP:
	{
		HELPINFO* pHelpInfo = (HELPINFO*)lParam;
		WinHelpW((HWND)pHelpInfo->hItemHandle, winmineHelp, HELP_WM_HELP, (ULONG_PTR)winnersHelpData);
		return FALSE;
	}
	case WM_CONTEXTMENU:
	{
		HELPINFO* pHelpInfo = (HELPINFO*)lParam;
		WinHelpW((HWND)wParam, winmineHelp, HELP_CONTEXTMENU, (ULONG_PTR)winnersHelpData);
		return FALSE;
	}
	case WM_INITDIALOG:
		break;
	case WM_COMMAND:
	{
		DWORD msg = LOWORD(wParam);

		if (msg <= 0) {
			return FALSE;
		}

		if (msg <= 2 || msg == 100 || msg == 109) {
			EndDialog(hDlg, 1);
			return FALSE;
		}

		if (msg == ID_DIALOG_WINNERS_BTN_RESET_SCORES) {
			Time_InitFile[0] = 999;
			Time_InitFile[1] = 999;
			Time_InitFile[2] = 999;

			lstrcpyW(Name1_InitFile, AnonymousStr);
			lstrcpyW(Name2_InitFile, AnonymousStr);
			lstrcpyW(Name3_InitFile, AnonymousStr);

			NeedToSaveConfigToRegistry = TRUE;
		}
	}
	default:
		return FALSE;
	}


	ShowWinnerNameAndTime(hDlg, ID_DIALOG_WINNERS_TIME_BEGINNER, Time_InitFile[0], Name1_InitFile);
	ShowWinnerNameAndTime(hDlg, ID_DIALOG_WINNERS_TIME_INTERMIDIATE, Time_InitFile[1], Name2_InitFile);
	ShowWinnerNameAndTime(hDlg, ID_DIALOG_WINNERS_TIME_EXPERT, Time_InitFile[2], Name3_InitFile);
	return TRUE;
}

void ShowWinnerNameAndTime(HWND hDlg, int nIDDlgItem, int secondsLeft, LPCWSTR lpName) {
	WCHAR lpSecondsLeftFormatted[30];
	wsprintfW(SecondsLeftBuffer, lpSecondsLeftFormatted, secondsLeft);
	SetDlgItemTextW(hDlg, nIDDlgItem, lpSecondsLeftFormatted);
	SetDlgItemTextW(hDlg, nIDDlgItem + 1, lpName);
}

void WinnersDialogBox() {
	DialogBoxParamW(hModule, (LPCWSTR)ID_DIALOG_WINNERS,
		hWnd, WinnersDialogProc, (LPARAM)NULL);
}


void InitializeWindowBorder(DWORD flags) {
	BOOL differentCordsForMenus = FALSE;
	RECT rcGameMenu;
	RECT rcHelpMenu;

	if (hWnd == NULL) {
		return;
	}

	WindowHeightIncludingMenu = ScreenHeightInPixels;

	if (Menu_InitFile & 1) {
		WindowHeightIncludingMenu += MenuBarHeightInPixels;


		if (hMenu != NULL &&
			GetMenuItemRect(hWnd, hMenu, GAME_MENU_INDEX, &rcGameMenu) &&
			GetMenuItemRect(hWnd, hMenu, HELP_MENU_INDEX, &rcHelpMenu) &&
			rcGameMenu.top != rcHelpMenu.top) {
			differentCordsForMenus = TRUE;
			// Add it twice
			WindowHeightIncludingMenu += MenuBarHeightInPixels;
		}
	}

	xRight = (Width_InitFile2 * 16) + 24;   // 24 is the size of pixels in the side of the window, 16 is the size of block
	yBottom = (Height_InitFile2 * 16) + 67; // 16 is the size of the pixels in a block, 67 is the size of pixels in the sides

											// Check If The Place On The Screen Overflows The End Of The Screen
											// If it is, Move The Window To A New Place
	int diffFromEnd;

	// If diffFromEnd is negative, it means the window does not overflow the screen
	// If diffFromEnd is positive, it means the window overflows the screen and needs to be moved
	diffFromEnd = (xRight + Xpos_InitFile) - SimpleGetSystemMetrics(GET_SCREEN_WIDTH);

	if (diffFromEnd > 0) {
		flags |= MOVE_WINDOW;
		Xpos_InitFile -= diffFromEnd;
	}

	diffFromEnd = (yBottom + Ypos_InitFile) - SimpleGetSystemMetrics(GET_SCREEN_HEIGHT);

	if (diffFromEnd > 0) {
		flags |= MOVE_WINDOW;
		Ypos_InitFile -= diffFromEnd;
	}

	if (Minimized) {
		return;
	}

	if (flags & MOVE_WINDOW) {
		MoveWindow(hWnd,
			Xpos_InitFile, Ypos_InitFile,
			WindowWidthInPixels + xRight,
			yBottom + WindowHeightIncludingMenu,
			TRUE);
	}

	if (differentCordsForMenus &&
		hMenu != NULL &&
		GetMenuItemRect(hWnd, hMenu, 0, &rcGameMenu) &&
		GetMenuItemRect(hWnd, hMenu, 1, &rcHelpMenu) &&
		rcGameMenu.top == rcHelpMenu.top) {
		WindowHeightIncludingMenu -= MenuBarHeightInPixels;

		MoveWindow(hWnd, Xpos_InitFile, Ypos_InitFile, WindowWidthInPixels + xRight, yBottom + WindowHeightIncludingMenu, TRUE);
	}

	if (flags & REPAINT_WINDOW) {
		RECT rc;
		SetRect(&rc, 0, 0, xRight, yBottom);
		// Cause a Repaint of the whole window
		InvalidateRect(hWnd, &rc, TRUE);
	}


}


DWORD SimpleGetSystemMetrics(DWORD val) {
	DWORD result;

	switch (val) {
	case GET_SCREEN_WIDTH:
		result = GetSystemMetrics(SM_CXVIRTUALSCREEN);

		if (result == 0) {
			result = GetSystemMetrics(SM_CXSCREEN);
		}
		break;
	case GET_SCREEN_HEIGHT:
		result = GetSystemMetrics(SM_CYVIRTUALSCREEN);

		if (result == 0) {
			result = GetSystemMetrics(SM_CYSCREEN);
		}
		break;
	default:
		result = GetSystemMetrics(val);
	}

	return result;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	// uMsg is between 0x201 to 0x212 (inclusive)
	switch (uMsg) {
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MBUTTONUP:
		if (HasMouseCapture) {
			ReleaseMouseCapture();
		}
		break;
	case WM_ENTERMENULOOP:
		IsMenuOpen = TRUE;
		break;
	case WM_EXITMENULOOP:
		IsMenuOpen = FALSE;
		break;
	case WM_MBUTTONDOWN:
		if (IgnoreSingleClick) {
			IgnoreSingleClick = FALSE;
			return FALSE;
		}

		if ((StateFlags & STATE_GAME_IS_ON) == 0) {
			Is3x3Click = TRUE;
			return CaptureMouseInput(hwnd, uMsg, wParam, lParam);
		}
		break;
	case WM_RBUTTONDOWN:
		if (IgnoreSingleClick) {
			IgnoreSingleClick = FALSE;
			return FALSE;
		}

		if ((StateFlags & STATE_GAME_IS_ON) == 0) {
			break;
		}

		if (HasMouseCapture) {
			ReleaseBlocksClick();
			Is3x3Click = TRUE;
			PostMessageW(hWnd, WM_MOUSEMOVE, wParam, lParam);
		}
		else if (wParam & 1) {
			CaptureMouseInput(hwnd, uMsg, wParam, lParam);
		}
		else if (!IsMenuOpen) {

			HandleRightClick((BoardPoint) {
				.Column = (LOWORD(lParam) + 4) / 16,
					.Row = (HIWORD(lParam) - 39) / 16
			});
		}

		return FALSE;

	case WM_LBUTTONDOWN:
		if (IgnoreSingleClick) {
			IgnoreSingleClick = FALSE;
			return FALSE;
		}
		if (HandleLeftClick((DWORD)lParam)) {
			return 0;
		}

		if ((StateFlags & STATE_GAME_IS_ON) == 0) {
			break;
		}

		Is3x3Click = (wParam & 6) ? TRUE : FALSE;
		return CaptureMouseInput(hwnd, uMsg, wParam, lParam);

	case WM_MOUSEMOVE:
		return MouseMoveHandler(hwnd, uMsg, wParam, lParam);

	case WM_KEYDOWN:
		KeyDownHandler(wParam);
		break;

	case WM_COMMAND:
		if (!MenuHandler(LOWORD(wParam))) {
			return FALSE;
		}

		break;

	case WM_SYSCOMMAND:

		switch (GET_SC_WPARAM(wParam)) {
		case SC_MINIMIZE:
			NotifyMinimize();
			// Maybe add minimize flags..
			StateFlags |= (STATE_WINDOW_MINIMIZED | STATE_WINDOW_MINIMIZED_2);
			break;
		case SC_RESTORE:
			// Clean those bits from earlier
			StateFlags &= ~(STATE_WINDOW_MINIMIZED | STATE_WINDOW_MINIMIZED_2);
			NotifyWindowRestore();
			IgnoreSingleClick = FALSE;
			break;
		}

		break;

	case WM_DESTROY:
		KillTimer(hWnd, TIMER_ID);
		PostQuitMessage(0);
		break;
	case WM_ACTIVATE:
		if (wParam == WA_CLICKACTIVE) {
			IgnoreSingleClick = TRUE;
		}
		break;
	case WM_WINDOWPOSCHANGED:
		if ((StateFlags & STATE_WINDOW_MINIMIZED_2) == 0) {
			WINDOWPOS* pos = (WINDOWPOS*)lParam;
			Xpos_InitFile = pos->x;
			Ypos_InitFile = pos->y;
		}
		break;
	case WM_PAINT:
	{
		PAINTSTRUCT paint;
		HDC hDC = BeginPaint(hWnd, &paint);
		RedrawUIOnDC(hDC);
		EndPaint(hWnd, &paint);
		return FALSE;
	}
	case WM_TIMER:
		TickSeconds();
		return FALSE;

	}

	return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

__inline BOOL MenuHandler(WORD menuItem) {

	if (menuItem >= ID_MENUITEM_BEGINNER && menuItem <= ID_MENUITEM_EXPERT) {
		Difficulty_InitFile = LOWORD(menuItem - ID_MENUITEM_BEGINNER);

		Mines_InitFile = DifficultyConfigTable[Difficulty_InitFile].Mines;
		Height_InitFile = DifficultyConfigTable[Difficulty_InitFile].Height;
		Width_InitFile = DifficultyConfigTable[Difficulty_InitFile].Width;
		InitializeNewGame();
		NeedToSaveConfigToRegistry = TRUE;
		InitializeMenu(Menu_InitFile);
		return TRUE;
	}

	switch (menuItem) {
	case ID_MENUITEM_COLOR:
		// Toogle Color
		Color_InitFile = !Color_InitFile;
		FreePenAndBlocks();

		if (LoadBitmaps()) {
			RedrawUI();
			NeedToSaveConfigToRegistry = TRUE;
			InitializeMenu(Menu_InitFile);
		}
		else {
			DisplayErrorMessage(ID_OUT_OF_MEM);

			// Send a close message
			SendMessageW(hWnd, WM_SYSCOMMAND, SC_CLOSE, 0);
			return FALSE;
		}
		break;
	case ID_MENUITEM_BEST_TIMES:
		WinnersDialogBox();
		break;
	case ID_MENUITEM_NEWGAME:
		InitializeNewGame();
		break;
	case ID_MENUITEM_CUSTOM:
		CustomFieldDialogBox();
		break;
	case ID_MENUITEM_SEARCH_FOR_HELP:
		ShowHelpHtml(1, 2);
		break;
	case ID_MENUITEM_USING_HELP:
		ShowHelpHtml(4, 0);
		break;
	case ID_MENUITEM_ABOUT:
		ShowAboutWindow();
		return FALSE;
		break;
	case ID_MENUITEM_CONTENTS:
		ShowHelpHtml(3, 0);
		break;
	case ID_MENUITEM_MARKS:
		Mark_InitFile = (Mark_InitFile) ? FALSE : TRUE;
		NeedToSaveConfigToRegistry = TRUE;
		InitializeMenu(Menu_InitFile);
		break;
	case ID_MENUITEM_SOUND:

		if (Sound_InitFile) {
			FreeSound();
			Sound_InitFile = 0;
		}
		else {
			Sound_InitFile = StopAllSound();
		}
		NeedToSaveConfigToRegistry = TRUE;
		InitializeMenu(Menu_InitFile);
		break;
	case ID_MENUITEM_EXIT:
		ShowWindow(hWnd, FALSE);
		SendMessageW(hWnd, WM_SYSCOMMAND, SC_CLOSE, 0);
		break;
	default:
		break;
	}

	return TRUE;
}


__inline void KeyDownHandler(WPARAM wParam) {
	switch (wParam) {
	case VK_SHIFT:
		if (CheatPasswordIndex >= 5) {
			// flip these bits
			CheatPasswordIndex ^= 0x14; // 0b10100
		}
		break;
	case VK_F4:
		if (Sound_InitFile > 1) {
			if (Sound_InitFile == 3) {
				FreeSound();
				Sound_InitFile = 2;
			}
			else {
				Sound_InitFile = StopAllSound();
			}
		}
		break;
	case VK_F5:
		if (Menu_InitFile) {
			InitializeMenu(1);
		}
		break;
	case VK_F6:
		if (Menu_InitFile) {
			InitializeMenu(2);
		}
		break;
	default:
		if (CheatPasswordIndex < 5) {
			if (CheatPassword[CheatPasswordIndex] == LOWORD(wParam)) {
				CheatPasswordIndex++;
			}
			else {
				CheatPasswordIndex = 0;
			}
		}
	}
}

__inline LRESULT CaptureMouseInput(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	// Shared code...
	SetCapture(hWnd);
	ClickedBlock = (BoardPoint) { -1, -1 };
	HasMouseCapture = TRUE;
	DisplaySmile(SMILE_WOW);
	return MouseMoveHandler(hwnd, uMsg, wParam, lParam);
}

__inline LRESULT MouseMoveHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	// WM_MOUSEMOVE_Handler!
	if (!HasMouseCapture) {
		if (CheatPasswordIndex == 0) {
			return DefWindowProcW(hWnd, uMsg, wParam, lParam);
		}

		if (CheatPasswordIndex > 5 || (CheatPasswordIndex == 5 && wParam != MK_CONTROL)) {
			// Get Mouse Block
			ClickedBlock = (BoardPoint) {
				.Column = (LOWORD(lParam) + 4) / 16,
					.Row = (HIWORD(lParam) - 39) / 16
			};

			if (!IsInBoardRange(ClickedBlock)) {
				return DefWindowProcW(hWnd, uMsg, wParam, lParam);
			}

			HDC hDesktop = GetDC(NULL);
			BYTE block = ACCESS_BLOCK(ClickedBlock);
			SetPixel(hDesktop, 0, 0, (block & BLOCK_IS_BOMB) ? BLACK_COLOR : WHITE_COLOR);

			return FALSE;
		}
	}
	else if (!(StateFlags & STATE_GAME_IS_ON)) {
		ReleaseMouseCapture();
	}
	else {
		// Update Mouse Block
		UpdateClickedBlocksState((BoardPoint) {
			.Column = (LOWORD(lParam) + 4) / 16,
				.Row = (HIWORD(lParam) - 39) / 16
		});
	}

	return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	hModule = hInstance;
	InitMetricsAndFirstGame();

	if (nCmdShow == SW_SHOWMINNOACTIVE || nCmdShow == SW_SHOWMINIMIZED) {
		Minimized = TRUE;
	}
	else {
		Minimized = FALSE;
	}

	INITCOMMONCONTROLSEX picce;
	picce.dwSize = sizeof(INITCOMMONCONTROLSEX);
	picce.dwICC =
		ICC_LISTVIEW_CLASSES | ICC_BAR_CLASSES | ICC_TAB_CLASSES | ICC_UPDOWN_CLASS |
		ICC_PROGRESS_CLASS | ICC_HOTKEY_CLASS | ICC_ANIMATE_CLASS | ICC_USEREX_CLASSES |
		ICC_COOL_CLASSES | ICC_PAGESCROLLER_CLASS;

	InitCommonControlsEx(&picce);
	hIcon = LoadIconW(hModule, (LPCWSTR)ID_ICON_GROUP);

	WNDCLASSW cls;

	cls.style = 0;
	cls.lpfnWndProc = WindowProc;
	cls.cbClsExtra = 0;
	cls.cbWndExtra = 0;
	cls.hInstance = hInstance;
	cls.hIcon = hIcon;
	cls.hCursor = LoadCursorW(NULL, (LPWSTR)IDC_ARROW);
	cls.hbrBackground = (HBRUSH)GetStockObject(LTGRAY_BRUSH);
	cls.lpszMenuName = NULL;
	cls.lpszClassName = ClassName;

	if (!RegisterClassW(&cls)) {
		return 0;
	}

	hMenu = LoadMenuW(hModule, (LPCWSTR)ID_MENU);
	HACCEL hAccTable = LoadAcceleratorsW(hModule, (LPCWSTR)ID_ACCELERATORS);

	InitializeConfigFromRegistry();

	hWnd = CreateWindowExW(0,
		ClassName,
		ClassName,
		WS_MINIMIZEBOX | WS_SYSMENU | WS_DLGFRAME | WS_BORDER,
		Xpos_InitFile - WindowWidthInPixels,
		Ypos_InitFile - WindowHeightIncludingMenu,
		xRight + WindowWidthInPixels,
		yBottom + WindowHeightIncludingMenu,
		NULL,
		NULL,
		hModule,
		NULL
	);

	if (hWnd == NULL) {
		// (Error: %d, 1000)
		DisplayErrorMessage(1000);
		return 0;
	}

	// 1 does not do anything...
	InitializeWindowBorder(1);

	if (!InitializeBitmapsAndBlockArray()) {
		DisplayErrorMessage(ID_OUT_OF_MEM);
		return 0;
	}

	InitializeMenu(Menu_InitFile);
	InitializeNewGame();
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);
	Minimized = 0;
	MSG msg;

	while (GetMessageW(&msg, hWnd, 0, 0)) {
		if (TranslateAcceleratorW(hWnd, hAccTable, &msg) == 0) {
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
	}

	FreePenAndBlocksAndSound();

	if (NeedToSaveConfigToRegistry) {
		SaveConfigToRegistry();
	}

	return msg.wParam;
}

void CustomFieldDialogBox() {
	DialogBoxParamW(hModule, (LPCWSTR)ID_DIALOG_CUSTOM_FIELD, hWnd, CustomFieldDialogProc, 0);
	Difficulty_InitFile = 3;
	// WIERD: Unconditionally change the difficulty to 3...
	InitializeCheckedMenuItems();
	NeedToSaveConfigToRegistry = TRUE;
}

void InitializeMenu(DWORD menuFlags) {
	Menu_InitFile = menuFlags;
	InitializeCheckedMenuItems();
	SetMenu(hWnd, (Menu_InitFile & 1) ? NULL : hMenu);
	InitializeWindowBorder(MOVE_WINDOW);
}


void SetMenuItemState(DWORD uID, BOOL isChecked) {
	CheckMenuItem(hMenu, uID, isChecked ? MF_CHECKED : MF_BYCOMMAND);

}


int GetDlgIntOfRange(HWND hDlg, int nIDDlgItem, int min, int max) {

	int result = GetDlgItemInt(hDlg, nIDDlgItem, &nIDDlgItem, FALSE);

	if (result < min) {
		return min;
	}

	if (result > max) {
		return max;
	}

	return result;
}


