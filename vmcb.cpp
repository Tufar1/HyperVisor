#include "vmcb.h"

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
				*(uint64_t*)(SharedCommunication->buffer) = targetCR3;

				Vmcb->state.rax = 1;
				break;
			}
			
			case 0x420010: { //Get Base address from name
				auto moduleAddress = GetTargetModule(targetEProcess, reinterpret_cast<const wchar_t*>(SharedCommunication->buffer));

				RtlZeroMemory(SharedCommunication, 0x1000);
				*(uint64_t*)(SharedCommunication->buffer) = moduleAddress;
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

	auto rsp = ReadRSP();

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

	rflags &= ~(1ULL << 9);	
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

	return true;
}
