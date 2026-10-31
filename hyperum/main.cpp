#include <windows.h>
#include <thread>
#include <cstdio>
#include <cstdint>

static inline uint64_t vmmcall(uint64_t rax_in, uint64_t rcx_in) {
    uint64_t rax_out;
    asm volatile (
        "vmmcall"
        : "=a"(rax_out)
        : "a"(rax_in), "c"(rcx_in)
        : "memory"
    );
    return rax_out;
}

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

#pragma pack(push, 1)
struct SharedComms{
	uint64_t code;
	uint64_t address;
	uint64_t dataLength;
	uint8_t buffer[0xFE8];
};
#pragma pack(pop)

static_assert(sizeof(SharedComms) == 0x1000);

int test(int a, int b){
	return a + b;
}

int main() {
  DWORD coreCount = std::thread::hardware_concurrency(); 
  for (DWORD i = 0; i < coreCount; i++) {
  	DWORD_PTR mask = 1ULL << i;

  	SetThreadAffinityMask(GetCurrentThread(), mask);
    Sleep(100);      
    vmmcall(0,0);
  }
  
	SetThreadAffinityMask(GetCurrentThread(), (1ULL << coreCount) - 1);

	auto sharedAddress = VirtualAlloc(nullptr, 0x1000, MEM_COMMIT | MEM_RESERVE,PAGE_READWRITE);
	printf("Allocated on: 0x%llX\n", sharedAddress);

	auto shared = reinterpret_cast<SharedComms*>(sharedAddress);

	char* dst = (char*)shared->buffer;
	const char* src = "sauerbraten.ex";
	int i = 0;
	while (src[i] != '\0' && i < sizeof(shared->buffer) - 1) {
    dst[i] = src[i];
    i++;
	}
	dst[i] = '\0';
	i++;	

	uint64_t result = vmmcall(0x420001, reinterpret_cast<uint64_t>(sharedAddress));

  printf("RAX po VMMCALL: 0x%llX\n", (unsigned long long)result);
	printf("HV response: 0x%llX\n", shared->address);

	Sleep(5000);

	const wchar_t* src2 = L"opengl32.dll";
	wchar_t* dst2 = (wchar_t*)shared->buffer;
	i = 0;
	while (src2[i] != L'\0' && i < (sizeof(shared->buffer) / 2) - 1) {
    dst2[i] = src2[i];
    i++;
	}
	dst2[i] = L'\0';

	result = vmmcall(0x420010, 0);

  printf("RAX po VMMCALL: 0x%llX\n", (unsigned long long)result);
	printf("HV response: 0x%llX\n", shared->address);
	
	Sleep(5000);
	
	auto openglBase = shared->address;

	shared->address = openglBase + 0x7F70; 
	result = vmmcall(0x420099, 0);

  printf("RAX po VMMCALL: 0x%llX\n", (unsigned long long)result);
	auto regs = reinterpret_cast<BreakpointHit*>(shared->buffer);
	
	while(true){
  	if(regs->newHit){
    	printf("BP hit!\n");
      printf("RAX: %llx  RCX: %llx  RDX: %llx  RBX: %llx\n", regs->Rax, regs->Rcx, regs->Rdx, regs->Rbx);
      printf("RSP: %llx  RBP: %llx  RSI: %llx  RDI: %llx\n", regs->Rsp, regs->Rbp, regs->Rsi, regs->Rdi);
      printf("R8:  %llx  R9:  %llx  R10: %llx  R11: %llx\n", regs->R8,  regs->R9,  regs->R10, regs->R11);
      printf("R12: %llx  R13: %llx  R14: %llx  R15: %llx\n", regs->R12, regs->R13, regs->R14, regs->R15);
      regs->newHit = 0;
    	Sleep(2000);
    	vmmcall(0x420090, 0);
		}
    Sleep(1);
	}

	system("pause");
  return 0;
}
