#include "packet_io.h"
#include <stdio.h>
#include <stdlib.h>

static int packets_read    = 0;
static int packets_written = 0;
int        bytes_allocated = 0;

bool packet_io_write(packet_t * packet)
{
    //    printf("writing packet type %d with %lu bytes\n", packet->header.type, packet->header.bytes);
    if(!usb_blocking_write(ENDPOINT_DEV_OUT, (uint8_t *)packet,
                           sizeof(packet_header_t)))
        return false;

    uint64_t bytes_left = packet->header.bytes - sizeof(packet_header_t);
    if(!usb_blocking_write(ENDPOINT_DEV_OUT, (uint8_t *)&packet->data,
                           bytes_left))
        return false;
    packets_written++;
    if(packets_written % 1000 == 0)
        printf("%d packets written\n", packets_written);
    return true;
}

bool packet_io_unblock_read()
{
    return usb_unblock(ENDPOINT_DEV_IN);
}

bool packet_io_read(packet_t ** packet)
{
    packet_header_t header;
    int             sc;
    //    printf("reading header %lu bytes\n", sizeof(packet_header_t));
    if(!usb_blocking_read(ENDPOINT_DEV_IN, (uint8_t *)&header,
                          sizeof(packet_header_t)))
        return false;
    //    printf("header %lu %u %u %llu\n", header.bytes, header.type, header.sensor_id, header.time);
    switch(header.type) {
        default:
            *packet = (packet_t *)malloc(header.bytes);
            bytes_allocated += header.bytes;
            //printf("Malloc: %d allocated: %d\n", header.bytes, bytes_allocated);
    }
    (*packet)->header   = header;
    uint64_t bytes_left = (*packet)->header.bytes - sizeof(packet_header_t);
    //    printf("trying to read %lu", bytes_left);
    if(!usb_blocking_read(ENDPOINT_DEV_IN, (uint8_t *)&(*packet)->data,
                          bytes_left)) {
        packet_io_free(*packet);
        return false;
    }
    packets_read++;
    return true;
}

void packet_io_free(packet_t * packet)
{
    int sc;
    switch(packet->header.type) {
        default:
            bytes_allocated -= packet->header.bytes;
            //printf("free: %ld allocated: %d\n", packet->header.bytes, bytes_allocated);
            free(packet);
    }
}
