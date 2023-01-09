#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <objbase.h>


#include <strsafe.h>

#include <ScrnSave.h>
#pragma comment(lib, "Scrnsavw.lib")

#pragma comment(lib, "Comctl32.lib")


#pragma region c++

#include <algorithm>
#include <fstream>
#include <sstream>

#include "Image.h"
#pragma endregion

void ErrorExit()
{
	// Retrieve the system error message for the last-error code

	LPVOID lpMsgBuf;
	const auto dw = GetLastError();

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		reinterpret_cast<LPTSTR>(&lpMsgBuf),
		0, nullptr);

	// Display the error message and exit the process

	const auto lpDisplayBuf = LocalAlloc(LMEM_ZEROINIT,
	                                     (lstrlen(static_cast<LPCTSTR>(lpMsgBuf)) + 40) * sizeof(TCHAR));
	if (lpDisplayBuf != nullptr)
	{
		StringCchPrintf(static_cast<LPTSTR>(lpDisplayBuf),
		                LocalSize(lpDisplayBuf) / sizeof(TCHAR),
		                TEXT("%s failed with error %d: %s"),
		                "a Function", dw, lpMsgBuf);
		MessageBox(nullptr, static_cast<LPCTSTR>(lpDisplayBuf), TEXT("Error"), MB_OK);
	}

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);
}


static TCHAR szAppName[APPNAMEBUFFERLEN] = L"ScreenSaver01";

static size_t i = 0;
static FLOAT x, y;
std::wostringstream debug;

scene::Image *image = nullptr;


LRESULT WINAPI ScreenSaverProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static UINT_PTR timerID;
	static HDC hdc;
	static RECT rct;
	switch (msg)
	{
	case WM_CREATE:
		{
			HRESULT hr = S_OK;
			hr = ::CoInitialize(nullptr);
			if (FAILED(hr))
			{
				debug << L"failed to co initialize" << std::endl;
				return false;
			}	

			image = new scene::Image(hwnd);

			timerID = SetTimer(hwnd, 1, 25, nullptr);
		}
		break;
	case WM_ERASEBKGND:
		return TRUE;
	case WM_TIMER:
		{

			image->Draw(x,y);
			x += 0.5f;
			y += 0.5f;
		}
		break;
	case WM_DESTROY:
		{
			if (timerID)
				KillTimer(hwnd, timerID);

			if(image != nullptr)
			{
				delete image;
			}
			CoUninitialize();

			std::wfstream out;
			out.open(L"C:\\code\\debug.txt", std::ios_base::out);
			out << L"Writing log" << std::endl;
			out << debug.str().c_str();
			break;
		}
	default:
		return DefScreenSaverProc(hwnd, msg, wParam, lParam);
	}
	return DefScreenSaverProc(hwnd, msg, wParam, lParam);
}

BOOL WINAPI ScreenSaverConfigureDialog(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_INITDIALOG:
		// Should write it to registry instead of using old 16 bit code.
		return TRUE;
	case WM_COMMAND:
		
		if(wParam== IDOK)
		{
			
			// Find image
		}else if (wParam == IDCANCEL)
		{
			EndDialog(hDlg, 0);
		}
		break;
	}
	return FALSE;
}

BOOL WINAPI RegisterDialogClasses(HANDLE hInst)
{
	return TRUE;
}
