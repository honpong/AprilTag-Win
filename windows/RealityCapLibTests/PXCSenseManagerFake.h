#pragma once

#include "pxcsensemanager.h"

class PXCSenseManagerFake : public PXCSenseManager
{
public:
    PXCSenseManagerFake() {};
    virtual ~PXCSenseManagerFake() {};
    void operator delete(void* pthis) {};

    pxcStatus FakeStatus = PXC_STATUS_NO_ERROR;

    virtual PXCSession* PXCAPI QuerySession(void) override { return nullptr; };
    virtual PXCCaptureManager* PXCAPI QueryCaptureManager(void) override { return nullptr; };
    virtual PXCCapture::Sample* PXCAPI QuerySample(pxcUID mid) override { return nullptr; };
    virtual PXCBase* PXCAPI QueryModule(pxcUID mid) override { return nullptr; };
    virtual pxcStatus PXCAPI Init(Handler *handler) override { return FakeStatus; };
    __inline pxcStatus Init(void) { return Init(0); }
    virtual pxcStatus PXCAPI StreamFrames(pxcBool blocking) override { return FakeStatus; };
    virtual pxcBool PXCAPI IsConnected(void) override { return true; };
    virtual pxcStatus PXCAPI AcquireFrame(pxcBool ifall, pxcI32 timeout) override { return PXC_STATUS_DATA_UNAVAILABLE; };
    virtual void PXCAPI FlushFrame(void) override {};
    virtual void PXCAPI ReleaseFrame(void) override {};
    virtual void PXCAPI Close(void) override {};
    virtual pxcStatus PXCAPI EnableStreams(PXCVideoModule::DataDesc *sdesc) override { return FakeStatus; };
    virtual pxcStatus PXCAPI EnableModule(pxcUID mid, PXCSession::ImplDesc *mdesc) override { return FakeStatus; };
    virtual void PXCAPI PauseModule(pxcUID mid, pxcBool pause) override {};
    virtual void* PXCAPI QueryInstance(pxcUID cuid) override { return 0; };
    virtual void  PXCAPI Release(void) override {};

    __inline static PXCSenseManagerFake* CreateInstance(void) {
        PXCSenseManagerFake* fake = new PXCSenseManagerFake();
        return fake;
    };
};