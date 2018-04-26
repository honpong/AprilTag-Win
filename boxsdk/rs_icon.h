/*******************************************************************************
 
 Copyright 2015-2018 Intel Corporation.
 
 This software and the related documents are Intel copyrighted materials,
 and your use of them is governed by the express license under which
 they were provided to you. Unless the License provides otherwise,
 you may not use, modify, copy, publish, distribute, disclose or transmit
 this software or the related documents without Intel's prior written permission.
 
 This software and the related documents are provided as is, with no express or
 implied warranties, other than those that are expressly stated in the License.

*******************************************************************************/
#ifndef rs_icon_h
#define rs_icon_h

#define get_icon_width(NAME) (int)(sizeof(icon_ ## NAME  ## _data[0])/sizeof(icon_ ## NAME ## _data[0][0]))
#define get_icon_height(NAME) (int)(sizeof(icon_ ## NAME ## _data)/sizeof(icon_ ## NAME ## _data[0]))
#define get_icon_data(NAME) ((const char*)(icon_ ## NAME ## _data))
#define get_icon(NAME) get_icon_data(NAME), get_icon_width(NAME), get_icon_height(NAME)

extern const unsigned int icon_realsense_data[54][320][1];

extern const unsigned char icon_reset_data[64][64][1];

extern const unsigned char icon_close_data[64][64][1];

extern const unsigned char icon_number_data[73][674][1];

extern const unsigned char icon_mm3_data[73][194][1];


#endif
