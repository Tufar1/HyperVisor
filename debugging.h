#pragma once

#include <ntddk.h>
#include <cstdint>

#include "structs.h"
#include "memory.h"

bool SetupBreakpoint(uint64_t virtualAddress, uint64_t CR3);
