#pragma once
#include <cstdint>
#include <intrin.h>
#include <ntddk.h>
#include <ntdef.h>

extern "C" void WriteMSR(uint32_t msr, uint64_t value);
extern "C" void RunLoop(uint64_t VMCBPA, uint64_t CustomRSP);

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

#pragma pack(push, 1)
struct GDTR {
    uint16_t limit;
    uint64_t base;
};

struct VmcbSegment {
    uint16_t selector;
    uint16_t attrib;
    uint32_t limit;
    uint64_t base;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct GuestContext {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx;
};
#pragma pack(pop)

struct VmcbControlArea {
    uint8_t  reserved1[0xC];           // 0x00
    uint32_t interceptMisc1;           // 0x0C
    uint32_t interceptMisc2;           // 0x10
    uint8_t  reserved2[0x2C];          // 0x14 - 0x3F
    uint64_t iopmBasePA;               // 0x40
    uint64_t msrpmBasePA;              // 0x48
    uint8_t  reserved3[0x08];          // 0x50
    uint32_t guestASID;                // 0x58
    uint32_t tlbControl;
    uint8_t  reserved4[0x10];          // 0x60 - 0x77
    uint64_t exitCode;                 // 0x70
    uint64_t exitInfo1;                // 0x78
    uint64_t exitInfo2;                // 0x80
    uint8_t  reserved6[0x40];          // 0x88 - 0xC7
    uint64_t nRIP;                     // 0xC8
    uint8_t  reserved5[0x330];         // 0xD0
};
static_assert(sizeof(VmcbControlArea) == 0x400, "Control Area size mismatch");

struct VmcbStateArea {
    VmcbSegment es;                    // 0x400
    VmcbSegment cs;                    // 0x410
    VmcbSegment ss;                    // 0x420
    VmcbSegment ds;                    // 0x430
    VmcbSegment fs;                    // 0x440
    VmcbSegment gs;                    // 0x450
    VmcbSegment gdtr;                  // 0x460
    VmcbSegment idtr;                  // 0x470
    VmcbSegment ldtr;                  // 0x480
    VmcbSegment tr;                    // 0x490
    uint8_t     reserved1[0x2B];       // 0x4A0 - 0x4CA
    uint8_t     cpl;                   // 0x4CB
    uint32_t    reserved2;             // 0x4CC
    uint64_t    efer;                  // 0x4D0
    uint8_t     reserved3[0x70];       // 0x4D8 - 0x547
    uint64_t    cr4;                   // 0x548
    uint64_t    cr3;                   // 0x550
    uint64_t    cr0;                   // 0x558
    uint64_t    dr7;                   // 0x560
    uint64_t    dr6;                   // 0x568
    uint64_t    rflags;                // 0x570
    uint64_t    rip;                   // 0x578
    uint8_t     reserved4[0x58];       // 0x580 - 0x5D7
    uint64_t    rsp;                   // 0x5D8
    uint8_t     reserved5[0x18];       // 0x5E0 - 0x5F7
    uint64_t    rax;                   // 0x5F8
};

struct Vmcb {
    VmcbControlArea control;
    VmcbStateArea   state;
};

inline Vmcb* VMCB;
inline PHYSICAL_ADDRESS MaxAddress {.QuadPart = MAXLONGLONG};
bool SetupVMCB(PHYSICAL_ADDRESS VMCBPA);
