#include <string>

#pragma once
namespace RealityCap
{
	class Debug
	{
	public:
		/**
		Returns the formatted string.
		*/
		static std::wstring Log(std::wstring format, ...);
	};
}
