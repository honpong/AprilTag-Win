#pragma once

#include <memory>
#include "packet.h"

typedef std::unique_ptr<packet_t, decltype(&free)> rc_packet_t;

bool packet_io_write(const rc_packet_t &packet);
bool packet_io_write_parts(const packet_header_t *header, const uint8_t *content);
bool packet_io_read(rc_packet_t &packet);
