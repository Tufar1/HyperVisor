#include "debugging.h"
#include "wdm.h"

bool SetNX(uint64_t guestPA, bool value) {
  uint64_t pdpti = (guestPA >> 30) & 0x1FF;
  uint64_t pdi   = (guestPA >> 21) & 0x1FF;
  uint64_t pti   = (guestPA >> 12) & 0x1FF;

  auto pd = reinterpret_cast<PDE*>(
    reinterpret_cast<uint64_t>(pdBlockAddr) + pdpti * 0x1000
  );
	
	if(!(pd[pdi].page_size)){
    auto existingPT = reinterpret_cast<PTE*>(MmGetVirtualForPhysical({.QuadPart = (LONGLONG)(pd[pdi].pfn << 12)}));
    existingPT[pti].nx = value;
    return 1;
	}

	auto pte = MmAllocateContiguousMemory(0x1000, MaxAddress);

	if(!pte)
		return 0;

	auto PTPa = MmGetPhysicalAddress(pte);

	auto base4kPFN = reinterpret_cast<PDE_2MB*>(&pd[pdi])->pfn << 9;

	auto pt = reinterpret_cast<PTE*>(pte);
	for(int i = 0; i < 512; i++){
		pt[i].present = 1;
		pt[i].rw = pd[pdi].rw;
		pt[i].us = pd[pdi].us;
		pt[i].pwt = pd[pdi].pwt;
		pt[i].pcd = pd[pdi].pcd;
		pt[i].pat = 0;
		pt[i].nx = 0;
		pt[i].accessed = pd[pdi].accessed;
		pt[i].dirty = reinterpret_cast<PDE_2MB*>(&pd[pdi])->dirty;
		pt[i].global = reinterpret_cast<PDE_2MB*>(&pd[pdi])->global;
		pt[i].pfn = base4kPFN + i;
	}
	pt[pti].nx = value;
	
	PDE newPde{};
	newPde.present = 1;
	newPde.rw = pd[pdi].rw;
	newPde.us = pd[pdi].us;
	newPde.pwt = pd[pdi].pwt;
	newPde.pcd = pd[pdi].pcd;
	newPde.nx = 0;
	newPde.pfn = PTPa.QuadPart >> 12;
	pd[pdi] = newPde;	
	
	return 1;
}


bool SetupBreakpoint(uint64_t addressToDebug, uint64_t targetCR3){
	auto addressToDebugPA = GetTargetPAFromVA(addressToDebug, targetCR3);		
	if(!addressToDebugPA)
		return 0;

	breakpoints[breakpointCount++] = {addressToDebugPA, addressToDebug, true};

	return SetNX(addressToDebugPA, 1);
}
