#pragma once
#include <fstream>


class Log
{

	std::wstring filename;
	std::wostringstream log;
public:
	Log(const wchar_t *szFilename) : filename (szFilename)
	{
		log << "Startup logging" << std::endl;
		
	}

	void Out(const wchar_t *text)
	{
		log << text << std::endl;
	}
	~Log()
	{
		std::wfstream fout(filename.c_str(), std::ios_base::out);
		log << "Ending loggin" << std::endl;
		fout << log.str().c_str();
	}

};
