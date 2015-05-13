//
// MainPage.xaml.cpp
// Implementation of the MainPage class.
//

#include "pch.h"
#include "MainPage.xaml.h"

using namespace RCUtility;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;

typedef enum AppState
{
	Idle,
	Calibrating,
	Capturing,
	Live,
	Replay
};

AppState appState = Idle;

MainPage::MainPage()
{
	InitializeComponent();
}

void RCUtility::MainPage::buttonCalibration_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	if (appState == Idle) StartCalibration();
	else if (appState = Calibrating) StopCalibration();
}


void RCUtility::MainPage::buttonCapture_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	if (appState == Idle) StartCapture();
	else if (appState == Capturing) StopCapture();
}


void RCUtility::MainPage::buttonLive_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	if (appState == Idle) BeginLiveVis();
	else if (appState == Live) EndLiveVis();
}


void RCUtility::MainPage::buttonReplay_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	if (appState == Idle) BeginReplay();
	else if (appState == Replay) EndReplay();
}

void RCUtility::MainPage::StartCapture()
{
	if (appState != Idle) return;

	buttonCapture->Content = "Stop Capture";
	statusText->Text = "Starting capture...";
	
	bool result = true;

	if (!result)
	{
		statusText->Text = "Failed to start sensors";
		return;
	}

	if (result)
	{
		statusText->Text = "Capturing.";
	}
	else
	{
		statusText->Text = "Failed to start capture";
		return;
	}

	if (result) appState = Capturing;
}

void RCUtility::MainPage::StopCapture()
{
	if (appState != Capturing) return;
	statusText->Text = "Stopping capture...";
	
	statusText->Text = "Capture complete.";
	buttonCapture->Content = "Start Capture";
	appState = Idle;
}

void RCUtility::MainPage::StartCalibration()
{
	if (appState != Idle) return;
	statusText->Text = "Starting calibration...";

	bool result = true;

	if (result)
	{
		buttonCalibration->Content = "Stop Calibrating";
		appState = Calibrating;
	}
	else
	{
		statusText->Text = "Failed to start calibration.";
	}
}

void RCUtility::MainPage::StopCalibration()
{
	if (appState != Calibrating) return;
	statusText->Text = "Stopping calibration...";
	
	statusText->Text = "Calibration complete.";
	buttonCalibration->Content = "Start Calibrating";
	appState = Idle;
}

bool RCUtility::MainPage::OpenVisualizationWindow()
{
	if (appState != Idle) return false;
	
	return true;
}

void RCUtility::MainPage::BeginLiveVis()
{
	if (appState != Idle) return;
	if (!OpenVisualizationWindow()) return;
	appState = Live;
	statusText->Text = "Beginning live visualization...";

	// run filter
}

void RCUtility::MainPage::EndLiveVis()
{
	if (appState != Live) return;
	appState = Idle;
	statusText->Text = "";
}

void RCUtility::MainPage::BeginReplay()
{
	if (appState != Idle) return;
	if (!OpenVisualizationWindow()) return;
	appState = Replay;
	statusText->Text = "Beginning replay visualization...";

	// do opengl stuff
}

void RCUtility::MainPage::EndReplay()
{
	if (appState != Replay) return;
	appState = Idle;
	statusText->Text = "";
}