#pragma once
#include <ntddk.h>
#include <cstdint>
#include <intrin.h>

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
    VmcbSegment ldtr;                  // 0x470
    VmcbSegment idtr;                  // 0x480
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

struct alignas(0x1000) GuestState {
	union {
  uint8_t HostStack[0x2000];

  struct {
  	uint8_t StackContents[0x1B00];

    CONTEXT oldContext;  // 0x1B00
    PHYSICAL_ADDRESS GuestVmcbPA; // 0x1FD0
    uint64_t HostVmcbPA;  // 0x1FD8

    Vmcb* Vmcb;           // 0x1FE0
    bool virtualised;     // 0x1FE8
	};
  };
};

#pragma pack(push, 1)
struct SharedComms{
	uint64_t code;
	uint64_t address;
	uint64_t dataLength;
	uint8_t buffer[0xFE8];
};
#pragma pack(pop)

union PML4E {
    std::uint64_t value;
    struct {
        std::uint64_t present : 1;
        std::uint64_t rw : 1;
        std::uint64_t us : 1;
        std::uint64_t pwt : 1;
        std::uint64_t pcd : 1;
        std::uint64_t accessed : 1;
        std::uint64_t ignored_0 : 1;
        std::uint64_t reserved_0 : 1;
        std::uint64_t ignored_1 : 3;
        std::uint64_t restart : 1;
        std::uint64_t pfn : 40;
        std::uint64_t ignored_2 : 11;
        std::uint64_t nx : 1;
    };
};

union PDPTE {
    std::uint64_t value;
    struct {
        std::uint64_t present : 1;
        std::uint64_t rw : 1;
        std::uint64_t us : 1;
        std::uint64_t pwt : 1;
        std::uint64_t pcd : 1;
        std::uint64_t accessed : 1;
        std::uint64_t ignored_0 : 1;
        std::uint64_t page_size : 1;
        std::uint64_t ignored_1 : 3;
        std::uint64_t restart : 1;
        std::uint64_t pfn : 40;
        std::uint64_t ignored_2 : 11;
        std::uint64_t nx : 1;
    };
};

union PDPTE_1GB {
    std::uint64_t value;
    struct {
        std::uint64_t present : 1;
        std::uint64_t rw : 1;
        std::uint64_t us : 1;
        std::uint64_t pwt : 1;
        std::uint64_t pcd : 1;
        std::uint64_t accessed : 1;
        std::uint64_t dirty : 1;
        std::uint64_t page_size : 1;
        std::uint64_t global : 1;
        std::uint64_t ignored_0 : 2;
        std::uint64_t restart : 1;
        std::uint64_t pat : 1;
        std::uint64_t reserved_0 : 17;
        std::uint64_t pfn : 22;
        std::uint64_t ignored_1 : 7;
        std::uint64_t mpk : 4;
        std::uint64_t nx : 1;
  };
};

union PDE {
    std::uint64_t value;
    struct {
        std::uint64_t present : 1;
        std::uint64_t rw : 1;
        std::uint64_t us : 1;
        std::uint64_t pwt : 1;
        std::uint64_t pcd : 1;
        std::uint64_t accessed : 1;
        std::uint64_t ignored_0 : 1;
        std::uint64_t page_size : 1;
        std::uint64_t ignored_1 : 3;
        std::uint64_t restart : 1;
        std::uint64_t pfn : 40;
        std::uint64_t ignored_2 : 11;
        std::uint64_t nx : 1;
    };
};

union PDE_2MB {
    std::uint64_t value;
    struct {
        std::uint64_t present : 1;
        std::uint64_t rw : 1;
        std::uint64_t us : 1;
        std::uint64_t pwt : 1;
        std::uint64_t pcd : 1;
        std::uint64_t accessed : 1;
        std::uint64_t dirty : 1;
        std::uint64_t page_size : 1;
        std::uint64_t global : 1;
        std::uint64_t ignored_0 : 2;
        std::uint64_t restart : 1;
        std::uint64_t pat : 1;
        std::uint64_t reserved_0 : 8;
        std::uint64_t pfn : 31;
        std::uint64_t ignored_1 : 7;
        std::uint64_t mpk : 4;
        std::uint64_t nx : 1;
    };
};

union PTE {
    std::uint64_t value;
    struct {
        std::uint64_t present : 1;
        std::uint64_t rw : 1;
        std::uint64_t us : 1;
        std::uint64_t pwt : 1;
        std::uint64_t pcd : 1;
        std::uint64_t accessed : 1;
        std::uint64_t dirty : 1;
        std::uint64_t pat : 1;
        std::uint64_t global : 1;
        std::uint64_t ignored_0 : 2;
        std::uint64_t restart : 1;
        std::uint64_t pfn : 40;
        std::uint64_t ignored_1 : 7;
        std::uint64_t mpk : 4;
        std::uint64_t nx : 1;
    };
};



static_assert(sizeof(SharedComms) == 0x1000);

static_assert(sizeof(CONTEXT) == 0x4D0);
static_assert(offsetof(GuestState, GuestVmcbPA) == 0x1FD0);
static_assert(offsetof(GuestState, Vmcb) == 0x1FE0);
static_assert(offsetof(GuestState, oldContext) == 0x1B00);
