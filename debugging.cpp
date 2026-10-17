#include "debugging.h"
#include "ntddk.h"
#include "vmcb.h"
#include <cstdint>
#include <ntdef.h>

bool SetupBreakpoint(uint64_t virtualAddress){
	
	return true;
}

uint64_t GetTargetPAFromVA(uint64_t virtualAddress) {
	auto pml4 = targetCR3 >> 12 << 12;
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

uint64_t getEmptyIndex(uint64_t tableVA){
  for (uint64_t i = 0; i < 512; i++) {
    if(reinterpret_cast<PTE*>(tableVA)[i].present == 0){
      return i;
    }
  }
  return 0;
}

void readBytes(uint64_t PA,uint32_t bytesToRead,uint8_t* outBuffer){//only 4kb mapping
	auto cr3 = __readcr3();
	auto pml4 = cr3 >> 12 << 12;

	PHYSICAL_ADDRESS pml4PA{};
	pml4PA.QuadPart = static_cast<LONGLONG>(pml4);

  auto self_ref = reinterpret_cast<uint64_t>(MmGetVirtualForPhysical(pml4PA));
  if (!self_ref) return;

  uint64_t custom_mapping = self_ref;

	auto p1Index = (custom_mapping >> 12) & 0x1FF;
	auto offset = (custom_mapping) & 0xFFF;
  
	p1Index = getEmptyIndex(self_ref);
  
	auto& custom_mapping_entry = reinterpret_cast<PTE*>(self_ref)[p1Index];

	size_t bytesRead = 0;

	while(bytesRead < bytesToRead){
  	auto currentPA = PA + bytesRead;
		offset = currentPA & 0xFFF;
		
		custom_mapping = (custom_mapping & ~(0x1FF << 12)) | ((p1Index & 0x1FF) << 12);
		custom_mapping = (custom_mapping & ~(0xFFF)) | ((offset & 0xFFF));
  
		custom_mapping_entry.pfn = currentPA >> 12;
  	custom_mapping_entry.rw = true;
  	custom_mapping_entry.present = true;
		
		uint32_t bytesThisPage = 0x1000 - (currentPA & 0xFFF);

		if(bytesThisPage > bytesToRead - bytesRead)
			bytesThisPage = bytesToRead - bytesRead;
  	
		invlpg(reinterpret_cast<void*>(custom_mapping));

		for(uint32_t i = 0; i < bytesThisPage; i += 8){
			*reinterpret_cast<uint64_t*>(outBuffer + bytesRead + i) = *reinterpret_cast<uint64_t*>(custom_mapping + i);
		}

  	custom_mapping_entry.pfn = 0;
  	custom_mapping_entry.rw = false;
  	custom_mapping_entry.present = false;

  	invlpg(reinterpret_cast<void*>(custom_mapping));
		
		bytesRead += 8;
	}
	return;
}

uint64_t GetTargetModuleBase() {
    if (!targetEProcess) {
        return 0;
    }

    auto peb = *reinterpret_cast<uint64_t*>(targetEProcess + EProcess::PEB);
    if (!peb) {
        return 0;
    }

    auto pebPA = GetTargetPAFromVA(peb);
    if (!pebPA) {
        return 0;
    }

    auto ldr = read<uint64_t>(pebPA + Peb::Ldr);
    if (!ldr) {
        return 0;
    }

    auto listHead = ldr + ldrData::InLoadOrderModuleList;
    auto listHeadPA = GetTargetPAFromVA(listHead);
    if (!listHeadPA) {
        return 0;
    }

    auto current = read<uint64_t>(listHeadPA);
    if (!current) {
        return 0;
    }

    auto targetName = reinterpret_cast<const wchar_t*>(SharedCommunication->buffer);

    uint16_t targetLen = 0;
    while (targetName[targetLen] != L'\0') targetLen++;

    uint32_t iteration = 0;
    while (current != listHead) {
        auto currentPA = GetTargetPAFromVA(current);
        if (!currentPA) {
            break;
        }

        auto moduleBase = read<uint64_t>(currentPA + ldrDataTableEntry::DllBase);
        auto nameLen    = read<uint16_t>(currentPA + ldrDataTableEntry::BaseDllName);
        auto nameBuf    = read<uint64_t>(currentPA + ldrDataTableEntry::BaseDllName + 0x8);

        if (nameLen > 0 && nameBuf) {
            char ascii[64]{};
            uint16_t printLen = (nameLen / 2 < 63) ? nameLen / 2 : 63;
            auto nameBufPA = GetTargetPAFromVA(nameBuf);
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

uint64_t GetCR3FromName(char* name){
  uint64_t entry = reinterpret_cast<uint64_t>(PsInitialSystemProcess) + 0x1D8;
  for (uint64_t currentEntry = *reinterpret_cast<uint64_t*>(entry); currentEntry != entry;) {
  	uint64_t eprocess = currentEntry - 0x1D8;
    char* processName = reinterpret_cast<char*>(eprocess + 0x338);
    if (_stricmp(name, processName) == 0) {
			targetEProcess = eprocess;
    	return *reinterpret_cast<uint64_t*>(eprocess + 0x28);
    }
  	currentEntry = *reinterpret_cast<uint64_t*>(currentEntry);
  }
  return 0;
}
