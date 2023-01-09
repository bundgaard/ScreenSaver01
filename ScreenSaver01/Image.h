#pragma once

#include <d2d1.h>
#pragma comment(lib, "d2d1.lib")
#include <wincodec.h>
#pragma comment(lib, "windowscodecs.lib")
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

namespace scene
{
	class Image
	{
		HWND Hwnd;
		RECT ClientRect{};

		ID2D1Factory* pFactory = nullptr;
		ID2D1HwndRenderTarget* pRenderTarget = nullptr;
		ID2D1SolidColorBrush* pBrush = nullptr;
		ID2D1Bitmap* pBitmap = nullptr;

		IWICImagingFactory* pWicFactory = nullptr;
		IWICBitmapDecoder* pBitmapDecoder = nullptr;
		IWICBitmapFrameDecode* pBitmapFrameDecoder = nullptr;
		IWICFormatConverter* pWicConverter = nullptr;

		bool Init()
		{
			HRESULT hr = S_OK;
			hr = D2D1CreateFactory(
				D2D1_FACTORY_TYPE_SINGLE_THREADED,
				_uuidof(ID2D1Factory),
				reinterpret_cast<void**>(&pFactory));
			if (FAILED(hr))
			{
				return false;
			}

			GetClientRect(Hwnd, &ClientRect);
			const auto width = ClientRect.right - ClientRect.left;
			const auto height = ClientRect.bottom - ClientRect.top;

			hr = pFactory->CreateHwndRenderTarget(
				D2D1::RenderTargetProperties(),
				D2D1::HwndRenderTargetProperties(
					Hwnd,
					D2D1::SizeU(width, height)),
				&pRenderTarget);
			if (FAILED(hr))
			{
				return false;
			}

			hr = pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &pBrush);
			if (FAILED(hr))
			{
				return false;
			}

			hr = CoCreateInstance(
				CLSID_WICImagingFactory,
				nullptr,
				CLSCTX_INPROC_SERVER,
				IID_PPV_ARGS(&pWicFactory)
			);
			if (FAILED(hr))
			{
				return false;
			}

			hr = pWicFactory->CreateDecoderFromFilename(
				L"C:\\Code\\test2.png",
				nullptr,
				GENERIC_READ,
				WICDecodeMetadataCacheOnLoad, &pBitmapDecoder);
			if (FAILED(hr))
			{
				return false;
			}

			hr = pBitmapDecoder->GetFrame(0, &pBitmapFrameDecoder);
			if (FAILED(hr))
			{
				return false;
			}


			UINT w, h;
			hr = pBitmapFrameDecoder->GetSize(&w, &h);
			if (!SUCCEEDED(hr))
			{
				return false;
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
						return false;
					}
				}
			}

			return true;
		}

	public:
		explicit Image(HWND hwnd) : Hwnd(hwnd)
		{
			if (!Init())
			{
			}
		}

		~Image()
		{
			SafeRelease(&pWicConverter);
			SafeRelease(&pBrush);
			SafeRelease(&pRenderTarget);
			SafeRelease(&pFactory);
			SafeRelease(&pWicFactory);
			SafeRelease(&pBitmapDecoder);
			SafeRelease(&pBitmapFrameDecoder);
			SafeRelease(&pBitmap);
		}


		void Draw(FLOAT x, FLOAT y)
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
		}
	};
}
