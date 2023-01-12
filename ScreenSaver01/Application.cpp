#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <objbase.h>
#include <commdlg.h>

#include <strsafe.h>

#include <ScrnSave.h>
#pragma comment(lib, "Scrnsavw.lib")

#pragma comment(lib, "Comctl32.lib")


#pragma region c++

#include <algorithm>
#include <array>
#include <fstream>
#include <sstream>
#include <vector>

#include "Image.h"
#include "Registry.h"
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

constexpr wchar_t applicationProvider[] = L"Tretton63";
static TCHAR szAppName[APPNAMEBUFFERLEN] = L"ScreenSaver01";

static size_t i = 0;
static FLOAT x, y, zz = 0.0f;

static Scene::Image* image = nullptr;

LRESULT WINAPI ScreenSaverProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static UINT_PTR timerID;
	static HDC hdc;
	static RECT rct;


	switch (msg)
	{
	case WM_CREATE:
		{
			srand(time(nullptr));
			HRESULT hr = S_OK;
			hr = ::CoInitialize(nullptr);
			if (FAILED(hr))
			{
				SendMessage(hwnd, WM_DESTROY, 0, 0);
				return FALSE;
			}
			tretton63::Registry reg(applicationProvider);
			if(!reg.IsHiveOpen())
			{
				MessageBox(hwnd, L"Please configure the screen saver", L"Not configured", MB_OK | MB_ICONERROR);
				return FALSE;
			}
			auto filename = reg.Filename();
			image = new Scene::Image(hwnd, filename); 

			timerID = SetTimer(hwnd, 1, 25, nullptr);
		}
		break;
	case WM_ERASEBKGND:
		return TRUE;
	case WM_TIMER:
		{
			image->Draw(x,y);
			
			x = 40 * sin(zz+0.75f* 75.0f);
			y = 40 * cos(zz+0.75f* 75.f);
			zz += 0.01f;
		}
		break;
	case WM_DESTROY:
		{
			if (timerID)
				KillTimer(hwnd, timerID);
			delete image;
			CoUninitialize();
			break;
		}
	default:
		return DefScreenSaverProc(hwnd, msg, wParam, lParam);
	}
	return DefScreenSaverProc(hwnd, msg, wParam, lParam);
}

BOOL WINAPI ScreenSaverConfigureDialog(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_INITDIALOG:
		{
			tretton63::Registry reg(applicationProvider);
			if(!reg.IsHiveOpen())
			{
				return FALSE;
			}
			const auto filename = reg.Filename();
			if (!filename.empty())
			{
				OutputDebugStringW(filename.c_str());
			}
		}
		return TRUE;
	case WM_COMMAND:

		if (wParam == IDOK)
		{
			// Find image

			tretton63::Registry reg(applicationProvider);
			if(!reg.IsHiveOpen())
			{
				OutputDebugStringW(L"Registry is not open");
				return FALSE;
			}
			wchar_t szFilename[255] = {0};
			OPENFILENAMEW ofn{};
			ZeroMemory(&ofn, sizeof(ofn));
			ofn.lStructSize = sizeof(ofn);
			ofn.dwReserved = 0;
			ofn.hInstance = hMainInstance;
			ofn.hwndOwner = hwnd;
			ofn.lpstrFile = szFilename;
			ofn.lpstrFile[0] = '\0';
			ofn.nMaxFile = sizeof(szFilename);
			ofn.lpstrFilter = L"All\0*.*\0Image Files\0*.PNG;*.JPG;*.JPEG\0";
			ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;



			
			if (!GetOpenFileNameW(&ofn))
			{
				OutputDebugStringW(L"failed to use openfile dialog\n");
				return FALSE;
			}

			auto newFilename = std::wstring(ofn.lpstrFile);
			reg.SetFilename(newFilename);
		}
		else if (wParam == IDCANCEL)
		{
			EndDialog(hwnd, 0);
		}
		break;
	default:
		return FALSE;
	}
	return FALSE;
}

BOOL WINAPI RegisterDialogClasses(HANDLE hInst)
{
	return TRUE;
}
