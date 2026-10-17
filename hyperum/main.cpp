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
        // přesuň current thread na core i
        DWORD_PTR mask = 1ULL << i;
        SetThreadAffinityMask(GetCurrentThread(), mask);
        
        // dej OS chvilku aby thread opravdu přesunul
        Sleep(1);
        
        vmmcall(0,0);
    }
    
    SetThreadAffinityMask(GetCurrentThread(), (1ULL << coreCount) - 1);


	auto sharedAddress = VirtualAlloc(nullptr, 0x1000, MEM_COMMIT | MEM_RESERVE,PAGE_READWRITE);
	printf("HV response: 0x%llX\n", sharedAddress);
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
	printf("HV response: %s\n", (char*)shared->buffer);
	printf("HV response: 0x%llX\n", *(uint64_t*)shared->buffer);
	
	printf("result: %i\n", result);

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
	printf("HV response: %s\n", (char*)shared->buffer);
	printf("HV response: 0x%llX\n", *(uint64_t*)shared->buffer);
	
	printf("result: %i\n", result);
	
	system("pause");
  return 0;
}
