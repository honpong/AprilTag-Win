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
        static void Log(std::wstring format, ...);
    };
}
