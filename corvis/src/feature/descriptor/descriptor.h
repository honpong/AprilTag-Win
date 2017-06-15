#pragma once

#ifndef DESCRIPTOR
#define DESCRIPTOR patch_descriptor
#endif

#define TO_STR(d) #d
#define DOT_H(d) TO_STR(d.h)
#include DOT_H(DESCRIPTOR)
