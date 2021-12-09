#include <stdio.h>
#include <stdbool.h>
#include <utility>
#include <heapapi.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <libcharset.h>

using namespace std;

int msk = 0xFFFFFF8;

struct Block {
    size_t size;
    bool used;
    struct Block *next;
    intptr_t data[1];
} *heapstart = nullptr;

auto top = heapstart;

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

intptr_t *m_alloc(size_t n) {
    size_t real_size = allign(n);
    auto block = HeapRequest(real_size);
    block->size = real_size;
    block->used = 1;

    if (heapstart == nullptr) {
        heapstart = block;
    }

    if (top != nullptr) {
        top->next = block;
    }

    top = block;

    return block->data;
}

int main() {
    return 0;
}
