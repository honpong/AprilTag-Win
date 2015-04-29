// RCUtility.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "RCUtility.h"
#include "IMUManager.h"
#include "Debug.h"
#include "CaptureManager.h"
#include "AccelerometerLib.h"
#include <shellapi.h>
#include "CalibrationManager.h"
#include <shlobj.h>
#include <shlwapi.h>

#using <Windows.winmd>
#using <Platform.winmd>

using namespace RealityCap;
using namespace Platform;
using namespace Windows::Devices::Sensors;

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name
CaptureManager^ capMan;
CalibrationManager^ calMan;
bool isCapturing = false;
bool isCalibrating = false;
HWND hLabel;
HWND hCaptureButton;
HWND hCalibrateButton;
HWND hLiveButton;
HWND hReplayButton;

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

class MyCalDelegate : CalibrationManagerDelegate
{
public:
	MyCalDelegate(): CalibrationManagerDelegate() {};

	virtual void OnProgressUpdated(float progress) 
	{
		if (progress < 1.)
		{
			std::wstring progString = Debug::Log(L"Progress %2.0f%%", progress * 100);
			SetWindowText(hLabel, progString.c_str());
		}
	};

	virtual void OnStatusUpdated(int status) 
	{
		switch (status)
		{
		case 1:
			SetWindowText(hLabel, TEXT("Place the device flat on a table."));
			break;
		default:
			break;
		}		
	};
};

MyCalDelegate calDelegate;

void StartCapture()
{
	if (isCapturing || isCalibrating) return;

	SetWindowText(hCaptureButton, L"Stop Capture");
	SetWindowText(hLabel, L"Starting capture...");
	bool result;
	capMan = ref new CaptureManager();

	result = SetAccelerometerSensitivity(0);
	if (!result)
	{
		SetWindowText(hLabel, L"Failed to set accelerometer sensitivity");
		return;
	}

	result = capMan->StartSensors();
	if (!result)
	{
		SetWindowText(hLabel, L"Failed to start sensors");
		return;
	}

	result = capMan->StartCapture();
	if (result)
	{
		SetWindowText(hLabel, L"Capturing.");
	}
	else
	{
		SetWindowText(hLabel, L"Failed to start capture");
		return;
	}

	isCapturing = result;
}

void StopCapture()
{
	if (!isCapturing) return;
	SetWindowText(hLabel, L"Stopping capture...");
	capMan->StopCapture();
	capMan->StopSensors();
	delete capMan;
	isCapturing = false;
	SetWindowText(hLabel, L"Capture complete.");
	SetWindowText(hCaptureButton, L"Start Capture");
}

void StartCalibration()
{
	if (isCalibrating || isCapturing) return;
	SetWindowText(hLabel, TEXT("Starting calibration..."));

	bool result = SetAccelerometerSensitivity(0);
	if (!result)
	{
		SetWindowText(hLabel, L"Failed to set accelerometer sensitivity");
		return;
	}

	calMan = ref new CalibrationManager();
	calMan->SetDelegate(&calDelegate);
	result = calMan->StartCalibration();
	if (result)
	{
		SetWindowText(hCalibrateButton, L"Stop Calibrating");
		isCalibrating = true;
	}
	else
	{
		SetWindowText(hLabel, TEXT("Failed to start calibration."));
	}
}

void StopCalibration()
{
	if (!isCalibrating) return;
	SetWindowText(hLabel, TEXT("Stopping calibration..."));
	calMan->StopCalibration();
	delete calMan;
	SetWindowText(hLabel, TEXT("Calibration complete."));
	SetWindowText(hCalibrateButton, L"Start Calibrating");
	isCalibrating = false;
}

void OpenLiveWindow()
{
	if (isCalibrating || isCapturing) return;
	SetWindowText(hLabel, TEXT("Opening live window..."));

	
}

///////////////////////////////////////// File Picker ///////////////////////////////////////////

class CDialogEventHandler : public IFileDialogEvents, public IFileDialogControlEvents
{
public:
	// IUnknown methods
	IFACEMETHODIMP QueryInterface(REFIID riid, void** ppv)
	{
		static const QITAB qit[] = {
			QITABENT(CDialogEventHandler, IFileDialogEvents),
			QITABENT(CDialogEventHandler, IFileDialogControlEvents),
			{ 0 },
		};
		return QISearch(this, qit, riid, ppv);
	}

	IFACEMETHODIMP_(ULONG) AddRef()
	{
		return InterlockedIncrement(&_cRef);
	}

	IFACEMETHODIMP_(ULONG) Release()
	{
		long cRef = InterlockedDecrement(&_cRef);
		if (!cRef)
			delete this;
		return cRef;
	}

	// IFileDialogEvents methods
	IFACEMETHODIMP OnFileOk(IFileDialog *) { return S_OK; };
	IFACEMETHODIMP OnFolderChange(IFileDialog *) { return S_OK; };
	IFACEMETHODIMP OnFolderChanging(IFileDialog *, IShellItem *) { return S_OK; };
	IFACEMETHODIMP OnHelp(IFileDialog *) { return S_OK; };
	IFACEMETHODIMP OnSelectionChange(IFileDialog *) { return S_OK; };
	IFACEMETHODIMP OnShareViolation(IFileDialog *, IShellItem *, FDE_SHAREVIOLATION_RESPONSE *) { return S_OK; };
	IFACEMETHODIMP OnTypeChange(IFileDialog *pfd) { return S_OK; };
	IFACEMETHODIMP OnOverwrite(IFileDialog *, IShellItem *, FDE_OVERWRITE_RESPONSE *) { return S_OK; };

	// IFileDialogControlEvents methods
	IFACEMETHODIMP OnItemSelected(IFileDialogCustomize *pfdc, DWORD dwIDCtl, DWORD dwIDItem) { return S_OK; };
	IFACEMETHODIMP OnButtonClicked(IFileDialogCustomize *, DWORD) { return S_OK; };
	IFACEMETHODIMP OnCheckButtonToggled(IFileDialogCustomize *, DWORD, BOOL) { return S_OK; };
	IFACEMETHODIMP OnControlActivating(IFileDialogCustomize *, DWORD) { return S_OK; };

	CDialogEventHandler() : _cRef(1) { };
private:
	~CDialogEventHandler() { };
	long _cRef;
};

HRESULT CDialogEventHandler_CreateInstance(REFIID riid, void **ppv)
{
	*ppv = NULL;
	CDialogEventHandler *pDialogEventHandler = new (std::nothrow) CDialogEventHandler();
	HRESULT hr = pDialogEventHandler ? S_OK : E_OUTOFMEMORY;
	if (SUCCEEDED(hr))
	{
		hr = pDialogEventHandler->QueryInterface(riid, ppv);
		pDialogEventHandler->Release();
	}
	return hr;
}

HRESULT OpenReplayFilePicker()
{
	// CoCreate the File Open Dialog object.
	IFileDialog *pfd = NULL;
	HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&pfd));
	if (SUCCEEDED(hr))
	{
		// Create an event handling object, and hook it up to the dialog.
		IFileDialogEvents *pfde = NULL;
		hr = CDialogEventHandler_CreateInstance(IID_PPV_ARGS(&pfde));
		if (SUCCEEDED(hr))
		{
			// Hook up the event handler.
			DWORD dwCookie;
			hr = pfd->Advise(pfde, &dwCookie);
			if (SUCCEEDED(hr))
			{
				// Set the options on the dialog.
				DWORD dwFlags;

				// Before setting, always get the options first in order 
				// not to override existing options.
				hr = pfd->GetOptions(&dwFlags);
				if (SUCCEEDED(hr))
				{
					// In this case, get shell items only for file system items.
					hr = pfd->SetOptions(dwFlags | FOS_FORCEFILESYSTEM);
					if (SUCCEEDED(hr))
					{
						hr = pfd->Show(NULL);
						if (SUCCEEDED(hr))
						{
							// Obtain the result once the user clicks 
							// the 'Open' button.
							// The result is an IShellItem object.
							IShellItem *psiResult;
							hr = pfd->GetResult(&psiResult);
							if (SUCCEEDED(hr))
							{
								// We are just going to print out the 
								// name of the file for sample sake.
								PWSTR pszFilePath = NULL;
								hr = psiResult->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
								if (SUCCEEDED(hr))
								{
									SetWindowText(hLabel, Debug::Log(L"File: %s", pszFilePath).c_str());
									CoTaskMemFree(pszFilePath);
								}
								psiResult->Release();
							}
						}
					}
				}
				// Unhook the event handler.
				pfd->Unadvise(dwCookie);
			}
			pfde->Release();
		}
		pfd->Release();
	}
	return hr;
}


[MTAThread] // inits WinRT runtime
int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPTSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_RCUTILITY, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_RCUTILITY));
		
	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int) msg.wParam;
}


//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_RCUTILITY));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_RCUTILITY);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // Store instance handle in our global variable

   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   hLabel = CreateWindow(TEXT("static"), TEXT(""), WS_CHILD | WS_VISIBLE, 10, 10, 600, 20, hWnd, (HMENU)3, NULL, NULL);
   hCaptureButton = CreateWindow(TEXT("button"), TEXT("Start Capture"), WS_CHILD | WS_VISIBLE, 10, 60, 140, 50, hWnd, (HMENU)IDB_CAPTURE, NULL, NULL);
   hCalibrateButton = CreateWindow(TEXT("button"), TEXT("Start Calibration"), WS_CHILD | WS_VISIBLE, 170, 60, 140, 50, hWnd, (HMENU)IDB_CALIBRATE, NULL, NULL);
   hLiveButton = CreateWindow(TEXT("button"), TEXT("Live"), WS_CHILD | WS_VISIBLE, 330, 60, 140, 50, hWnd, (HMENU)IDB_LIVE, NULL, NULL);
   hReplayButton = CreateWindow(TEXT("button"), TEXT("Replay"), WS_CHILD | WS_VISIBLE, 490, 60, 140, 50, hWnd, (HMENU)IDB_REPLAY, NULL, NULL);

   ShowWindow(hWnd, nCmdShow);	
   UpdateWindow(hWnd);
   
   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		case IDB_CAPTURE:
			isCapturing ? StopCapture() : StartCapture();
			break;
		case IDB_CALIBRATE:
			isCalibrating ? StopCalibration() : StartCalibration();
			break;
		case IDB_LIVE:
			OpenLiveWindow();
			break;
		case IDB_REPLAY:
			OpenReplayFilePicker();
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO: Add any drawing code here...
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_CLOSE:
		StopCapture();
		DestroyWindow(hWnd);
		break;
	case WM_LBUTTONUP:
		
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
