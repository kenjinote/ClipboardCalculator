#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#include <windows.h>
#include <wchar.h>
#include "resource.h"

TCHAR szClassName[] = TEXT("Window");

class calc
{
public:
	calc(LPWSTR lpszCalc, double* ans/*, bool* error*/)
		: g_pbuf(lpszCalc)
		, g_back(TK_NONE)
	{
		*ans = expr();
	}
private:
	enum TOKEN {
		TK_NONE = -1,
		TK_EOF = 0,
		TK_CONST,
		TK_PLUS,
		TK_MINUS,
		TK_MULT,
		TK_DIV,
		TK_LPAREN,
		TK_RPAREN
	};
	LPWSTR g_pbuf;
	double g_value;
	TOKEN g_back;
	double expr()
	{
		double val = term();
		for (;;)
		{
			const TOKEN token = get_token();
			switch (token)
			{
			case TK_PLUS:
				val += term();
				break;
			case TK_MINUS:
				val -= term();
				break;
			default:
				g_back = token;
				return val;
			}
		}
	}
	TOKEN get_token()
	{
		if (g_back >= 0)
		{
			const TOKEN ret = g_back;
			g_back = TK_NONE;
			return ret;
		}
		while (isspace(*g_pbuf))g_pbuf++;
		switch (*g_pbuf)
		{
		case '+':
			g_pbuf++;
			return TK_PLUS;
		case '-':
			g_pbuf++;
			return TK_MINUS;
		case '*':
			g_pbuf++;
			return TK_MULT;
		case '/':
			g_pbuf++;
			return TK_DIV;
		case '(':
			g_pbuf++;
			return TK_LPAREN;
		case ')':
			g_pbuf++;
			return TK_RPAREN;
		default:
			g_value = wcstod(g_pbuf, &g_pbuf);
			return TK_CONST;
		}
	}
	double prim()
	{
		const int token = get_token();
		switch (token)
		{
		case TK_CONST:
			return g_value;
		case TK_MINUS:
			return -prim();
		case TK_LPAREN:
		{
			const double val = expr();
			get_token();
			return val;
		}
		default:
			return 0;
		}
	}
	double term()
	{
		double val1 = prim(), val2;
		g_back = get_token();
		for (;;)
		{
			const TOKEN token = get_token();
			switch (token)
			{
			case TK_MULT:
				val1 *= prim();
				break;
			case TK_DIV:
				val2 = prim();
				g_back = get_token();
				val1 /= val2;
				break;
			default:
				g_back = token;
				return val1;
			}
		}
	}
};

enum STATUS {
	STATUS_NONE,
	STATUS_CALC,
	STATUS_ERROR,
	STATUS_SUCCESS,
};

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static STATUS status = STATUS_NONE;
	switch (msg)
	{
	case WM_APP:
		if (OpenClipboard(hWnd))
		{
			BOOL bReturn = FALSE;
			LPWSTR strText = 0;
			HGLOBAL hg = GetClipboardData(CF_UNICODETEXT);
			if (hg)
			{
				SIZE_T dwSize = GlobalSize(hg);
				strText = (LPWSTR)GlobalAlloc(0, dwSize);
				if (strText)
				{
					LPWSTR strClip = (LPWSTR)GlobalLock(hg);
					if (strClip)
					{
						lstrcpyW(strText, strClip);
						GlobalUnlock(hg);
					}
				}
				GlobalUnlock(hg);
			}
			if (strText && lstrlen(strText) != 0)
			{
				double ans = 0.0;
				calc(strText, &ans);
				WCHAR szText[1024];
				swprintf(szText, _countof(szText), TEXT("%g"), ans);
				hg = GlobalAlloc(GHND | GMEM_SHARE, sizeof(WCHAR) * (lstrlenW(szText) + 1));
				if (hg)
				{
					LPVOID strMem = (LPVOID)GlobalLock(hg);
					if (strMem)
					{
						lstrcpy((LPWSTR)strMem, szText);
					}
					GlobalUnlock(hg);
					EmptyClipboard();
					SetClipboardData(CF_UNICODETEXT, hg);
					bReturn = TRUE;
				}
			}
			CloseClipboard();
			return bReturn;
		}
		return FALSE;
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hWnd, &ps);
			HBRUSH hWhiteBrush = (HBRUSH)GetStockObject(WHITE_BRUSH);
			RECT rect;
			GetClientRect(hWnd, &rect);
			int nOldBkMode = SetBkMode(hdc, TRANSPARENT);
			switch (status)
			{
			case STATUS_NONE:
				FillRect(hdc, &rect, hWhiteBrush);
				break;
			case STATUS_CALC:
				{
					FillRect(hdc, &rect, hWhiteBrush);
					WCHAR szMessage[16];
					LoadString(GetModuleHandle(0), IDS_CALC, szMessage, _countof(szMessage));
					DrawText(hdc, szMessage, -1, &rect, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
				}
				break;
			case STATUS_ERROR:
				{
					HBRUSH hRedBrush = CreateSolidBrush(RGB(255, 192, 192));
					HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hRedBrush);
					FillRect(hdc, &rect, hRedBrush);
					HPEN hPen = CreatePen(0, 16, RGB(255, 0, 0));
					HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
					MoveToEx(hdc, rect.left + 10, rect.top + 10, 0);
					LineTo(hdc, rect.right - 10, rect.bottom - 10);
					MoveToEx(hdc, rect.right - 10, rect.top + 10, 0);
					LineTo(hdc, rect.left + 10, rect.bottom - 10);
					SelectObject(hdc, hOldPen);
					SelectObject(hdc, hOldBrush);
					DeleteObject(hPen);
					DeleteObject(hRedBrush);
					WCHAR szMessage[16];
					LoadString(GetModuleHandle(0), IDS_ERROR, szMessage, _countof(szMessage));
					DrawText(hdc, szMessage, -1, &rect, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
				}
				break;
			case STATUS_SUCCESS:
				{
					HBRUSH hGreenBrush = CreateSolidBrush(RGB(192, 255, 192));
					HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hGreenBrush);
					FillRect(hdc, &rect, hGreenBrush);
					HPEN hPen = CreatePen(0, 16, RGB(0, 255, 0));
					HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
					Ellipse(hdc, rect.left + 10, rect.top + 10, rect.right - 10, rect.bottom - 10);
					SelectObject(hdc, hOldPen);
					SelectObject(hdc, hOldBrush);
					DeleteObject(hPen);
					DeleteObject(hGreenBrush);
					WCHAR szMessage[16];
					LoadString(GetModuleHandle(0), IDS_SUCCESS, szMessage, _countof(szMessage));
					DrawText(hdc, szMessage, -1, &rect, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
			}
				break;
			}
			SetBkMode(hdc, nOldBkMode);
			EndPaint(hWnd, &ps);
		}
		break;
	case WM_SETFOCUS:
		status = STATUS_CALC;
		InvalidateRect(hWnd, 0, 1);
		UpdateWindow(hWnd);
		if (SendMessage(hWnd, WM_APP, 0, 0))
		{
			status = STATUS_SUCCESS;
		}
		else
		{
			status = STATUS_ERROR;
		}
		InvalidateRect(hWnd, 0, 1);
		UpdateWindow(hWnd);
		break;
	case WM_KILLFOCUS:
		status = STATUS_NONE;
		InvalidateRect(hWnd, 0, 1);
		UpdateWindow(hWnd);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPreInst, LPWSTR pCmdLine, int nCmdShow)
{
	MSG msg;
	WNDCLASS wndclass = {
		CS_HREDRAW | CS_VREDRAW,
		WndProc,
		0,
		0,
		hInstance,
		LoadIcon(hInstance,MAKEINTRESOURCE(IDI_ICON1)),
		LoadCursor(0,IDC_ARROW),
		(HBRUSH)(COLOR_WINDOW + 1),
		0,
		szClassName
	};
	RegisterClass(&wndclass);
	WCHAR szTitle[16];
	LoadString(hInstance, IDS_APPTITLE, szTitle, _countof(szTitle));
	HWND hWnd = CreateWindowEx(
		WS_EX_TOPMOST,
		szClassName,
		szTitle,
		WS_CAPTION | WS_SYSMENU,
		CW_USEDEFAULT,
		0,
		200,
		200,
		0,
		0,
		hInstance,
		0
	);
	ShowWindow(hWnd, SW_SHOWDEFAULT);
	UpdateWindow(hWnd);
	while (GetMessage(&msg, 0, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return (int)msg.wParam;
}
