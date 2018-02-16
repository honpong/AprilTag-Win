#include "packet_io.h"
#include <stdio.h>
#include <stdlib.h>
#include "tm2_outcall.h"
#include "usb_definitions.h"

static int packets_read    = 0;
static int packets_written = 0;
int        bytes_allocated = 0;

bool packet_io_write(const rc_packet_t &packet)
{
    return packet_io_write_parts(&packet->header, packet->data);
}

bool packet_io_write_parts(const packet_header_t *header, const uint8_t *content)
{
    // printf("writing packet type %d with %lu bytes\n", packet->header.type, packet->header.bytes);
    if(!usb_blocking_write(ENDPOINT_DEV_OUT, (uint8_t *)header,
        sizeof(packet_header_t)))
        return false;

    uint64_t bytes_left = header->bytes - sizeof(packet_header_t);
    if(bytes_left && !usb_blocking_write(ENDPOINT_DEV_OUT, content, bytes_left))
        return false;
    return true;
}

bool packet_io_read(rc_packet_t &packet)
{
    packet_header_t header;
    int             sc;
    //    printf("reading header %lu bytes\n", sizeof(packet_header_t));
    if(!usb_blocking_read(ENDPOINT_DEV_IN, (uint8_t *)&header,
                          sizeof(packet_header_t))) {
        printf("Error: failed to read header after packet of type %d\n", header.type);
        return false;
    }
    //    printf("header %lu %u %u %llu\n", header.bytes, header.type, header.sensor_id, header.time);
    switch(header.type) {
        default:
            packet = { (packet_t *)malloc(header.bytes), free };
            if(!packet) return false;
            bytes_allocated += header.bytes;
            //printf("Malloc: %d allocated: %d\n", header.bytes, bytes_allocated);
    }
    packet->header   = header;
    uint64_t bytes_left = packet->header.bytes - sizeof(packet_header_t);
    //    printf("trying to read %lu", bytes_left);
    if(bytes_left && !usb_blocking_read(ENDPOINT_DEV_IN, (uint8_t *)packet->data,
                          bytes_left)) {
        printf("Error: failed to read body of packet type %d\n", header.type);
        return false;
    }
    packets_read++;
    return true;
}
