#include "vmcb.h"
#include "debugging.h"
#include "helpers.h"
#include "ntddk.h"
#include "structs.h"
#include "wdm.h"
#include <cstdint>

extern "C" int HandleExit(GuestContext* ctx, Vmcb* Vmcb, uint64_t HostStatePA) {
	svm_vmload(reinterpret_cast<void*>(HostStatePA));

	auto ExitCode = Vmcb->control.exitCode;
	auto Info1 = Vmcb->control.exitInfo1;
	auto Info2 = Vmcb->control.exitInfo2;
 
	DbgPrintEx(0,0,"Exitcode: %llx\n", ExitCode);

	if(ExitCode == 0x81){
		switch (Vmcb->state.rax){
			case 0x420001: {//INIT, map allocated page to our driver for simpler communication and set target cr3
				auto MapTranslated = GetTargetPAFromVA(ctx->rcx, Vmcb->state.cr3);
				if(!MapTranslated){
					Vmcb->state.rax = 5;
					break;
				}

				PHYSICAL_ADDRESS MapPA{.QuadPart = static_cast<LONGLONG>(MapTranslated)};

				auto MappedAddress = MmMapIoSpace(MapPA, 0x1000, MmNonCached);

				if(!MappedAddress){
					Vmcb->state.rax = 4;
					break;
				}

				SharedCommunication = reinterpret_cast<SharedComms*>(MappedAddress);

				auto name = reinterpret_cast<char*>(SharedCommunication->buffer);	

				targetCR3 = GetCR3FromName(name, targetEProcess);

				if(!targetCR3){
					Vmcb->state.rax = 3;
					break;
				}	
				RtlZeroMemory(SharedCommunication, 0x1000);
				*(uint64_t*)(SharedCommunication->address) = targetCR3;

				Vmcb->state.rax = 1;
				break;
			}

			case 0x420099:{
				if(!SetupBreakpoint(ctx->rcx, targetCR3)){
					Vmcb->state.rax = 0;
					break;
				}
				Vmcb->state.rax = 1;
			}
			
			case 0x420010: { //Get Base address from name
				auto moduleAddress = GetTargetModule(targetEProcess, reinterpret_cast<const wchar_t*>(SharedCommunication->buffer));

				if(!moduleAddress){
					Vmcb->state.rax = 0;
					break;
				}

				RtlZeroMemory(SharedCommunication, 0x1000);
				Vmcb->state.rax = 1;
				*(uint64_t*)(SharedCommunication->address) = moduleAddress;
				break;
			}
			default: {
				break;
				//trigger ud
			}
		}
	}

	if(Vmcb->control.nRIP)
		Vmcb->state.rip = Vmcb->control.nRIP;

	return 1;
}

uint64_t SetupNPT(){
	auto pml4eAddr = MmAllocateContiguousMemory(0x1000, MaxAddress); //Page, just one entry pointing to pdpt
	auto pdptAddr = MmAllocateContiguousMemory(0x1000, MaxAddress); //Page for all 512 pdpt entries. Each pdpt entry has 512 PDs
	auto pdBlockAddr = MmAllocateContiguousMemory(0x1000 * 512, MaxAddress); //All 512*512 PDs
	
	if(!pml4eAddr || !pdptAddr || !pdBlockAddr)
		return 0;

	RtlZeroMemory(pml4eAddr, 0x1000);
	RtlZeroMemory(pdptAddr, 0x1000);
	RtlZeroMemory(pdBlockAddr, 0x1000 * 512);
	
	auto pdBlockPA = MmGetPhysicalAddress(pdBlockAddr);
	if(!pdBlockPA.QuadPart)
		return 0;

	auto pdptPA = MmGetPhysicalAddress(pdptAddr);
	if(!pdptPA.QuadPart)
		return 0;

	auto n_cr3 = MmGetPhysicalAddress(pml4eAddr);
	if(!n_cr3.QuadPart)
		return 0;
	
	auto pml4e = reinterpret_cast<PML4E*>(pml4eAddr);
	auto pdpt = reinterpret_cast<PDPTE*>(pdptAddr);

	pml4e->pfn = pdptPA.QuadPart >> 12;  	
	pml4e->present = 1;
	pml4e->us = 1;
	pml4e->rw = 1;

	for(int i = 0; i < 512; i++){
		pdpt[i].pfn = (pdBlockPA.QuadPart + (i * 0x1000)) >> 12; //Each pfn points to 0x1000 size of PD entries. So we just jump over 512 entries for exach loop. PD entries has each PFN difference 2MB
		
		pdpt[i].present = 1;
		pdpt[i].us = 1;
		pdpt[i].rw = 1;

		auto pd = reinterpret_cast<PDE*>(reinterpret_cast<uint64_t>(pdBlockAddr) + i * 0x1000);
		
		if(!pd)
			continue;
		
		for(int j = 0; j < 512; j++){
			pd[j].pfn = ((i * 0x40000000ULL) + (j * 0x200000ULL)) >> 12; //Once per 512 entries jump by 1Gb. Because each entry has 2mb, and theres 512 of them...
		
			pd[j].present = 1;
			pd[j].us = 1;
			pd[j].rw = 1;
			pd[j].page_size = 1;
		}
	}
	return n_cr3.QuadPart;
}

bool SetupVMCB(GuestState* GS){
	//Control Area	
  
	GS->Vmcb->control.interceptMisc2 |= (1u << 0);// - vmrun
	GS->Vmcb->control.interceptMisc2 |= (1u << 1);// - vmmcall
	
	GS->Vmcb->control.guestASID = 1;

	void* msrpm = MmAllocateContiguousMemory(2 * PAGE_SIZE, MaxAddress);
	void* iopm  = MmAllocateContiguousMemory(3 * PAGE_SIZE, MaxAddress);

	RtlZeroMemory(msrpm, 2 * PAGE_SIZE);
	RtlZeroMemory(iopm, 3 * PAGE_SIZE);	
	
	PHYSICAL_ADDRESS msrpmPhys = MmGetPhysicalAddress(msrpm);
	PHYSICAL_ADDRESS iopmPhys  = MmGetPhysicalAddress(iopm);
	
  GS->Vmcb->control.msrpmBasePA = MmGetPhysicalAddress(msrpm).QuadPart;
  GS->Vmcb->control.iopmBasePA  = MmGetPhysicalAddress(iopm).QuadPart;

	//State area	

	auto cr0 = ReadCR0();
	auto cr3 = __readcr3();
	auto cr4 = ReadCR4();
	auto dr6 = ReadDR6();
	auto dr7 = ReadDR7();

	auto EFER = __readmsr(0xC0000080);

	auto cs = ReadCS();
	auto ss = ReadSS();

	VmcbSegment csSeg = GetSegmentFromGDT(cs);
	VmcbSegment ssSeg = GetSegmentFromGDT(ss);
	
	GDTR gdtr;
	ReadGDT(&gdtr);
	GDTR idtr;
	ReadIDT(&idtr);

	uint64_t rflags = __readeflags();

	//rflags &= ~(1ULL << 9);	
	GS->Vmcb->state.rflags = rflags;

	GS->Vmcb->state.gdtr.limit = gdtr.limit;
  GS->Vmcb->state.gdtr.base  = gdtr.base;
  
	GS->Vmcb->state.idtr.limit = idtr.limit;
  GS->Vmcb->state.idtr.base  = idtr.base;
	
  GS->Vmcb->state.cs.selector = csSeg.selector;
  GS->Vmcb->state.cs.attrib   = csSeg.attrib;
  GS->Vmcb->state.cs.limit    = csSeg.limit;
  GS->Vmcb->state.cs.base     = csSeg.base;

  GS->Vmcb->state.ss.selector = ssSeg.selector;
  GS->Vmcb->state.ss.attrib   = ssSeg.attrib;
  GS->Vmcb->state.ss.limit    = ssSeg.limit;
  GS->Vmcb->state.ss.base     = ssSeg.base;

  GS->Vmcb->state.cr0 = cr0;
  GS->Vmcb->state.cr3 = cr3;
  GS->Vmcb->state.cr4 = cr4;
  GS->Vmcb->state.dr6 = dr6;
  GS->Vmcb->state.dr7 = dr7;

	GS->Vmcb->state.rip = GS->oldContext.Rip;
	GS->Vmcb->state.rsp = GS->oldContext.Rsp;

	GS->Vmcb->state.efer = EFER;

	GS->Vmcb->state.cpl = 0;

	//setup NPT
	
	GS->Vmcb->control.nCr3 = SetupNPT();	
	GS->Vmcb->control.npEnable |= (1u << 0);

	return true;
}
