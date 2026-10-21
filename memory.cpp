#include "memory.h"

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
  
	p1Index = GetEmptyIndex(self_ref);
  
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
