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
    static imu_sample_t sample4 = { 10000000,{ .1, .1, .1 } };
    static imu_sample_t sample5 = { 10000001,{ .2, .2, .2 } };
    static imu_sample_t sample6 = { 10000002,{ .3, .3, .3 } };
    static imu_sample_t sample7 = { 10000000,{ .1, .1, .1 } };
    static imu_sample_t sample8 = { 10000001,{ .2, .2, .2 } };
    static imu_sample_t sample9 = { 10000002,{ .3, .3, .3 } };
    static imu_sample_t sample10 = { 10000000,{ .1, .1, .1 } };
    static imu_sample_t sample11 = { 10000001,{ .2, .2, .2 } };
    static imu_sample_t sample12 = { 10000002,{ .3, .3, .3 } };
    static imu_sample_t sample13 = { 10000000,{ .1, .1, .1 } };
    static imu_sample_t sample14 = { 10000001,{ .2, .2, .2 } };
    static imu_sample_t sample15 = { 10000002,{ .3, .3, .3 } };
    static imu_sample_t sample16 = { 10000000,{ .1, .1, .1 } };

    imu_sample_t* sample  = (imu_sample_t*)buffer;
    sample[0] = sample1;
    sample[1] = sample2;
    sample[2] = sample3;
    sample[3] = sample4;
    sample[4] = sample5;
    sample[5] = sample6;
    sample[6] = sample7;
    sample[7] = sample8;
    sample[8] = sample9;
    sample[9] = sample10;
    sample[10] = sample11;
    sample[11] = sample12;
    sample[12] = sample13;
    sample[13] = sample14;
    sample[14] = sample15;
    sample[15] = sample16;

    return PXC_STATUS_NO_ERROR;
}
