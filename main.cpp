#include <cstdio>
#include <utility>
#include <heapapi.h>
#include <iostream>
#include <assert.h>

using namespace std;

int msk = 0xFFFFFF8;

struct Block {
    size_t size;
    bool used;
    struct Block *next;
    intptr_t data[1];
} *heapstart = nullptr;

Block *segregatedList[] = {
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr
};

auto top = heapstart;
auto searchstart = heapstart;

size_t allign(size_t n) {
    return (n + sizeof(intptr_t) - 1) & msk;
}

size_t total_alloc_size(size_t n) {
    return n + sizeof(struct Block) - sizeof(declval<Block>().data);
}

Block *HeapRequest(size_t size) {
    auto block = (Block *)HeapAlloc(GetProcessHeap(), 0x00000004, total_alloc_size(size));
    return block;
}

Block *getHeader(intptr_t *data) {
    return (Block *)((char *)data + sizeof(std::declval<Block>().data) - sizeof(Block));
}

bool canSplit(Block *block, size_t size) {
    int remaining = block->size - size;
    if (remaining < sizeof(Block) - sizeof(std::declval<Block>().data)) return 0;
    return 1;
}

void split(Block *block, size_t size) {
    if (!canSplit(block, size)) return ;
    int remaining = block->size - size;
    block->size = size;
    auto sp = (Block *) ((char *)block + size);
    sp->size = remaining;
    sp->next = block->next;
    sp->used = false;
    block->next = sp;
}

bool canCombine(Block *block) {
    return block->next && !block->next->used;
}

void Combine(Block *block) {
    if (!canCombine(block)) {
        return;
    }
    block->size += block->next->size;
    block->next = block->next->next;
}

Block *FirstFit(size_t size) {
    auto block = heapstart;

    while (block != nullptr) {
        if (!block->used && block->size >= size) {
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
        if (!block->used && block->size >= size) {
            searchstart = block;
            return block;
        }
        block = block->next;
    }
    block = heapstart;
    while (block != stop) {
        if (!block->used && block->size >= size) {
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
    int mini = 1e9;
    while (block != nullptr) {
        if (!block->used && block->size >= size) {
            if (block->size - size < mini) {
                mini = block->size - size;
                ales = block;
            }
        }
        block = block->next;
    }
    return ales;
}

void ResetHeap() {
    if (heapstart == nullptr) {
        return;
    }
    HeapDestroy(GetProcessHeap());
    heapstart = nullptr;
    top = nullptr;
    searchstart = nullptr;
}

intptr_t *m_alloc(size_t n) {
    size_t real_size = allign(n);

    auto search = BestFit(real_size);
    if (search != nullptr) {
        split(search, n);
        return search->data;
    }

    auto block = HeapRequest(real_size);
    block->size = real_size;
    block->used = true;
    block->next = nullptr;

    if (heapstart == nullptr) {
        heapstart = block;
        searchstart = heapstart;
    }

    if (top != nullptr) {
        top->next = block;
    }

    top = block;
    return block->data;
}

void free(intptr_t *data) {
    auto blk = getHeader(data);
    Combine(blk);
    blk->used = false;
}

void Initialization() {
    ResetHeap();
}

int main() {
    Initialization();

//    auto da = m_alloc(64);
//    auto dab = getHeader(da);
//    free(da);
//    printf("%p\n", dab);
//    printf("%p\n", dab->next);
//    auto ok = m_alloc(8);
//    auto okb = getHeader(ok);
//    printf("%p\n", okb);
//    printf("%p\n", okb->next);
//    printf("%p\n", okb->next->next);
//    printf("%d\n", dab->size);
//    printf("%d\n", dab->next->size);
//    printf("%d\n", okb->size);
//    printf("%d\n", okb->next->size);
//
//    free(ok);
//    printf("%p\n", okb);
//    printf("%p\n", okb->next);
//    printf("%d\n", okb->size);
    return 0;
}
