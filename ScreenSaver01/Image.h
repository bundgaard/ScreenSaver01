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

	inline auto validate_result(HRESULT result, std::string const& error_message)
	{
		if (FAILED(result)) throw std::runtime_error{error_message};
	}
	template <typename OutType>
	OutType Width(const RECT& rct)
	{
		return static_cast<OutType>(rct.right - rct.left);
	}
	template<typename OutType>
	OutType Height(const RECT& rct)
	{
		return static_cast<OutType>(rct.bottom - rct.top);
	}

	class Graphic
	{
		HWND Hwnd;

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

		auto Init() -> bool
		{
			HRESULT hr = S_OK;

			if (FAILED(hr))
			{
				return false;
			}


			return true;
		}

	public:
		Graphic(HWND hwnd) : Hwnd(hwnd)
		{
			HRESULT hr = S_OK;
			hr = D2D1CreateFactory(
				D2D1_FACTORY_TYPE_SINGLE_THREADED,
				pFactory.GetAddressOf());
			validate_result(hr, "Failed to create factory");
			RECT ClientRect{};
			GetClientRect(hwnd, &ClientRect);
			hr = pFactory->CreateHwndRenderTarget(
				D2D1::RenderTargetProperties(),
				D2D1::HwndRenderTargetProperties(
					Hwnd,
					D2D1::SizeU(Width<UINT32>(ClientRect), Height<UINT32>(ClientRect))),
				pRenderTarget.GetAddressOf());
			validate_result(hr, "Failed to create render target");

			hr = pRenderTarget->QueryInterface(__uuidof(ID2D1DeviceContext), &pDeviceContext);
			validate_result(hr, "Failed to create device context");


			/////////////////////////////
			hr = CoCreateInstance(
				CLSID_WICImagingFactory,
				nullptr,
				CLSCTX_INPROC_SERVER,
				IID_PPV_ARGS(&pWicFactory)
			);
			validate_result(hr, "failed to create wic image factory");


			/////////////////////////////

			hr = pRenderTarget->CreateSolidColorBrush(
				D2D1::ColorF(D2D1::ColorF::White),
				pBrush.GetAddressOf());
			validate_result(hr, "Failed to create solid brush");

			hr = pDeviceContext->CreateEffect(CLSID_D2D1EdgeDetection, &pEffect);
			validate_result(hr, "Failed to create effect");

			OutputDebugStringW(L"Created graphic object");
		}


		auto BeginDraw() const
		{
			pDeviceContext->BeginDraw();
			pDeviceContext->SetTransform(D2D1::IdentityMatrix());
			pDeviceContext->Clear(D2D1::ColorF(D2D1::ColorF::Black));
		}

		auto EndDraw() const
		{
			pDeviceContext->EndDraw();
		}

		auto Resize()
		{
			RECT ClientRect{};
			GetClientRect(Hwnd, &ClientRect);
			validate_result(pRenderTarget->Resize(D2D1::SizeU(Width<UINT32>(ClientRect), Height<UINT32>(ClientRect))),
			                "failed to resize render target");
		}

		auto DrawLine(const float x1, const float y1, const float x2, const float y2,
		              const float strokeWidth = 1.0f) const
		{
			pDeviceContext->DrawLine(
				D2D1::Point2F(x1, y1),
				D2D1::Point2F(x2, y2),
				pBrush.Get(),
				strokeWidth);
		}


		auto LoadBitmap(const std::wstring& filename)
		{
			HRESULT hr = S_OK;

			hr = pWicFactory->CreateDecoderFromFilename(
				filename.c_str(),
				nullptr,
				GENERIC_READ,
				WICDecodeMetadataCacheOnLoad, &pWicBitmapDecoder);
			validate_result(hr, "failed to create decoder from file");

			hr = pWicBitmapDecoder->GetFrame(0, &pWicBitmapFrameDecoder);
			validate_result(hr, "failed to get frame");


			hr = pWicFactory->CreateFormatConverter(&pWicConverter);
			validate_result(hr, "failed to create converter");

			hr = pWicConverter->Initialize(
				pWicBitmapFrameDecoder.Get(),
				GUID_WICPixelFormat32bppPBGRA,
				WICBitmapDitherTypeNone,
				nullptr,
				0.0,
				WICBitmapPaletteTypeMedianCut);
			validate_result(hr, "failed to initialize converter");

			hr = pWicFactory->CreateBitmapFromSource(pWicConverter.Get(), WICBitmapCacheOnDemand, &pWicBitmap);
			validate_result(hr, "failed to create bitmap from source");

			hr = pRenderTarget->CreateBitmapFromWicBitmap(
				pWicConverter.Get(),
				&pBitmap);
			validate_result(hr, "failed to create bitmap from wic bitmap");
		}

		[[nodiscard]] auto IsBitmapLoaded() const -> bool
		{
			return pBitmap != nullptr;
		}

		auto DrawBitmap(float left, float top, float right, float bottom, float alpha = 1.0f) const
		{
			pDeviceContext->DrawBitmap(pBitmap.Get(), D2D1::RectF(left, top, right, bottom), alpha);
		}
	};

	class Image
	{
		HWND Hwnd;

		std::wstring filename;

	public:
		explicit Image(HWND hwnd, std::wstring& filename) : Hwnd(hwnd), filename(filename)
		{
			/*if (!Init())
			{
			}*/
		}

		~Image() = default;


		auto Draw(FLOAT x, FLOAT y) -> void
		{
			/*	pDeviceContext->BeginDraw();
				pDeviceContext->SetTransform(D2D1::IdentityMatrix());
				pDeviceContext->Clear();
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
				pDeviceContext->EndDraw();*/
		}
	};
}
