#pragma once

#include "pxcsensemanager.h"

class PXCSenseManagerFake : public PXCSenseManager
{
public:
    PXCSenseManagerFake() {};
    virtual ~PXCSenseManagerFake() {};
    void operator delete(void* pthis) {};

    virtual PXCSession* PXCAPI QuerySession(void) { return nullptr; };
    virtual PXCCaptureManager* PXCAPI QueryCaptureManager(void) { return nullptr; };
    virtual PXCCapture::Sample* PXCAPI QuerySample(pxcUID mid) { return nullptr; };
    virtual PXCBase* PXCAPI QueryModule(pxcUID mid) { return nullptr; };
    virtual pxcStatus PXCAPI Init(Handler *handler) { return PXC_STATUS_NO_ERROR; };
    virtual pxcStatus PXCAPI Init() { return PXC_STATUS_NO_ERROR; };
    virtual pxcStatus PXCAPI StreamFrames(pxcBool blocking) { return PXC_STATUS_NO_ERROR; };
    virtual pxcBool PXCAPI IsConnected(void) { return true; };
    virtual pxcStatus PXCAPI AcquireFrame(pxcBool ifall, pxcI32 timeout) { return PXC_STATUS_DATA_UNAVAILABLE; };
    virtual void PXCAPI FlushFrame(void) {};
    virtual void PXCAPI ReleaseFrame(void) {};
    virtual void PXCAPI Close(void) {};
    virtual pxcStatus PXCAPI EnableStreams(PXCVideoModule::DataDesc *sdesc) { return PXC_STATUS_NO_ERROR; };
    virtual pxcStatus PXCAPI EnableModule(pxcUID mid, PXCSession::ImplDesc *mdesc) { return PXC_STATUS_NO_ERROR; };
    virtual void PXCAPI PauseModule(pxcUID mid, pxcBool pause) {};
    virtual void* PXCAPI QueryInstance(pxcUID cuid) { return 0; };
    virtual void  PXCAPI Release(void) {};

    __inline static PXCSenseManagerFake* CreateInstance(void) {
        PXCSenseManagerFake* fake = new PXCSenseManagerFake();
        return fake;
    };
};