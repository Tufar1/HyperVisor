#include "vmcb.h"
#include "ntddk.h"
#include "wdm.h"
#include <cstdint>

void DumpVMCB(void* vmcb, size_t from, size_t to) {
	auto* base = reinterpret_cast<uint8_t*>(vmcb);
	for (size_t off = from; off < to; off += 8) {
  	uint64_t v = *reinterpret_cast<uint64_t*>(base + off);
    if (v) {
    	DbgPrintEx(0, 0, "VMCB[0x%03llX] = 0x%016llX\n", (uint64_t)off, v);
    }
  }
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

extern "C" int HandleExit(GuestContext* ctx) {
	auto ExitCode = VMCB->control.exitCode;
	auto Info1 = VMCB->control.exitInfo1;
	auto Info2 = VMCB->control.exitInfo2;
  DbgPrintEx(0, 0, "ExitCode: 0x%llX, info1: 0x%llX, info2: 0x%llX\n", ExitCode, Info1, Info2);
	
	if(ExitCode == 0x72){
		DbgPrintEx(0,0,"CPUID intercepted\n");
	
		DbgPrintEx(0,0,"From Guest: [rax] 0x%llX [rbx] 0x%llX [rcx] 0x%llX [rdx] 0x%llX\n", VMCB->state.rax, ctx->rbx, ctx->rcx, ctx->rdx);

    int regs[4];
    __cpuidex(regs, (int)VMCB->state.rax, (int)ctx->rcx);
		
		regs[2] &= ~(1 << 2);
		
  	VMCB->state.rax = (uint32_t)regs[0];
    ctx->rbx = (uint32_t)regs[1];
    ctx->rcx = (uint32_t)regs[2];
    ctx->rdx = (uint32_t)regs[3];

		if((ctx->rcx & (1 << 2)) == 0)
			DbgPrintEx(0, 0, "(Fake) SVM Not available\n");
		
		VMCB->state.rip = VMCB->control.nRIP;
	}
	return 1;
}

bool SetupVMCB(){
	//Control Area
	
  VMCB->control.interceptMisc1 |= (1u << 18);  // CPUID intercept
  VMCB->control.interceptMisc2 |= (1u << 0);   // VMRUN intercept
	VMCB->control.guestASID = 1;

	PHYSICAL_ADDRESS maxAddr;
	maxAddr.QuadPart = MAXULONG64;

	void* msrpm = MmAllocateContiguousMemory(2 * PAGE_SIZE, maxAddr);
	void* iopm  = MmAllocateContiguousMemory(3 * PAGE_SIZE, maxAddr);

	RtlZeroMemory(msrpm, 2 * PAGE_SIZE);
	RtlZeroMemory(iopm, 3 * PAGE_SIZE);	
	
	PHYSICAL_ADDRESS msrpmPhys = MmGetPhysicalAddress(msrpm);
	PHYSICAL_ADDRESS iopmPhys  = MmGetPhysicalAddress(iopm);
	
  VMCB->control.msrpmBasePA = MmGetPhysicalAddress(msrpm).QuadPart;
  VMCB->control.iopmBasePA  = MmGetPhysicalAddress(iopm).QuadPart;

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

	void* guestCode = MmAllocateContiguousMemory(PAGE_SIZE, maxAddr);
	uint8_t* code = (uint8_t*)guestCode;
	int i = 0;

	// MOV EAX, 0x80000001
	code[i++] = 0xB8;
	code[i++] = 0x01;
	code[i++] = 0x00;
	code[i++] = 0x00;
	code[i++] = 0x80;

	// CPUID
	code[i++] = 0x0F;
	code[i++] = 0xA2;
	
	// CPUID
	code[i++] = 0x0F;
	code[i++] = 0xA2;
	
	// JMP $
	code[i++] = 0xEB;
	code[i++] = 0xFE;

	uint64_t rflags = __readeflags();

	rflags &= ~(1ULL << 9);	
	VMCB->state.rflags = rflags;

	VMCB->state.gdtr.limit = gdtr.limit;
  VMCB->state.gdtr.base  = gdtr.base;
  
	VMCB->state.idtr.limit = idtr.limit;
  VMCB->state.idtr.base  = idtr.base;
	
  VMCB->state.cs.selector = csSeg.selector;
  VMCB->state.cs.attrib   = csSeg.attrib;
  VMCB->state.cs.limit    = csSeg.limit;
  VMCB->state.cs.base     = csSeg.base;

  VMCB->state.ss.selector = ssSeg.selector;
  VMCB->state.ss.attrib   = ssSeg.attrib;
  VMCB->state.ss.limit    = ssSeg.limit;
  VMCB->state.ss.base     = ssSeg.base;

  VMCB->state.cr0 = cr0;
  VMCB->state.cr3 = cr3;
  VMCB->state.cr4 = cr4;
  VMCB->state.dr6 = dr6;
  VMCB->state.dr7 = dr7;

	VMCB->state.rip = (uint64_t)guestCode;
	VMCB->state.rsp = rsp;

	VMCB->state.efer = EFER;

	VMCB->state.cpl = 0;

	DumpVMCB(VMCB, 0x000, 0x400);   // Control Area
	DumpVMCB(VMCB, 0x400, 0x700);   // State Save Area	

	int regs[4];
	__cpuid(regs, 0x80000001);

  DbgPrintEx(0, 0, "CPUID eax=0x%X ebx=0x%X ecx=0x%X edx=0x%X\n",regs[0], regs[1], regs[2], regs[3]);	

  return true;
}
