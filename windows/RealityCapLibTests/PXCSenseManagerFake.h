#pragma once

#include "pxcsensemanager.h"
#include "pxcmetadata.h"

class PXCImageFake : public PXCImage
{
public:
    PXCImageFake() {};

    pxcStatus fakeStatus = PXC_STATUS_NO_ERROR;

    virtual void* PXCAPI QueryInstance(pxcUID cuid);
    virtual pxcStatus PXCAPI AcquireAccess(Access access, PixelFormat format, Option options, ImageData *data) { return fakeStatus; };
    virtual pxcStatus PXCAPI ImportData(ImageData *data, pxcEnum flags) { return fakeStatus; };
    virtual pxcStatus PXCAPI ExportData(ImageData *data, pxcEnum flags) { return fakeStatus; };
    virtual pxcStatus PXCAPI CopyImage(PXCImage *src_image) { return fakeStatus; };
    virtual void PXCAPI SetOptions(Option options) {};
    virtual void PXCAPI SetStreamType(pxcEnum streamType) {};
    virtual void PXCAPI SetTimeStamp(pxcI64 ts) {};
    virtual Option PXCAPI QueryOptions(void) { return OPTION_ANY; };
    virtual pxcEnum PXCAPI QueryStreamType(void) { return 0; };
    virtual pxcI64 PXCAPI QueryTimeStamp(void) { return 0; };
    virtual ImageInfo PXCAPI QueryInfo(void);
    virtual pxcStatus   PXCAPI  ReleaseAccess(ImageData *data) { return fakeStatus; };
    virtual void  PXCAPI Release(void) {};
};

class PXCMetadataFake :public PXCMetadata 
{
public:
    pxcStatus fakeStatus = PXC_STATUS_NO_ERROR;

    virtual void* PXCAPI QueryInstance(pxcUID cuid) { return 0; };
    virtual pxcUID PXCAPI QueryUID(void) { return 0; };
    virtual pxcUID PXCAPI QueryMetadata(pxcI32 idx) { return 0; };
    virtual pxcStatus PXCAPI DetachMetadata(pxcUID id) { return fakeStatus; };
    virtual pxcStatus PXCAPI AttachBuffer(pxcUID id, pxcBYTE *buffer, pxcI32 size) { return fakeStatus; };
    virtual pxcI32 PXCAPI QueryBufferSize(pxcUID id) { return 0; };
    virtual pxcStatus PXCAPI QueryBuffer(pxcUID id, pxcBYTE *buffer, pxcI32 size);
    virtual pxcStatus PXCAPI AttachSerializable(pxcUID id, PXCBase *instance) { return fakeStatus; };
    virtual pxcStatus PXCAPI CreateSerializable(pxcUID id, pxcUID cuid, void **instance) { return fakeStatus; };
    virtual void  PXCAPI Release(void) {};
};

class PXCSenseManagerFake : public PXCSenseManager
{
public:
    PXCSenseManagerFake() {};
    virtual ~PXCSenseManagerFake() {};
    void operator delete(void* pthis) {};

    pxcStatus fakeStatus = PXC_STATUS_NO_ERROR;

    virtual PXCSession* PXCAPI QuerySession(void) override { return nullptr; };
    virtual PXCCaptureManager* PXCAPI QueryCaptureManager(void) override { return nullptr; };
    virtual PXCCapture::Sample* PXCAPI QuerySample(pxcUID mid) override;
    virtual PXCBase* PXCAPI QueryModule(pxcUID mid) override { return nullptr; };
    virtual pxcStatus PXCAPI Init(Handler *handler) override { return fakeStatus; };
    __inline pxcStatus Init(void) { return Init(0); }
    virtual pxcStatus PXCAPI StreamFrames(pxcBool blocking) override { return fakeStatus; };
    virtual pxcBool PXCAPI IsConnected(void) override { return true; };
    virtual pxcStatus PXCAPI AcquireFrame(pxcBool ifall, pxcI32 timeout) override;
    virtual void PXCAPI FlushFrame(void) override {};
    virtual void PXCAPI ReleaseFrame(void) override {};
    virtual void PXCAPI Close(void) override {};
    virtual pxcStatus PXCAPI EnableStreams(PXCVideoModule::DataDesc *sdesc) override { return fakeStatus; };
    virtual pxcStatus PXCAPI EnableModule(pxcUID mid, PXCSession::ImplDesc *mdesc) override { return fakeStatus; };
    virtual void PXCAPI PauseModule(pxcUID mid, pxcBool pause) override {};
    virtual void* PXCAPI QueryInstance(pxcUID cuid) override { return 0; };
    virtual void  PXCAPI Release(void) override {};

    __inline static PXCSenseManagerFake* CreateInstance(void) {
        PXCSenseManagerFake* fake = new PXCSenseManagerFake();
        return fake;
    };
};