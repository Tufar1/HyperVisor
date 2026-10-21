#pragma once

#include <cstdint>

#include "structs.h"
#include "memory.h"

extern "C" void ReadGDT(void* out);

uint64_t GetTargetPAFromVA(uint64_t virtualAddress, uint64_t CR3);
uint64_t GetCR3FromName(char* name, uint64_t& EProcess);
uint64_t GetEmptyIndex(uint64_t tableVA);
uint64_t GetTargetModule(uint64_t EProcess, const wchar_t* targetName);
VmcbSegment GetSegmentFromGDT(uint16_t selector);
void DumpVMCB(void* vmcb, size_t from, size_t to);

inline void invlpg(void* addr) {
    asm volatile("invlpg (%0)" :: "r"(addr) : "memory");
}

namespace EProcess{
  constexpr uint64_t CR3 = 0x28;
  constexpr uint64_t ActiveProcessLinksOffset = 0x1D8;
  constexpr uint64_t ImageFileNameOffset = 0x338;
  constexpr uint64_t ExitTimeOffset = 0x5C0;
  constexpr uint64_t PEB = 0x2E0;
  constexpr uint64_t SectionBaseAddress = 0x2B0;
}

namespace Peb{
  constexpr uint64_t Ldr = 0x18;
}

namespace ldrData{
  constexpr uint64_t InLoadOrderModuleList = 0x10;
}

namespace ldrDataTableEntry {
  constexpr uint64_t DllBase = 0x30;
  constexpr uint64_t BaseDllName = 0x58;
}

namespace imageDosHeader {
  constexpr uint64_t e_lfanew = 0x3C;
}

namespace imageFileHeader {
  constexpr uint64_t NumberOfSections = 0x06;
  constexpr uint64_t Size = 0x14;
}

namespace imageNtHeaders {
  constexpr uint64_t OptionalHeader = 0x18;
}

namespace imageSectionHeader {
  constexpr uint64_t SizeOfRawData = 0x10;
  constexpr uint64_t VirtualSize = 0x08;
  constexpr uint64_t VirtualAddress = 0x0C;
  constexpr uint64_t Characteristics = 0x24;
  constexpr uint64_t Size = 0x28;
}
