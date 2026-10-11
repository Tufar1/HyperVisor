#include <ntddk.h>
#include <intrin.h>
#include <ntstatus.h>

#include "vmcb.h"
#include "wdm.h"

extern "C" void RunLoop(uint64_t VMCBPA, uint64_t CustomRSP);
extern "C" void WriteMSR(uint32_t msr, uint64_t value);

void DriverUnload(DRIVER_OBJECT* driver_object) {
	UNREFERENCED_PARAMETER(driver_object);
}

void SetupVirtualization(void* ctx){
	bool CanVirt = false;

	auto VM_CR = __readmsr(0xC0010114);
	if(((VM_CR >> 4) & 1) == 0){
		DbgPrintEx(0, 0, "SVM Ready\n");
		CanVirt = true;
	}
	
	int regs[4];
	__cpuid(regs, 0x80000001);
	auto ecx = regs[2];
	
	if((ecx & (1 << 2)) == 0)
		DbgPrintEx(0, 0, "SVM Not available\n");

	__cpuid(regs, 0x8000000A);
	auto edx = regs[3];	

	if(!CanVirt){
		if((edx & (1 << 2)) == 0)
			DbgPrintEx(0, 0, "SVM Disabled at bios\n");
		else
			DbgPrintEx(0, 0, "SVM Disabled with key\n");
		return;
	}

	//VMRUN Instruction ----
	//Block is accessed thru Physical Address in [RAX]
	//Block should be mapped as write-back memory, 4KB aligned page
	//Block describes virtual machine for execution (my system clone?)
	//VM_HSAVE_PA MSR is pointer to where to save host state on vmrun and recover on vmexit
  
	//Enable EFER.SVME on each processor
	//enable virtualization
	//todo - each processor
	
	KAFFINITY affinity = (KAFFINITY)1;

	KeSetSystemAffinityThread(affinity);	

	auto EFER = __readmsr(0xC0000080);
	EFER = (EFER | (0x1 << 12));
	WriteMSR(0xC0000080, EFER);
	DbgPrintEx(0, 0, "Enabled SVME\n");

	//write to HSAVE msr
	PHYSICAL_ADDRESS MaxAddress;
	MaxAddress.QuadPart = MAXULONG;
		
	VMCB = reinterpret_cast<Vmcb*>(MmAllocateContiguousMemory(0x1000, MaxAddress));
	auto HostState = MmAllocateContiguousMemory(0x1000, MaxAddress);
	
	if(!VMCB || !HostState)
		return;

	RtlZeroMemory(VMCB, 0x1000);
	RtlZeroMemory(HostState, 0x1000);
	
	PHYSICAL_ADDRESS VMCBPA = MmGetPhysicalAddress(VMCB);
	PHYSICAL_ADDRESS HostStatePA = MmGetPhysicalAddress(HostState);
	
	//write to HSAVE msr
	WriteMSR(0xC0010117, HostStatePA.QuadPart);
	
	//setup custom stack
	auto CustomStack = reinterpret_cast<uint64_t>(MmAllocateContiguousMemory(0x5000, MaxAddress));	
	
	if(!CustomStack){
		return;
	}

	auto CustomRSP = reinterpret_cast<uint64_t>(CustomStack) + 0x5000;	
	
	DbgPrintEx(0, 0, "ExitCode Before: %i\n", VMCB->control.exitCode);

	SetupVMCB();

	RunLoop(VMCBPA.QuadPart, CustomRSP);
	
	KeRevertToUserAffinityThread();
	
	//Turn off svme 
	EFER = __readmsr(0xC0000080);
	EFER &= ~(1ULL << 12);
	WriteMSR(0xC0000080, EFER);

	PsTerminateSystemThread(STATUS_SUCCESS);
}

NTSTATUS DriverEntry(DRIVER_OBJECT* driver_object, UNICODE_STRING*) {
	DbgPrintEx(0, 0, "Hello from driver!\n");
	driver_object->DriverUnload = DriverUnload;

	HANDLE OutHandle;
	PsCreateSystemThread(&OutHandle, GENERIC_ALL, nullptr, nullptr, nullptr, SetupVirtualization, nullptr);
	ZwClose(OutHandle);
	
	return STATUS_SUCCESS;
}
