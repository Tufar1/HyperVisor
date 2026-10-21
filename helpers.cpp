#include "helpers.h"
#include <cstdint>

uint64_t GetTargetPAFromVA(uint64_t virtualAddress, uint64_t CR3) {
	auto pml4 = CR3 >> 12 << 12;
	auto offset = virtualAddress & 0xFFF;

	auto p4Index = (virtualAddress >> 39) & 0x1FF;
	auto p3Index = (virtualAddress >> 30) & 0x1FF;
	auto p2Index = (virtualAddress >> 21) & 0x1FF;
	auto p1Index = (virtualAddress >> 12) & 0x1FF;

	auto p4e = read<PML4E>((pml4) + p4Index * 8);
  if(!p4e.present)
    return 0;

  auto p3e = read<PML4E>((p4e.pfn << 12) + p3Index*8);
	auto* p3e_ptr = &p3e;
  if(!p3e.present)
    return 0;

  if(reinterpret_cast<PDPTE_1GB*>(p3e_ptr)->page_size)
    return (p3e.pfn << 30) + offset;

  auto p2e = read<PML4E>((p3e.pfn << 12) + p2Index*8);
	auto* p2e_ptr = &p2e;
  if(!p2e.present)
    return 0;

  if(reinterpret_cast<PDE_2MB*>(p2e_ptr)->page_size)
    return (p2e.pfn << 21) + offset;

  auto pe = read<PTE>((p2e.pfn << 12) + p1Index*8);
  if(!pe.present)
    return 0;

  return (pe.pfn << 12) + offset;
}

uint64_t GetCR3FromName(char* name, uint64_t& EProcess){
  uint64_t entry = reinterpret_cast<uint64_t>(PsInitialSystemProcess) + 0x1D8;
  for (uint64_t currentEntry = *reinterpret_cast<uint64_t*>(entry); currentEntry != entry;) {
  	uint64_t eprocess = currentEntry - 0x1D8;
    char* processName = reinterpret_cast<char*>(eprocess + 0x338);
    if (_stricmp(name, processName) == 0) {
			EProcess = eprocess;
    	return *reinterpret_cast<uint64_t*>(eprocess + 0x28);
    }
  	currentEntry = *reinterpret_cast<uint64_t*>(currentEntry);
  }
  return 0;
}

uint64_t GetEmptyIndex(uint64_t tableVA){
  for (uint64_t i = 0; i < 512; i++) {
    if(reinterpret_cast<PTE*>(tableVA)[i].present == 0){
      return i;
    }
  }
  return 0;
}

uint64_t GetTargetModule(uint64_t EProcess, const wchar_t* targetName) {
    if (!EProcess) {
        return 0;
    }

		auto cr3 = *reinterpret_cast<uint64_t*>(EProcess + EProcess::CR3);
    
		auto peb = *reinterpret_cast<uint64_t*>(EProcess + EProcess::PEB);
    if (!peb) {
        return 0;
    }

    auto pebPA = GetTargetPAFromVA(peb, cr3);
    if (!pebPA) {
        return 0;
    }

    auto ldr = read<uint64_t>(pebPA + Peb::Ldr);
    if (!ldr) {
        return 0;
    }

    auto listHead = ldr + ldrData::InLoadOrderModuleList;
    auto listHeadPA = GetTargetPAFromVA(listHead, cr3);
    if (!listHeadPA) {
        return 0;
    }

    auto current = read<uint64_t>(listHeadPA);
    if (!current) {
        return 0;
    }

    uint16_t targetLen = 0;
    while (targetName[targetLen] != L'\0') targetLen++;

    uint32_t iteration = 0;
    while (current != listHead) {
        auto currentPA = GetTargetPAFromVA(current, cr3);
        if (!currentPA) {
            break;
        }

        auto moduleBase = read<uint64_t>(currentPA + ldrDataTableEntry::DllBase);
        auto nameLen    = read<uint16_t>(currentPA + ldrDataTableEntry::BaseDllName);
        auto nameBuf    = read<uint64_t>(currentPA + ldrDataTableEntry::BaseDllName + 0x8);

        if (nameLen > 0 && nameBuf) {
            char ascii[64]{};
            uint16_t printLen = (nameLen / 2 < 63) ? nameLen / 2 : 63;
            auto nameBufPA = GetTargetPAFromVA(nameBuf, cr3);
            if (nameBufPA) {
                for (uint16_t i = 0; i < printLen; i++) {
                    wchar_t c = read<wchar_t>(nameBufPA + i * 2);
                    ascii[i] = (char)(c & 0xFF);
                }
                ascii[printLen] = '\0';
            }

            if ((nameLen / 2) == targetLen && nameBufPA) {
                bool match = true;
                for (uint16_t i = 0; i < targetLen; i++) {
                    wchar_t a = read<wchar_t>(nameBufPA + i * 2);
                    wchar_t b = targetName[i];
                    if (a >= L'A' && a <= L'Z') a += 32;
                    if (b >= L'A' && b <= L'Z') b += 32;
                    if (a != b) { match = false; break; }
                }

                if (match) {
                    return moduleBase;
                }
            }
        }

        current = read<uint64_t>(currentPA);
        iteration++;

        if (iteration > 512) {
            break;
        }
    }
    return 0;
}

VmcbSegment GetSegmentFromGDT(uint16_t selector) {
  VmcbSegment seg{};
  seg.selector = selector;

  if ((selector & ~0x7) == 0) {
    return seg;
  }

  GDTR gdtr;
  ReadGDT(&gdtr);

  uint64_t descAddr = gdtr.base + (selector & ~0x7);
  uint64_t desc = *reinterpret_cast<uint64_t*>(descAddr);

  uint32_t limit_low   = static_cast<uint32_t>(desc & 0xFFFF);
  uint32_t base_low    = static_cast<uint32_t>((desc >> 16) & 0xFFFFFF);
  uint8_t  accessByte  = static_cast<uint8_t>((desc >> 40) & 0xFF);
  uint32_t limit_high  = static_cast<uint32_t>((desc >> 48) & 0xF);
  uint8_t  flags       = static_cast<uint8_t>((desc >> 52) & 0xF);
  uint32_t base_high   = static_cast<uint32_t>((desc >> 56) & 0xFF);

  seg.limit = limit_low | (limit_high << 16);
  seg.base  = base_low | (static_cast<uint64_t>(base_high) << 24);

  seg.attrib = static_cast<uint16_t>(accessByte) | (static_cast<uint16_t>(flags) << 8);

  return seg;
}

void DumpVMCB(void* vmcb, size_t from, size_t to) {
	auto* base = reinterpret_cast<uint8_t*>(vmcb);
	for (size_t off = from; off < to; off += 8) {
  	uint64_t v = *reinterpret_cast<uint64_t*>(base + off);
    if (v) {
    	DbgPrintEx(0, 0, "VMCB[0x%03llX] = 0x%016llX\n", (uint64_t)off, v);
    }
  }
}
