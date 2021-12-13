#include <cstdio>
#include <utility>
#include <heapapi.h>
#include <memoryapi.h>
#include <assert.h>

using namespace std;

int msk = 0xFFFFFF8;

struct Block {
    size_t header;
    struct Block *next;
    intptr_t *data;
} *heapstart = nullptr;

typedef Block* (*SearchModes) (size_t size);

static void *arena = nullptr;
static void *brk = nullptr;
static size_t arenaSize = 4194304;

auto top = heapstart;
auto searchstart = heapstart;

void *custom_sbrk(size_t increment) {
    if (arena == nullptr) {
        arena = VirtualAlloc(nullptr, arenaSize,MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        brk = arena;
        return brk;
    }
    if (increment == 0) return brk;
    if ((char*)brk + increment >= (char*)arena + arenaSize) return (void *)-1;
    brk = (char*)brk + increment;
    return brk;
}

size_t GetSize(Block *block) {
    return block->header & ~1;
}

bool GetUsed(Block *block) {
    return block->header & 1;
}

void SetUsed(Block *block, bool used) {
    if (used) block->header |= 1;
    else block->header &= ~1;
}

void SetSize(Block *block, size_t size) {
    bool tmp2 = GetUsed(block);
    block->header = size;
    SetUsed(block, tmp2);
}

size_t allign(size_t n) {
    return (n + sizeof(intptr_t) - 1) & msk;
}

size_t total_alloc_size(size_t n) {
    return n + sizeof(struct Block) - sizeof(intptr_t);
}

Block *HeapRequest(size_t size) {
    auto block = (Block *)custom_sbrk(0);
    if (custom_sbrk(total_alloc_size(size)) == (void *)-1) return nullptr;
    return block;
}

Block *getHeader(intptr_t *data) {
    return (Block *)(data + (sizeof(intptr_t) - sizeof(Block))/8);
}

bool canSplit(Block *block, size_t size) {
    size_t remaining = GetSize(block) - size;
    if (remaining < sizeof(Block)) return false;
    return true;
}

void split(Block *block, size_t size) {
    if (!canSplit(block, size)) return ;
    size_t remaining = GetSize(block) - size;
    SetSize(block, size);
    auto sp = (Block *) ((char *)block + sizeof(intptr_t) + size);
    sp->header = remaining - sizeof(struct Block) + sizeof(intptr_t);
    SetUsed(sp, false);
    sp->next = block->next;
    block->next = sp;
}

bool canCombine(Block *block) {
    return block->next && !GetUsed(block->next);
}

void Combine(Block *block) {
    if (!canCombine(block)) {
        return;
    }
    SetSize(block, GetSize(block) + GetSize(block->next) + sizeof(struct Block) - sizeof(intptr_t));
    block->next = block->next->next;
}

Block *FirstFit(size_t size) {
    auto block = heapstart;

    while (block != nullptr) {
        if (!GetUsed(block) && GetSize(block) >= size) {
            return block;
        }
        block = block->next;
    }
    return nullptr;
}

Block *NextFit(size_t size) {
    auto block = searchstart;
    auto stop = searchstart;
    while (block != nullptr) {
        if (!GetUsed(block) && GetSize(block) >= size) {
            searchstart = block;
            return block;
        }
        block = block->next;
    }
    block = heapstart;
    while (block != stop) {
        if (!GetUsed(block) && GetSize(block) >= size) {
            searchstart = block;
            return block;
        }
        block = block->next;
    }
    return nullptr;
}

Block *BestFit(size_t size) {
    auto block = heapstart;
    Block* ales = nullptr;
    size_t mini = 1e9;
    while (block != nullptr) {
        if (!GetUsed(block) && GetSize(block) >= size) {
            if (GetSize(block) - size < mini) {
                mini = GetSize(block) - size;
                ales = block;
            }
        }
        block = block->next;
    }
    return ales;
}

SearchModes SearchFunctions[] = {
        FirstFit,
        NextFit,
        BestFit
};

intptr_t *m_alloc(size_t n) {
    size_t real_size = allign(n);
    auto search = SearchFunctions[2](real_size);
    if (search != nullptr) {
        split(search, real_size);
        SetUsed(search, true);
        return (intptr_t *)&search->data;
    }

    auto block = HeapRequest(real_size);
    block->header = real_size;
    SetUsed(block, true);
    block->next = nullptr;

    if (heapstart == nullptr) {
        heapstart = block;
        searchstart = heapstart;
    }

    if (top != nullptr) {
        top->next = block;
    }

    top = block;
    return (intptr_t *)&block->data;
}

void free(intptr_t *data) {
    auto blk = getHeader(data);
    Combine(blk);
    SetUsed(blk, false);
}

int main() {
    heapstart = nullptr;
    top = nullptr;
    searchstart = nullptr;
    return 0;
}