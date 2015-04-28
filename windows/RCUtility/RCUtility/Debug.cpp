#include "stdafx.h"
#include "Debug.h"

using namespace RealityCap;

#define BUFFER_LENGTH 1024

std::wstring Debug::Log(std::wstring format, ...)
{
#if _DEBUG
	format.append(L"\n");
	wchar_t buffer[BUFFER_LENGTH];
	va_list args;
	va_start(args, format);
	_vsnwprintf_s(buffer, BUFFER_LENGTH, _TRUNCATE, format.c_str(), args);
	va_end(args);
	buffer[BUFFER_LENGTH - 1] = '\0'; //prevent buffer overflow
	OutputDebugString(buffer);
	return std::wstring(buffer);
#endif
}
