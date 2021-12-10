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

Block *FirstFit(size_t size) {
    auto block = heapstart;

    while (block != nullptr) {
        if (!block->used && block->size >= size) return block;
        block = block->next;
    }
    return nullptr;
}

bool canSplit(Block *block, size_t size) {
    int remaining = block->size - sizeof(Block);
    if (remaining < sizeof(Block) - sizeof(std::declval<Block>().data)) return 0;
    return 1;
}

Block *split(Block *block, size_t size) {
    if (!canSplit(Block, size)) return nullptr;
    block->size = size;
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
    if (search != nullptr) return search->data;

    auto block = HeapRequest(real_size);
    block->size = real_size;
    block->used = true;

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
    blk->used = false;
}

void Initialization() {
    ResetHeap();
}

int main() {
    Initialization();
    return 0;
}
