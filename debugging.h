#pragma once

#include <ntddk.h>
#include <cstdint>

#include "structs.h"
#include "memory.h"
#include "vmcb.h"

inline NptBreakpoint breakpoints[64] = {};
inline uint16_t breakpointCount = 0;

bool SetNX(uint64_t guestPA, bool value);
bool SetupBreakpoint(uint64_t virtualAddress, uint64_t targetCR3, bool doBlocking);
