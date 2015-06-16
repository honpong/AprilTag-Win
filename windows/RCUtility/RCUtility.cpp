// RCUtility.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "RCUtility.h"
#include "Debug.h"
#include <shellapi.h>
#include "TrackerManager.h"
#include <shlobj.h>
#include "RCFactory.h"
#include <wchar.h>
#include "visualization.h"
#include "render_data.h"
#include "FilePicker.h"

using namespace RealityCap;
using namespace std;

#define MAX_LOADSTRING 100

typedef enum
{
    Idle,
    Calibrating,
    Capturing,
    Live,
    Replay
} AppState;

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name
RCFactory factory;
auto trackMan = factory.CreateTrackerManager();
HWND hStatusLabel;
HWND hProgressLabel;
HWND hCaptureButton;
HWND hCalibrateButton;
HWND hLiveButton;
HWND hReplayButton;
AppState appState = Idle;
HWND hWnd;

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

void EnterCapturingState();
void ExitCapturingState();
void EnterCalibratingState();
void ExitCalibratingState();
void EnterLiveVisState();
void ExitLiveVisState();
void EnterReplayingState(const PWSTR filePath);
void ExitReplayingState();
wstring GetExePath();

render_data visualization_data;
visualization vis(&visualization_data);

class MyRepDelegate : public TrackerManagerDelegate
{
public:
    virtual void OnError(int code) override
    {
        Debug::Log(L"ERROR %d", code);
    };
    virtual void OnStatusUpdated(int status) override
    {
        Debug::Log(L"STATUS %d", status);
    };
    virtual void OnDataUpdated(rc_Timestamp time, rc_Pose pose, rc_Feature *features, size_t feature_count) override
    {
        Debug::Log(L"OnDataUpdated time %llu %f %f %f (%u features)", time, pose[3], pose[7], pose[11], feature_count);
        if (feature_count) {
            Debug::Log(L"First feature %f %f %f", features[0].world.x, features[0].world.y, features[0].world.z);
        }
        visualization_data.update_data(time, pose, features, feature_count);
    };
};

MyRepDelegate repDelegate;

class MyCalDelegate : public TrackerManagerDelegate
{
public: // these methods get called from the sensor manager thread
    virtual void OnStatusUpdated(int status) override
    {
        switch (status)
        {
        case 0:
            SetWindowText(hStatusLabel, TEXT("Calibration complete."));
            PostMessage(hWnd, IDB_EXIT_CALIBRATION, NULL, NULL); // message will be processed on window's main thread
            break;
        case 1:
            SetWindowText(hStatusLabel, TEXT("Place the device flat on a table."));
            break;
        case 5:
            SetWindowText(hStatusLabel, TEXT("Hold device steady in portrait orientation."));
            break;
        case 6:
            SetWindowText(hStatusLabel, TEXT("Hold device steady in landscape orientation."));
            break;
        default:
            break;
        }
    };

    virtual void OnProgressUpdated(float progress) override
    {
        if (progress < 1. && appState == Calibrating)
        {
            wchar_t title[1024];
            _snwprintf_s(title, 1024, L"Progress %2.0f%%", progress * 100);
            SetWindowText(hProgressLabel, title);
        }
    };

    virtual void OnError(int errorCode) override
    {
        switch (errorCode)
        {
        case 1:
            // not fatal
            SetWindowText(hStatusLabel, TEXT("Vision error."));
            break;
        case 2:
            ExitCalibratingState();
            SetWindowText(hStatusLabel, TEXT("Speed error."));
            break;
        case 3:
            ExitCalibratingState();
            SetWindowText(hStatusLabel, TEXT("Fatal error."));
            break;
        default:
            break;
        }
    };
};

MyCalDelegate calDelegate;

void EnterCapturingState()
{
    if (appState != Idle) return;

    SetWindowText(hCaptureButton, TEXT("Stop Capture"));
    SetWindowText(hStatusLabel, TEXT("Starting capture..."));
    bool result;

    wstring captureFile = GetExePath().append(L"\\capture.rssdk");
    result = trackMan->StartRecording(captureFile.c_str());
    if (result)
    {
        SetWindowText(hStatusLabel, TEXT("Capturing."));
    }
    else
    {
        SetWindowText(hStatusLabel, TEXT("Failed to start capture"));
        return;
    }

    if (result) appState = Capturing;
}

void ExitCapturingState()
{
    if (appState != Capturing) return;
    SetWindowText(hStatusLabel, TEXT("Stopping capture..."));
    trackMan->StopSensors();
    SetWindowText(hStatusLabel, TEXT("Capture complete."));
    SetWindowText(hCaptureButton, TEXT("Start Capture"));
    appState = Idle;
}

void EnterCalibratingState()
{
    if (appState != Idle) return;
    SetWindowText(hStatusLabel, TEXT("Starting calibration..."));

    trackMan->SetDelegate(&calDelegate);
    bool result = trackMan->StartCalibration();
    if (result)
    {
        SetWindowText(hCalibrateButton, TEXT("Stop Calibrating"));
        appState = Calibrating;
    }
    else
    {
        SetWindowText(hStatusLabel, TEXT("Failed to start calibration."));
    }
}

void ExitCalibratingState()
{
    if (appState != Calibrating) return;
    SetWindowText(hStatusLabel, TEXT("Stopping calibration..."));
    trackMan->Stop();
    SetWindowText(hStatusLabel, TEXT("Calibration stopped."));
    SetWindowText(hProgressLabel, TEXT(""));
    SetWindowText(hCalibrateButton, TEXT("Start Calibrating"));
    appState = Idle;
}

void EnterLiveVisState()
{
    if (appState != Idle) return;
    SetWindowText(hStatusLabel, TEXT("Starting live visualization..."));
    trackMan->SetDelegate(&repDelegate);
    bool result = trackMan->Start();
    if (result)
    {
        SetWindowText(hStatusLabel, TEXT("Running live visualization..."));
        SetWindowText(hLiveButton, TEXT("Stop Live"));
        appState = Live;
        vis.start(); // blocks until vis window is closed
        ExitLiveVisState();
    }
    else
    {
        SetWindowText(hStatusLabel, TEXT("Failed to start live visualization."));
    }
}

void ExitLiveVisState()
{
    if (appState != Live) return;
    SetWindowText(hStatusLabel, TEXT("Stopping live visualization..."));
    trackMan->Stop();
    visualization_data.reset();
    SetWindowText(hStatusLabel, TEXT("Live visualization stopped."));
    SetWindowText(hLiveButton, TEXT("Live"));
    appState = Idle;
}

void EnterReplayingState(const PWSTR filePath)
{
    appState = Replay;
    SetWindowText(hStatusLabel, TEXT("Beginning replay visualization..."));
    trackMan->SetDelegate(&repDelegate);
    bool result = trackMan->StartReplay(filePath, true);
    if (result)
    {
        SetWindowText(hStatusLabel, TEXT("Replaying..."));
        appState = Replay;
        vis.start(); // blocks until vis window is closed
        ExitReplayingState();
    }
    else
    {
        SetWindowText(hStatusLabel, TEXT("Failed to start Replay."));
    }
}

void ExitReplayingState()
{
    if (appState != Replay) return;
    SetWindowText(hStatusLabel, TEXT("Stopping replay..."));
    trackMan->Stop();
    visualization_data.reset();
    SetWindowText(hStatusLabel, TEXT("Replay stopped."));
    appState = Idle;
}

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
    if (appState != Idle) return S_FALSE;

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
                                    SetWindowText(hStatusLabel, pszFilePath);
                                    EnterReplayingState(pszFilePath);
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

wstring GetExePath()
{
    WCHAR buffer[MAX_PATH];
    GetModuleFileName(NULL, buffer, MAX_PATH);
    string::size_type pos = wstring(buffer).find_last_of(L"\\/");
    return wstring(buffer).substr(0, pos);
}

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
    if (!InitInstance(hInstance, nCmdShow))
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

    return (int)msg.wParam;
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

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_RCUTILITY));
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCE(IDC_RCUTILITY);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

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
    hInst = hInstance; // Store instance handle in our global variable

    hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, 660, 200, NULL, NULL, hInstance, NULL);

    if (!hWnd)
    {
        return FALSE;
    }

    hStatusLabel = CreateWindow(TEXT("static"), TEXT(""), WS_CHILD | WS_VISIBLE, 10, 10, 620, 20, hWnd, (HMENU)3, NULL, NULL);
    hProgressLabel = CreateWindow(TEXT("static"), TEXT(""), WS_CHILD | WS_VISIBLE, 10, 30, 620, 20, hWnd, (HMENU)3, NULL, NULL);
    hCaptureButton = CreateWindow(TEXT("button"), TEXT("Start Capture"), WS_CHILD | WS_VISIBLE, 10, 80, 140, 50, hWnd, (HMENU)IDB_CAPTURE, NULL, NULL);
    hCalibrateButton = CreateWindow(TEXT("button"), TEXT("Start Calibration"), WS_CHILD | WS_VISIBLE, 170, 80, 140, 50, hWnd, (HMENU)IDB_CALIBRATE, NULL, NULL);
    hLiveButton = CreateWindow(TEXT("button"), TEXT("Live"), WS_CHILD | WS_VISIBLE, 330, 80, 140, 50, hWnd, (HMENU)IDB_LIVE, NULL, NULL);
    hReplayButton = CreateWindow(TEXT("button"), TEXT("Replay"), WS_CHILD | WS_VISIBLE, 490, 80, 140, 50, hWnd, (HMENU)IDB_REPLAY, NULL, NULL);

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
        wmId = LOWORD(wParam);
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
            appState == Capturing ? ExitCapturingState() : EnterCapturingState();
            break;
        case IDB_CALIBRATE:
            if (appState == Calibrating) {
                ExitCalibratingState();
            } else {
                EnterCalibratingState();
            }
            break;
        case IDB_LIVE:
            EnterLiveVisState();
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
        EndPaint(hWnd, &ps);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_CLOSE:
        ExitCapturingState(); // TODO: handle other cases
        DestroyWindow(hWnd);
        break;
    case IDB_EXIT_CALIBRATION:
        if (appState == Calibrating) ExitCalibratingState();
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
