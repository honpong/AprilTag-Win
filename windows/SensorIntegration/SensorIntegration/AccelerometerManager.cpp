
#include "stdafx.h"
#include "AccelerometerManager.h"

AccelerometerManager::AccelerometerManager(void)
{
};


AccelerometerManager::~AccelerometerManager(void)
{
};

HRESULT AccelerometerManager::Initialize()
{
	CComPtr<ISensorManager> pSensorManager;
	CComPtr<ISensorCollection> pSensorColl;

	// Create the sensor manager.
	HRESULT hr = ::CoCreateInstance(CLSID_SensorManager, 
							NULL, CLSCTX_INPROC_SERVER,
							IID_PPV_ARGS(&pSensorManager));

	if(hr == HRESULT_FROM_WIN32(ERROR_ACCESS_DISABLED_BY_POLICY))
	{
		// Unable to retrieve sensor manager due to group policy settings. Alert the user.
		return hr;
	}

	if(FAILED(hr)){
		return hr;
	}

	hr = pSensorManager->GetSensorsByType(SENSOR_TYPE_ACCELEROMETER_3D, &pSensorColl);
	if (FAILED(hr))
	{
		::MessageBox(NULL, L"No Motion Sensor", NULL, MB_OK);
		return hr;
	}

	ULONG ulCount = 0;
	hr = pSensorColl->GetCount(&ulCount);
	if (FAILED(hr))
	{
		::MessageBox(NULL, L"No Accelerometer Sensor", NULL, MB_OK);
		return hr;
	}
	
	hr = pSensorColl->GetAt(0, &pSensor);

	if(SUCCEEDED(hr)){
		hr = pSensor->SetEventSink(NULL);
	}

	return hr;
};

AccelSample AccelerometerManager::GetSample()
{
	CComPtr<ISensorDataReport> pReport;
    HRESULT hr = pSensor->GetData(&pReport);
	AccelSample sample;
	
	if(SUCCEEDED(hr))
	{
		SYSTEMTIME pTimestamp;
		hr = pReport->GetTimestamp(&pTimestamp);
		if (SUCCEEDED(hr)) sample.timestamp = pTimestamp;

		PROPVARIANT var = {};

		hr = pReport->GetSensorValue(SENSOR_DATA_TYPE_ACCELERATION_X_G, &var);

		if(SUCCEEDED(hr))
		{
			if(var.vt == VT_R8) sample.x = var.dblVal;
		}

		PropVariantClear(&var);

		hr = pReport->GetSensorValue(SENSOR_DATA_TYPE_ACCELERATION_Y_G, &var);

		if(SUCCEEDED(hr))
		{
			if(var.vt == VT_R8) sample.y = var.dblVal;
		}

		PropVariantClear(&var);

		hr = pReport->GetSensorValue(SENSOR_DATA_TYPE_ACCELERATION_Z_G, &var);

		if (SUCCEEDED(hr))
		{
			if (var.vt == VT_R8) sample.z = var.dblVal;
		}
	}
	return sample;
}

HRESULT AccelerometerManager::SetChangeSensitivity(double sensitivity)
{
	// create an IPortableDeviceValues container for holding the <Data Field, Sensitivity> tuples. 
	HRESULT hr;
	CComPtr<IPortableDeviceValues> pInSensitivityValues; 
	hr = ::CoCreateInstance(CLSID_PortableDeviceValues, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pInSensitivityValues));
	if (FAILED(hr)) 
	{ 
		::MessageBox(NULL, _T("Unable to CoCreateInstance() a PortableDeviceValues collection."), _T("Sensor C++ Sample"), MB_OK | MB_ICONERROR);
		return -1; 
	} 
	
	// fill in IPortableDeviceValues container contents here for each of X, Y, and Z axes. 
	PROPVARIANT pv; 
	PropVariantInit(&pv); 
	pv.vt = VT_R8; // COM type for (double) 
	pv.dblVal = sensitivity;
	pInSensitivityValues->SetValue(SENSOR_DATA_TYPE_ACCELERATION_X_G, &pv); 
	pInSensitivityValues->SetValue(SENSOR_DATA_TYPE_ACCELERATION_Y_G, &pv); 
	pInSensitivityValues->SetValue(SENSOR_DATA_TYPE_ACCELERATION_Z_G, &pv); 
	
	// create an IPortableDeviceValues container for holding the <SENSOR_PROPERTY_CHANGE_SENSITIVITY, pInSensitivityValues> tuple. 
	CComPtr<IPortableDeviceValues> pInValues; 
	hr = ::CoCreateInstance(CLSID_PortableDeviceValues, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pInValues)); 
	if (FAILED(hr)) 
	{ 
		::MessageBox(NULL, _T("Unable to CoCreateInstance() a PortableDeviceValues collection."), _T("Sensor C++ Sample"), MB_OK | MB_ICONERROR);     
		return -1;
	} 

	// fill it in 
	pInValues->SetIPortableDeviceValuesValue(SENSOR_PROPERTY_CHANGE_SENSITIVITY, pInSensitivityValues); 

	// now actually set the sensitivity 
	CComPtr<IPortableDeviceValues> pOutValues; 
	hr = pSensor->SetProperties(pInValues, &pOutValues); 
	if (FAILED(hr)) 
	{     
		::MessageBox(NULL, _T("Unable to SetProperties() for Sensitivity."), _T("Sensor C++ Sample"), MB_OK | MB_ICONERROR);     
		return -1; 
	} 
	
	// check to see if any of the setting requests failed 
	DWORD dwCount = 0; 
	hr = pOutValues->GetCount(&dwCount); 
	if (FAILED(hr) || (dwCount > 0)) 
	{     
		::MessageBox(NULL, _T("Failed to set one-or-more Sensitivity values."), _T("Sensor C++ Sample"), MB_OK | MB_ICONERROR);     
		return -1; 
	} 
	
	PropVariantClear(&pv);

	return hr;
}

