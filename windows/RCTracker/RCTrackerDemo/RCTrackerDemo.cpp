// RCTrackerDemo.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <filter/rc_intel_interface.h>

using namespace std;

int _tmain(int argc, _TCHAR* argv[])
{
    const _TCHAR* fileName = nullptr;
    if (argc > 1) fileName = (const _TCHAR*)argv[1];

    if (fileName)
    {
        wcout << "Filename: " << fileName << endl;
    }

    cout << "Press return to exit." << endl;
    cin.get();
	return 0;
}

