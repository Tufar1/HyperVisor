#include "vmcb.h"
#include "wdm.h"

void DriverUnload(DRIVER_OBJECT* driver_object) {
	UNREFERENCED_PARAMETER(driver_object);
}

bool SetupVirtualization(void* ctx){
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
		return false;
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
		
	auto HostState = MmAllocateContiguousMemory(0x1000, MaxAddress);
	
	if(!HostState)
		return false;

	RtlZeroMemory(HostState, 0x1000);
	
	PHYSICAL_ADDRESS HostStatePA = MmGetPhysicalAddress(HostState);

	VMCB = reinterpret_cast<Vmcb*>(MmAllocateContiguousMemory(0x1000, MaxAddress));
	
	if(!VMCB)
		return false;
	
	RtlZeroMemory(VMCB, 0x1000);
	PHYSICAL_ADDRESS VMCBPA = MmGetPhysicalAddress(VMCB);

	//write to HSAVE msr
	WriteMSR(0xC0010117, HostStatePA.QuadPart);
	
	svm_vmsave(reinterpret_cast<void*>(HostStatePA.QuadPart));
	svm_vmsave(reinterpret_cast<void*>(VMCBPA.QuadPart));

	SetupVMCB(VMCBPA);

	return true;	
}

NTSTATUS DriverEntry(DRIVER_OBJECT* driver_object, UNICODE_STRING*) {
	DbgPrintEx(0, 0, "Hello from driver!\n");
	driver_object->DriverUnload = DriverUnload;

	//HANDLE OutHandle;
	//PsCreateSystemThread(&OutHandle, GENERIC_ALL, nullptr, nullptr, nullptr, SetupVirtualization, nullptr);
	//ZwClose(OutHandle);
	
	SetupVirtualization(nullptr);

	return STATUS_SUCCESS;
}
