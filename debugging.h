#pragma once

#include <ntddk.h>
#include <cstdint>

#include "structs.h"

bool SetupBreakpoint(uint64_t virtualAddress);
