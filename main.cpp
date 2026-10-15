#include "vmcb.h"
#include "wdm.h"
#include <cstdint>

typedef bool (*callback)(PVOID ctx);

void DriverUnload(DRIVER_OBJECT* driver_object) {
	UNREFERENCED_PARAMETER(driver_object);
}

bool VirtualiseProcessor(void* gs){
	auto GS = reinterpret_cast<GuestState*>(gs);

	auto EFER = __readmsr(0xC0000080);
	EFER = (EFER | (0x1 << 12));
	WriteMSR(0xC0000080, EFER);

	//write to HSAVE msr
	auto HostState = MmAllocateContiguousMemory(0x1000, MaxAddress);
	
	if(!HostState)
		return false;

	RtlZeroMemory(HostState, 0x1000);
	
	PHYSICAL_ADDRESS HostStatePA = MmGetPhysicalAddress(HostState);

	GS->Vmcb = reinterpret_cast<Vmcb*>(MmAllocateContiguousMemory(0x1000, MaxAddress));
	
	if(!GS->Vmcb)
		return false;
	
	RtlZeroMemory(GS->Vmcb, 0x1000);
	GS->HostVmcbPA = HostStatePA.QuadPart;
	GS->GuestVmcbPA = MmGetPhysicalAddress(GS->Vmcb);

	//write to HSAVE msr
	WriteMSR(0xC0010117, HostStatePA.QuadPart);
	
	svm_vmsave(reinterpret_cast<void*>(HostStatePA.QuadPart));
	RtlCaptureContext(&GS->oldContext);
	
	if(GS->virtualised){
		return true;
	}

	SetupVMCB(GS);
	GS->virtualised = true;	
	
	svm_vmsave(reinterpret_cast<void*>(GS->GuestVmcbPA.QuadPart));
	
	RunLoop(&GS->GuestVmcbPA);

	//This should never be executed
	
	__debugbreak();

	return false;
}

uint16_t ExecOnEachCore(callback callback, PVOID ctx)
{
	NTSTATUS status;
  PROCESSOR_NUMBER processor_number;
  GROUP_AFFINITY affinity, previous_affinity;
  uint16_t i, processors;

  processors = KeQueryActiveProcessorCountEx(ALL_PROCESSOR_GROUPS);

  for (i = 0; i < processors; i++)
  {
  	status = KeGetProcessorNumberFromIndex(i, &processor_number);
    if (!NT_SUCCESS(status))
    	return 0;

    affinity.Group = processor_number.Group;
    affinity.Mask = 1ULL << processor_number.Number;
    affinity.Reserved[0] = affinity.Reserved[1] = affinity.Reserved[2] = 0;

    KeSetSystemGroupAffinityThread(&affinity, &previous_affinity);

  	bool success = callback(reinterpret_cast<void*>(reinterpret_cast<uint64_t>(ctx) + i*sizeof(GuestState)));

    KeRevertToUserGroupAffinityThread(&previous_affinity);

    if (!success)
    	return i;
    }
  return i;
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

	auto processors = KeQueryActiveProcessorCountEx(ALL_PROCESSOR_GROUPS);

	auto SharedStruct = ExAllocatePoolWithTag(NonPagedPool, sizeof(GuestState) * processors, 'GS');

	if(!ExecOnEachCore(VirtualiseProcessor, SharedStruct)){
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
	
	return;
}

NTSTATUS DriverEntry(DRIVER_OBJECT* driver_object, UNICODE_STRING*) {
	//driver_object->DriverUnload = DriverUnload;

	//HANDLE OutHandle;
	//PsCreateSystemThread(&OutHandle, GENERIC_ALL, nullptr, nullptr, nullptr, SetupVirtualization, nullptr);
	
	SetupVirtualization(nullptr);

	return STATUS_SUCCESS;
}
