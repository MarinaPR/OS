#include "stdafx.h"
#include "Allocator.h"

#define WORD_SIZE 4
#define MAX_PAGES_NUM 15

Allocator::Allocator(size_t pagesNum)
{
	if(pagesNum > MAX_PAGES_NUM)
	{
		printf("Error: Wrong number of pages.\n");
		_getch();
	}

	pageAllocator = new PageAllocator(pagesNum*(PAGE_SIZE + BLOCK_HEADER_SIZE) + BLOCK_HEADER_SIZE);

	blockTypesNum = static_cast<size_t>(log2(PAGE_SIZE / 2) - 1);
	notFullBlockAllocatorsLists = new std::list<BlockAllocator*>*[blockTypesNum];

	for (size_t i = 0; i < blockTypesNum; i++)
	{
		notFullBlockAllocatorsLists[i] = new std::list<BlockAllocator*>();
	}
}


Allocator::~Allocator()
{
	for (BlockAllocator* curBlockAllocator : allBlockAllocators)
	{
		delete curBlockAllocator;
	}

	for (size_t i = 0; i < blockTypesNum; i++)
	{
		delete notFullBlockAllocatorsLists[i];
	}
	delete[] notFullBlockAllocatorsLists;

	delete pageAllocator;
}


void* Allocator::mem_alloc(size_t size)
{
	if (size > PAGE_SIZE / 2)
	{
		return pageAllocator->mem_alloc(size);
	}

	size_t log2size = static_cast<size_t>(log2(size));

	size_t blockTypeIndex;

	if (pow(2, log2size) < size)
	{
		blockTypeIndex = log2size - 1;
	}
	else
	{
		blockTypeIndex = log2size - 2;
	}

	BlockAllocator* blockAllocator = nullptr;

	if (notFullBlockAllocatorsLists[blockTypeIndex]->empty())
	{
		char* pagePtr = reinterpret_cast<char*>(pageAllocator->mem_alloc(PAGE_SIZE));
		if (pagePtr == nullptr)
			return nullptr;

		blockAllocator = new BlockAllocator(pagePtr, PAGE_SIZE, static_cast<size_t>(pow(2, blockTypeIndex + 2)), notFullBlockAllocatorsLists[blockTypeIndex]);


		pageAllocator->setBlockAllocator(pagePtr, blockAllocator);


		notFullBlockAllocatorsLists[blockTypeIndex]->push_back(blockAllocator);
		allBlockAllocators.push_back(blockAllocator);
	}
	else
	{
		blockAllocator = notFullBlockAllocatorsLists[blockTypeIndex]->front();
	}


	void* allocatedMem = blockAllocator->mem_alloc();

	if (blockAllocator->isFull())
	{
		notFullBlockAllocatorsLists[blockTypeIndex]->remove(blockAllocator);
	}

	return allocatedMem;
}

void* Allocator::mem_realloc(void* addr, size_t size)
{
	void* newAddr = mem_alloc(size);

	if(newAddr == nullptr)
	{
		return nullptr;
	}

	size_t copySize;
	BlockAllocator* blockAllocator = pageAllocator->getBlockAllocator(addr);

	if (blockAllocator == nullptr)
	{
		copySize = size < PAGE_SIZE ? size : PAGE_SIZE;
		memmove(newAddr, addr, copySize);
		
		pageAllocator->mem_free(addr);

		return newAddr;
	}

	size_t blockSize = blockAllocator->getBlockSize();
	copySize = size < blockSize ? size : blockSize;

	memmove(newAddr, addr, copySize);


	bool isInList = !blockAllocator->isFull();

	blockAllocator->mem_free(addr);
	if (blockAllocator->isPageFree())
	{
		if (isInList)
		{
			std::list<BlockAllocator*>* blockAllocatorOwnerList = blockAllocator->getOwnerList();
			blockAllocatorOwnerList->remove(blockAllocator);
		}
		allBlockAllocators.remove(blockAllocator);

		void* blockAllocatorPagePtr = blockAllocator->getPagePtr();
		delete blockAllocator;
		pageAllocator->mem_free(blockAllocatorPagePtr);

	}
	else if (!isInList)
	{
		size_t blockSize = blockAllocator->getBlockSize();

		size_t blockTypeIndex = static_cast<size_t>(log2(blockSize) - 2);

		notFullBlockAllocatorsLists[blockTypeIndex]->push_back(blockAllocator);
	}

	

	return newAddr;
}

void Allocator::mem_free(void* addr)
{
	BlockAllocator* blockAllocator = pageAllocator->getBlockAllocator(addr);

	if (blockAllocator == nullptr)
	{
		pageAllocator->mem_free(addr);
		return;
	}

	bool isInList = !blockAllocator->isFull();

	blockAllocator->mem_free(addr);
	if (blockAllocator->isPageFree())
	{
		if (isInList)
		{
			std::list<BlockAllocator*>* blockAllocatorOwnerList = blockAllocator->getOwnerList();
			blockAllocatorOwnerList->remove(blockAllocator);
		}
		allBlockAllocators.remove(blockAllocator);

		void* blockAllocatorPagePtr = blockAllocator->getPagePtr();
		delete blockAllocator;
		pageAllocator->mem_free(blockAllocatorPagePtr);

	}
	else if (!isInList)
	{
		size_t blockSize = blockAllocator->getBlockSize();

		size_t blockTypeIndex = static_cast<size_t>(log2(blockSize) - 2);

		notFullBlockAllocatorsLists[blockTypeIndex]->push_back(blockAllocator);
	}
}

void Allocator::mem_dump() const
{
	pageAllocator->mem_dump();
	printf("\nTotal number of block allocators: %d\n\n", allBlockAllocators.size());

	for (size_t i = 0; i < blockTypesNum; i++)
	{
		printf("\nBlock allocator type [%d]:\n\tNumber of doesn't full BlockAllocators: %d\n", i, notFullBlockAllocatorsLists[i]->size());
		for (BlockAllocator* curBlockAllocator : *notFullBlockAllocatorsLists[i])
		{
			curBlockAllocator->mem_dump();
		}
	}
}

size_t Allocator::alignSize(const size_t size)
{
	const size_t newSize = size / WORD_SIZE * WORD_SIZE;
	return newSize < size
		? newSize + WORD_SIZE
		: newSize;
}

