#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <ScrnSave.h>

#include <algorithm>
#include <strsafe.h>
#pragma comment(lib, "Scrnsavw.lib")
#pragma comment(lib, "Comctl32.lib")

#include <d2d1.h>
#pragma comment(lib, "d2d1.lib")
#include <fstream>
#include <sstream>
#include <wincodec.h>
#pragma comment(lib, "windowscodecs.lib")

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


// MUST BE DECLARED
template <class T>
void SafeRelease(T** ppT)
{
	if (*ppT)
	{
		(*ppT)->Release();
		*ppT = NULL;
	}
}

static TCHAR szAppName[APPNAMEBUFFERLEN] = L"ScreenSaver01";

static size_t i = 0;
static FLOAT x, y;
std::wostringstream debug;

static ID2D1Factory* pFactory = nullptr;
static ID2D1HwndRenderTarget* pRenderTarget = nullptr;
static ID2D1SolidColorBrush* pBrush = nullptr;
static ID2D1Bitmap* pBitmap = nullptr;

static IWICImagingFactory* pWicFactory = nullptr;
static IWICBitmapDecoder* pBitmapDecoder = nullptr;
static IWICBitmapFrameDecode* pBitmapFrameDecoder = nullptr;
static IWICFormatConverter* pWicConverter = nullptr;


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
			hr = CoInitialize(nullptr);
			if (FAILED(hr))
			{
				debug << L"failed to co initialize" << std::endl;
				return false;
			}


			hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, _uuidof(ID2D1Factory),
			                       reinterpret_cast<void**>(&pFactory));
			if (FAILED(hr))
			{
				debug << L"failed to create d2d1 factory" << std::endl;
				return FALSE;
			}
			GetClientRect(hwnd, &rct);
			const auto width = rct.right - rct.left;
			const auto height = rct.bottom - rct.top;
			hr = pFactory->CreateHwndRenderTarget(
				D2D1::RenderTargetProperties(),
				D2D1::HwndRenderTargetProperties(
					hwnd,
					D2D1::SizeU(width, height)),
				&pRenderTarget);
			if (FAILED(hr))
			{
				debug << L"failed to create hwnd render target" << std::endl;
				return FALSE;
			}

			hr = pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &pBrush);
			if (FAILED(hr))
			{
				debug << L"failed to create solid color brush" << std::endl;
				return FALSE;
			}

			// Create the COM imaging factory
			hr = CoCreateInstance(
				CLSID_WICImagingFactory,
				nullptr,
				CLSCTX_INPROC_SERVER,
				IID_PPV_ARGS(&pWicFactory)
			);
			if (FAILED(hr))
			{
				debug << L"failed to create wic factory" << std::endl;
				return FALSE;
			}

			hr = pWicFactory->CreateDecoderFromFilename(
				L"C:\\Code\\test2.png",
				nullptr,
				GENERIC_READ,
				WICDecodeMetadataCacheOnLoad, &pBitmapDecoder);
			if (FAILED(hr))
			{
				debug << L"failed to create decoder from filename" << std::endl;
				return FALSE;
			}

			hr = pBitmapDecoder->GetFrame(0, &pBitmapFrameDecoder);
			if (FAILED(hr))
			{
				debug << L"failed to get frame" << std::endl;
				return FALSE;
			}
			
			if (pBitmapFrameDecoder == nullptr)
			{
				debug << L"bitmap frame decoder is null" << std::endl;
			}
			UINT w, h;
			hr = pBitmapFrameDecoder->GetSize(&w, &h);
			if (SUCCEEDED(hr))
			{
				debug << L"Width " << w << L" " << L"Height " << h << std::endl;
			}

			// Before this can happen we need to convert

			hr = pWicFactory->CreateFormatConverter(&pWicConverter);
			if (SUCCEEDED(hr))
			{
				hr = pWicConverter->Initialize(
					pBitmapFrameDecoder,
					GUID_WICPixelFormat32bppPBGRA,
					WICBitmapDitherTypeNone,
					nullptr,
					0.0,
					WICBitmapPaletteTypeMedianCut);
				if (SUCCEEDED(hr))
				{
					hr = pRenderTarget->CreateBitmapFromWicBitmap(
						pWicConverter,
						&pBitmap);
					if (SUCCEEDED(hr))
					{
						//FormatMessage()

						debug << L"created bitmap from wic bitmap" << std::endl;
					}
				}
			}


			timerID = SetTimer(hwnd, 1, 25, nullptr);
		}
		break;
	case WM_ERASEBKGND:
		return TRUE;
	case WM_TIMER:
		{
			pRenderTarget->BeginDraw();
			pRenderTarget->SetTransform(D2D1::IdentityMatrix());
			pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::Black));
			pRenderTarget->DrawBitmap(
				pBitmap,
				D2D1::RectF(
					100.0f + x,
					100.0f + y,
					640.f,
					480.0f));

			pRenderTarget->EndDraw();
			x += 0.5f;
			y += 0.5f;
		}
		break;
	case WM_DESTROY:
		{
			if (timerID)
				KillTimer(hwnd, timerID);

			SafeRelease(&pWicConverter);
			SafeRelease(&pBrush);
			SafeRelease(&pRenderTarget);
			SafeRelease(&pFactory);
			SafeRelease(&pWicFactory);
			SafeRelease(&pBitmapDecoder);
			SafeRelease(&pBitmapFrameDecoder);
			SafeRelease(&pBitmap);
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
		GetPrivateProfileIntA()
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
