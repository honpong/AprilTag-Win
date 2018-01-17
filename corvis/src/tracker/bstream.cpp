/********************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION This software is supplied under the
terms of a license agreement or nondisclosure agreement with Intel Corporation
and may not be copied or disclosed except in accordance with the terms of that
agreement.
Copyright(c) 2016-2017 Intel Corporation. All Rights Reserved.

*********************************************************************************/

#include "bstream.h"

bool bstream_writer::save_sorted = false;

bstream_reader & operator >> (bstream_reader & cur_stream, transformation &transform) {
    cur_stream >> transform.T[0] >> transform.T[1] >> transform.T[2];
    cur_stream >> transform.Q.w() >> transform.Q.x() >> transform.Q.y() >> transform.Q.z();
    return cur_stream;
}

bstream_writer & operator << (bstream_writer & cur_stream, const transformation &transform) {
    cur_stream << transform.T[0] << transform.T[1] << transform.T[2];
    cur_stream << transform.Q.w() << transform.Q.x() << transform.Q.y() << transform.Q.z();
    return cur_stream;
}
