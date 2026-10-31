#pragma once

#include <ntddk.h>
#include <cstdint>
#include <intrin.h>

#include "structs.h"
#include "helpers.h"
#include "debugging.h"

extern "C" void WriteMSR(uint32_t msr, uint64_t value);
extern "C" void RunLoop(void* GuestVMCBPa);

extern "C" uint16_t ReadCS();
extern "C" uint16_t ReadSS();

extern "C" void ReadGDT(void* out);
extern "C" void ReadIDT(void* out);

extern "C" uint64_t ReadCR0();
extern "C" uint64_t ReadCR4();
extern "C" uint64_t ReadDR6();
extern "C" uint64_t ReadDR7();

extern "C" uint64_t ReadRSP();

extern "C" void svm_vmsave(void* VMCBPA);
extern "C" void svm_vmload(void* VMCBPA);

extern "C" NTSYSAPI VOID NTAPI RtlCaptureContext(_Out_ PCONTEXT ContextRecord);

bool SetupVMCB(GuestState* GS);

inline PHYSICAL_ADDRESS MaxAddress {.QuadPart = MAXLONGLONG};

inline SharedComms* SharedCommunication;
inline void* SharedStruct;

inline int blockedCore = -1;

inline uint64_t targetEProcess;
inline uint64_t targetCR3;
inline void* pdBlockAddr;
