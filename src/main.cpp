#include <cstdint>
#include <cstring>
#include <sys/mman.h>
#include "pl/Gloss.h"
#include "pl/Signature.h"

static const char* GFX_GAMMA_SIGNATURE =
    "CA 92 06 F8 29 01 40 F9 C8 E2 06 F8 48 02 80 52 "
    "A8 03 16 38 28 0C 80 52 BF E3 1A 38 C9 92 02 F8 "
    "C8 12 03 78 E8 4D 82 52 01 E4 00 2F 00 10 2C 1E "
    "68 50 A7 72 02 10 2E 1E";

constexpr uint32_t MOV_W8_10    = 0x52800148;
constexpr uint32_t SCVTF_S2_W8  = 0x1E220102;
constexpr uint32_t FMOV_S2_1_0  = 0x1E2E1002;

static bool PatchMemory(void* addr, const void* data, size_t size) {
    uintptr_t page_start = (uintptr_t)addr & ~(4095UL);
    size_t page_size = ((uintptr_t)addr + size - page_start + 4095) & ~(4095UL);

    if (mprotect((void*)page_start, page_size, PROT_READ | PROT_WRITE | PROT_EXEC) != 0)
        return false;

    memcpy(addr, data, size);
    __builtin___clear_cache((char*)addr, (char*)addr + size);
    mprotect((void*)page_start, page_size, PROT_READ | PROT_EXEC);

    return true;
}

static bool PatchGfxGamma() {
    uintptr_t addr = pl::signature::pl_resolve_signature(GFX_GAMMA_SIGNATURE, "libminecraftpe.so");
    if (addr == 0)
        return false;

    uint8_t* base = (uint8_t*)addr;
    uint32_t* fmov_addr = (uint32_t*)(base + 52);

    if (*fmov_addr != FMOV_S2_1_0)
        return false;

    uint32_t* movk_addr = (uint32_t*)(base + 48);

    if (!PatchMemory(movk_addr, &MOV_W8_10, sizeof(MOV_W8_10)))
        return false;

    if (!PatchMemory(fmov_addr, &SCVTF_S2_W8, sizeof(SCVTF_S2_W8)))
        return false;

    return true;
}

__attribute__((constructor))
void BetterBrightness_Init() {
    GlossInit(true);
    PatchGfxGamma();
}
