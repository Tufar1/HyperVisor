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
    uint32_t interceptCR;              // 0x000
    uint32_t interceptDR;              // 0x004
    uint32_t interceptExceptions;      // 0x008
    uint32_t interceptMisc1;           // 0x00C
    uint32_t interceptMisc2;           // 0x010
    uint8_t  reserved2[0x2C];          // 0x014 - 0x03F
    uint64_t iopmBasePA;               // 0x040
    uint64_t msrpmBasePA;              // 0x048
    uint8_t  reserved3[0x08];          // 0x050
    uint32_t guestASID;                // 0x058
    uint32_t tlbControl;               // 0x05C
    uint8_t  reserved4[0x10];          // 0x060 - 0x06F
    uint64_t exitCode;                 // 0x070
    uint64_t exitInfo1;                // 0x078
    uint64_t exitInfo2;                // 0x080
    uint8_t  reserved5[0x8];           // 0x088 - 0x08F
    uint64_t npEnable;                 // 0x090 
    uint8_t  reserved6[0x18];          // 0x098 - 0x0AF
    uint64_t nCr3;                     // 0x0B0
    uint8_t  reserved7[0x10];          // 0x0B8 - 0x0C7
    uint64_t nRIP;                     // 0x0C8
    uint8_t  reserved8[0x330];         // 0x0D0 - 0x3FF
};
static_assert(sizeof(VmcbControlArea) == 0x400, "Control Area size mismatch");

struct VmcbStateArea {
    VmcbSegment es;                    // 0x000
    VmcbSegment cs;                    // 0x010
    VmcbSegment ss;                    // 0x020
    VmcbSegment ds;                    // 0x030
    VmcbSegment fs;                    // 0x040
    VmcbSegment gs;                    // 0x050
    VmcbSegment gdtr;                  // 0x060
    VmcbSegment ldtr;                  // 0x070
    VmcbSegment idtr;                  // 0x080
    VmcbSegment tr;                    // 0x090
    uint8_t     reserved1[0x2B];       // 0x0A0 - 0x0CA
    uint8_t     cpl;                   // 0x0CB
    uint32_t    reserved2;             // 0x0CC
    uint64_t    efer;                  // 0x0D0
    uint8_t     reserved3[0x70];       // 0x0D8 - 0x147
    uint64_t    cr4;                   // 0x148
    uint64_t    cr3;                   // 0x150
    uint64_t    cr0;                   // 0x158
    uint64_t    dr7;                   // 0x160
    uint64_t    dr6;                   // 0x168
    uint64_t    rflags;                // 0x170
    uint64_t    rip;                   // 0x178
    uint8_t     reserved4[0x58];       // 0x180 - 0x1D7
    uint64_t    rsp;                   // 0x1D8
    uint8_t     reserved5[0x18];       // 0x1E0 - 0x1F7
    uint64_t    rax;                   // 0x1F8
    uint8_t     reserved6[0x68];       // 0x200 - 0x267
    uint64_t    gPAT;                  // 0x268
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
		uint64_t pendingPA; 	// 0x1FF0
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

struct NptBreakpoint {
  uint64_t guestPA;
	uint64_t guestVA;
  bool     active;
};

struct BreakpointHit{
	bool newHit;
  ULONG64 Rax;
  ULONG64 Rcx;
  ULONG64 Rdx;
  ULONG64 Rbx;
  ULONG64 Rsp;
  ULONG64 Rbp;
  ULONG64 Rsi;
  ULONG64 Rdi;
  ULONG64 R8;
  ULONG64 R9;
  ULONG64 R10;
  ULONG64 R11;
  ULONG64 R12;
  ULONG64 R13;
  ULONG64 R14;
  ULONG64 R15;
};

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
static_assert(offsetof(GuestState, pendingPA) == 0x1FF0);
