#pragma once

#include <d2d1.h>
#include <d2d1_1.h>
#include <d2d1effects.h>
#include <d2d1effects_2.h>
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dxguid.lib")
#include <wincodec.h>
#include <wrl.h>
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

namespace Scene
{
	using namespace Microsoft::WRL;

	inline long Width(const RECT& rct)
	{
		return rct.right - rct.left;
	}

	inline long Height(const RECT& rct)
	{
		return rct.bottom - rct.top;
	}

	class Image
	{
		HWND Hwnd;
		RECT ClientRect{};

		ComPtr<ID2D1Factory> pFactory = nullptr;
		ComPtr<ID2D1HwndRenderTarget> pRenderTarget = nullptr;
		ComPtr<ID2D1DeviceContext> pDeviceContext = nullptr;

		ComPtr<ID2D1Bitmap> pBitmap = nullptr;
		ComPtr<ID2D1Effect> pEffect = nullptr;
		ComPtr<ID2D1SolidColorBrush> pBrush = nullptr;
		

		ComPtr<IWICImagingFactory> pWicFactory = nullptr;
		ComPtr<IWICBitmapDecoder> pWicBitmapDecoder = nullptr;
		ComPtr<IWICBitmapFrameDecode> pWicBitmapFrameDecoder = nullptr;
		ComPtr<IWICFormatConverter> pWicConverter = nullptr;
		ComPtr<IWICBitmap> pWicBitmap = nullptr;


		std::wstring filename;


		bool Init()
		{
			HRESULT hr = S_OK;
			hr = D2D1CreateFactory(
				D2D1_FACTORY_TYPE_SINGLE_THREADED,
				pFactory.GetAddressOf());
			if (FAILED(hr))
			{
				MessageBoxW(Hwnd, L"Failed to create factoy", L"Errorr ", MB_OK);
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

			GetClientRect(Hwnd, &ClientRect);

			hr = pFactory->CreateHwndRenderTarget(
				D2D1::RenderTargetProperties(),
				D2D1::HwndRenderTargetProperties(
					Hwnd,
					D2D1::SizeU(Width(ClientRect), Height(ClientRect))),
				pRenderTarget.GetAddressOf());
			if (FAILED(hr))
			{
				return false;
			}

			hr = pRenderTarget->CreateSolidColorBrush(
				D2D1::ColorF(D2D1::ColorF::White),
				pBrush.GetAddressOf());
			if (FAILED(hr))
			{
				return false;
			}
			

			hr = pWicFactory->CreateDecoderFromFilename(
				filename.c_str(),
				nullptr,
				GENERIC_READ,
				WICDecodeMetadataCacheOnLoad, &pWicBitmapDecoder);
			if (FAILED(hr))
			{
				return false;
			}

			hr = pWicBitmapDecoder->GetFrame(0, &pWicBitmapFrameDecoder);
			if (FAILED(hr))
			{
				return false;
			}


			UINT w, h;
			hr = pWicBitmapFrameDecoder->GetSize(&w, &h);

			// Before this can happen we need to convert
			hr = pRenderTarget->QueryInterface(__uuidof(ID2D1DeviceContext), &pDeviceContext);
			if (FAILED(hr))
			{
				MessageBoxW(Hwnd, L"Failed to create device context", L"Error", MB_OK);
				return false;
			}

			hr = pDeviceContext->CreateEffect(CLSID_D2D1EdgeDetection, &pEffect);
			if (FAILED(hr))
			{
				MessageBox(Hwnd, L"Failed to create effect", L"Error", MB_OK | MB_ICONERROR);

				return false;
			}

			hr = pWicFactory->CreateFormatConverter(&pWicConverter);
			if (SUCCEEDED(hr))
			{
				hr = pWicConverter->Initialize(
					pWicBitmapFrameDecoder.Get(),
					GUID_WICPixelFormat32bppPBGRA,
					WICBitmapDitherTypeNone,
					nullptr,
					0.0,
					WICBitmapPaletteTypeMedianCut);
				if (SUCCEEDED(hr))
				{
					hr = pRenderTarget->CreateBitmapFromWicBitmap(
						pWicConverter.Get(),
						&pBitmap);
					if (SUCCEEDED(hr))
					{
						pEffect->SetInput(0, pBitmap.Get());
						pEffect->SetValue(D2D1_EDGEDETECTION_PROP_STRENGTH, 0.2);
						// pEffect->SetValue(D2D1_MORPHOLOGY_PROP_MODE, D2D1_MORPHOLOGY_MODE_ERODE);
						// pEffect->SetValue(D2D1_MORPHOLOGY_PROP_WIDTH, 14);
					}
				}
			}

			hr = pWicFactory->CreateBitmapFromSource(pWicConverter.Get(), WICBitmapCacheOnDemand, &pWicBitmap);
			if (FAILED(hr))
			{
				return false;
			}


			return true;
		}

	public:
		explicit Image(HWND hwnd, std::wstring& filename) : Hwnd(hwnd), filename(filename)
		{
			if (!Init())
			{
			}
		}

		~Image()
		{
		}


		void Draw(FLOAT x, FLOAT y)
		{
			pDeviceContext->BeginDraw();
			pDeviceContext->SetTransform(D2D1::IdentityMatrix());
			pDeviceContext->Clear(D2D1::ColorF(D2D1::ColorF::Black));
			for (int i = 0; i < 5; i++)
			{
				pDeviceContext->DrawBitmap(
					pBitmap.Get(),
					D2D1::RectF(
						static_cast<float>(i * 50) + x,
						static_cast<float>((i * 50)) + y,
						static_cast<float>((ClientRect.right - ClientRect.left) - (i * 50)),
						static_cast<float>((ClientRect.bottom - ClientRect.top) - (i * 50))));
			}
			pDeviceContext->DrawImage(pEffect.Get(), D2D1::Point2F(6 * 50, 6 * 50));
			pDeviceContext->EndDraw();
		}
	};
}
