#pragma once
#include <Windows.h>
#include <string>

namespace tretton63
{
	class Registry
	{
		std::wstring ApplicationName;
		HKEY hive = nullptr;
		LSTATUS lStatus = ERROR_SUCCESS;

	public:
		explicit Registry(const wchar_t* applicationName) : ApplicationName(applicationName)
		{
			lStatus = RegOpenKeyExW(HKEY_CURRENT_USER, applicationName, 0, KEY_READ | KEY_WRITE, &hive);
			if (lStatus != ERROR_SUCCESS)
			{
				DWORD regDisposition;
				RegCreateKeyExW(HKEY_CURRENT_USER, applicationName, 0, nullptr, REG_OPTION_NON_VOLATILE,
				                KEY_READ | KEY_WRITE, nullptr, &hive, &regDisposition);
			}
		}

		Registry(Registry& c) = delete;

		[[nodiscard]] bool IsHiveOpen() const
		{
			return hive != nullptr;
		}

		std::wstring Filename()
		{
			DWORD cellType;
			DWORD cellSize = 0;
			RegQueryValueExW(hive, L"filename", nullptr, &cellType, nullptr, &cellSize);

			std::wstring filename;
			filename.resize(cellSize);
			DWORD regType = 0;
			DWORD regGetSize = filename.size();
			lStatus = RegGetValueW(hive, nullptr, L"filename", RRF_RT_REG_SZ, &regType, filename.data(), &regGetSize);
			if (lStatus != ERROR_SUCCESS)
			{
				OutputDebugStringW(L"Failed to get value from cell\n");
				return {};
			}


			return filename;
		}

		void SetFilename(std::wstring& newFilename)
		{
			std::vector<BYTE> toCell(newFilename.size() * 4);

			auto toReserve = WideCharToMultiByte(CP_UTF8,
			                                     0,
			                                     newFilename.c_str(),
			                                     newFilename.size(),
			                                     nullptr,
			                                     0,
			                                     "",
			                                     nullptr);
			std::string filenameUTF8;
			filenameUTF8.resize(toReserve);
			WideCharToMultiByte(CP_UTF8, 0, newFilename.c_str(), newFilename.size(),
			                    filenameUTF8.data(), toReserve,
			                    "",
			                    nullptr);


			lStatus = RegSetValueExW(
				hive,
				L"filename",
				0,
				REG_SZ,
				reinterpret_cast<LPBYTE>(newFilename.data()),
				newFilename.size() * sizeof(wchar_t));
			if (lStatus != ERROR_SUCCESS)
			{
				OutputDebugStringW(L"Failed to write data to cell\n");
			}
		}

		~Registry()
		{
			RegCloseKey(hive);
		}
	};
}
