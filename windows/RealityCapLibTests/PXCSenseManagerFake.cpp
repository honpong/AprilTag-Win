#include "PXCSenseManagerFake.h"
#include "libpxcimu.h"
#include <thread>

PXCCapture::Sample* PXCAPI PXCSenseManagerFake::QuerySample(pxcUID mid)
{
    static PXCCapture::Sample sample;
    static PXCImageFake fakeImage;
    fakeImage.SetTimeStamp(10000000);

    sample.color = &fakeImage;
    sample.depth = &fakeImage;

    return &sample;
}

pxcStatus PXCAPI PXCSenseManagerFake::AcquireFrame(pxcBool ifall, pxcI32 timeout)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return fakeStatus;
}

void* PXCAPI PXCImageFake::QueryInstance(pxcUID cuid)
{
    static PXCMetadataFake fakeMetadata;
    return &fakeMetadata;
}

PXCImage::ImageInfo PXCAPI PXCImageFake::QueryInfo(void)
{
    return ImageInfo{ 640, 480, PIXEL_FORMAT_ANY, 0 }; // content is irrelevant
}

pxcStatus PXCAPI PXCMetadataFake::QueryBuffer(pxcUID id, pxcBYTE * buffer, pxcI32 size)
{

    static imu_sample_t sample1 = { 10000000, {.1, .1, .1} };
    static imu_sample_t sample2 = { 10000001, {.2, .2, .2} };
    static imu_sample_t sample3 = { 10000002, {.3, .3, .3} };

    imu_sample_t* sample  = (imu_sample_t*)buffer;
    sample[0] = sample3;
    sample[1] = sample2;
    sample[2] = sample1;

    return PXC_STATUS_NO_ERROR;
}
