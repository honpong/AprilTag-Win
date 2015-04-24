// SetAmeterSensitivity.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <windows.h>
#include <thread>
#include <chrono>
#include "AccelerometerLib.h"

int _tmain(int argc, _TCHAR* argv[])
{
	CoInitialize(NULL);

	bool exit = false;

	bool success = SetAccelerometerSensitivity(0);

	if (success)
		std::cout << "Set accelerometer change sensitivity to 0.\nLeave this app running to keep the setting in effect.\n";
	else 
		std::cout << "FAILED to set accelerometer change sensitivity.\n";

	std::cout << "Press space/escape/return to exit.\n";

	while (exit == false)
	{
		if (GetAsyncKeyState(VK_ESCAPE) || GetAsyncKeyState(VK_SPACE) || GetAsyncKeyState(VK_RETURN)) exit = true;

		std::this_thread::sleep_for(std::chrono::milliseconds(16));
	}

	CoUninitialize();
	return 0;
}

