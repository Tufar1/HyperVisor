#pragma once
#include "vmcb.h"
#include <ntddk.h>
#include <cstdint>


bool SetupBreakpoint(uint64_t virtualAddress);

uint64_t GetTargetModuleBase();

uint64_t GetTargetPAFromVA(uint64_t virtualAddress);
uint64_t GetCR3FromName(char* name);

inline uint64_t targetCR3;
inline uint64_t targetEProcess;

union PML4E {
    std::uint64_t value;
    struct {
        std::uint64_t present : 1;
        std::uint64_t rw : 1;
        std::uint64_t us : 1;
        std::uint64_t pwt : 1;
        std::uint64_t pcd : 1;
        std::uint64_t accessed : 1;
        std::uint64_t ignored_0 : 1;
        std::uint64_t reserved_0 : 1;
        std::uint64_t ignored_1 : 3;
        std::uint64_t restart : 1;
        std::uint64_t pfn : 40;
        std::uint64_t ignored_2 : 11;
        std::uint64_t nx : 1;
    };
};

union PDPTE {
    std::uint64_t value;
    struct {
        std::uint64_t present : 1;
        std::uint64_t rw : 1;
        std::uint64_t us : 1;
        std::uint64_t pwt : 1;
        std::uint64_t pcd : 1;
        std::uint64_t accessed : 1;
        std::uint64_t ignored_0 : 1;
        std::uint64_t page_size : 1;
        std::uint64_t ignored_1 : 3;
        std::uint64_t restart : 1;
        std::uint64_t pfn : 40;
        std::uint64_t ignored_2 : 11;
        std::uint64_t nx : 1;
    };
};

union PDPTE_1GB {
    std::uint64_t value;
    struct {
        std::uint64_t present : 1;
        std::uint64_t rw : 1;
        std::uint64_t us : 1;
        std::uint64_t pwt : 1;
        std::uint64_t pcd : 1;
        std::uint64_t accessed : 1;
        std::uint64_t dirty : 1;
        std::uint64_t page_size : 1;
        std::uint64_t global : 1;
        std::uint64_t ignored_0 : 2;
        std::uint64_t restart : 1;
        std::uint64_t pat : 1;
        std::uint64_t reserved_0 : 17;
        std::uint64_t pfn : 22;
        std::uint64_t ignored_1 : 7;
        std::uint64_t mpk : 4;
        std::uint64_t nx : 1;
  };
};

union PDE {
    std::uint64_t value;
    struct {
        std::uint64_t present : 1;
        std::uint64_t rw : 1;
        std::uint64_t us : 1;
        std::uint64_t pwt : 1;
        std::uint64_t pcd : 1;
        std::uint64_t accessed : 1;
        std::uint64_t ignored_0 : 1;
        std::uint64_t page_size : 1;
        std::uint64_t ignored_1 : 3;
        std::uint64_t restart : 1;
        std::uint64_t pfn : 40;
        std::uint64_t ignored_2 : 11;
        std::uint64_t nx : 1;
    };
};

union PDE_2MB {
    std::uint64_t value;
    struct {
        std::uint64_t present : 1;
        std::uint64_t rw : 1;
        std::uint64_t us : 1;
        std::uint64_t pwt : 1;
        std::uint64_t pcd : 1;
        std::uint64_t accessed : 1;
        std::uint64_t dirty : 1;
        std::uint64_t page_size : 1;
        std::uint64_t global : 1;
        std::uint64_t ignored_0 : 2;
        std::uint64_t restart : 1;
        std::uint64_t pat : 1;
        std::uint64_t reserved_0 : 8;
        std::uint64_t pfn : 31;
        std::uint64_t ignored_1 : 7;
        std::uint64_t mpk : 4;
        std::uint64_t nx : 1;
    };
};

union PTE {
    std::uint64_t value;
    struct {
        std::uint64_t present : 1;
        std::uint64_t rw : 1;
        std::uint64_t us : 1;
        std::uint64_t pwt : 1;
        std::uint64_t pcd : 1;
        std::uint64_t accessed : 1;
        std::uint64_t dirty : 1;
        std::uint64_t pat : 1;
        std::uint64_t global : 1;
        std::uint64_t ignored_0 : 2;
        std::uint64_t restart : 1;
        std::uint64_t pfn : 40;
        std::uint64_t ignored_1 : 7;
        std::uint64_t mpk : 4;
        std::uint64_t nx : 1;
    };
};

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

uint64_t getEmptyIndex(uint64_t tableVA);

inline void invlpg(void* addr) {
    asm volatile("invlpg (%0)" :: "r"(addr) : "memory");
}

template<typename T>
T read(uint64_t PA){//only 4kb mapping
	auto cr3 = __readcr3();
	auto pml4 = cr3 >> 12 << 12;

	PHYSICAL_ADDRESS pml4PA{};
	pml4PA.QuadPart = static_cast<LONGLONG>(pml4);

  auto self_ref = reinterpret_cast<uint64_t>(MmGetVirtualForPhysical(pml4PA));
  if (!self_ref) return static_cast<T>(0);

  uint64_t custom_mapping = self_ref;

	auto p1Index = (custom_mapping >> 12) & 0x1FF;
	auto offset = (custom_mapping) & 0xFFF;
  
	p1Index = getEmptyIndex(self_ref);
  offset = PA & 0xFFF;

	custom_mapping = (custom_mapping & ~(0x1FF << 12)) | ((p1Index & 0x1FF) << 12);
	custom_mapping = (custom_mapping & ~(0xFFF)) | ((offset & 0xFFF));
  
	auto& custom_mapping_entry = reinterpret_cast<PTE*>(self_ref)[p1Index];

  custom_mapping_entry.pfn = PA >> 12;
  custom_mapping_entry.rw = true;
  custom_mapping_entry.present = true;

  invlpg(reinterpret_cast<void*>(custom_mapping));

  T result = *reinterpret_cast<T*>(custom_mapping);

  custom_mapping_entry.pfn = 0;
  custom_mapping_entry.rw = false;
  custom_mapping_entry.present = false;

  return result;
}
