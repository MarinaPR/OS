#pragma once
#include "PageAllocator.h"
#include "BlockAllocator.h"

class Allocator
{
public:
	Allocator(size_t pagesNum);
	~Allocator();

	void* mem_alloc(size_t size);

	void* mem_realloc(void* addr, size_t size);

	void mem_free(void* addr);

	void mem_dump() const;

protected:
	static size_t alignSize(const size_t size);



	PageAllocator* pageAllocator;
	std::list<BlockAllocator*>** notFullBlockAllocatorsLists;
	std::list<BlockAllocator*> allBlockAllocators;
	size_t blockTypesNum;
};

