#include "stdafx.h"
#include "BlockAllocator.h"


BlockAllocator::BlockAllocator(char* inPagePtr, size_t pageSize, size_t inBlockSize, std::list<BlockAllocator*>* inOwnerList)
	: pagePtr(inPagePtr), blockSize(inBlockSize), ownerList(inOwnerList)
{
	totalBlocksNum = pageSize / inBlockSize;
	freeBlocksNum = totalBlocksNum;

	firstFreeBlock = reinterpret_cast<MemoryBlockHeader*>(pagePtr);

	for (size_t i = 0; i < freeBlocksNum - 1; i++)
	{
		MemoryBlockHeader* curBlock = reinterpret_cast<MemoryBlockHeader*>(pagePtr + (blockSize * i));
		curBlock->nextBlock = reinterpret_cast<size_t>(pagePtr + (blockSize * (i + 1)));
	}

	MemoryBlockHeader* lastBlock = reinterpret_cast<MemoryBlockHeader*>(pagePtr + (blockSize * (freeBlocksNum - 1)));
	lastBlock->nextBlock = reinterpret_cast<size_t>(nullptr);
}


BlockAllocator::~BlockAllocator()
{
}

void* BlockAllocator::mem_alloc()
{
	if(freeBlocksNum == 0)
		return nullptr;

	void* blockPtr = firstFreeBlock;

	firstFreeBlock = reinterpret_cast<MemoryBlockHeader*>(firstFreeBlock->nextBlock);

	--freeBlocksNum;

	return blockPtr;
}

void BlockAllocator::mem_free(void* addr)
{
	MemoryBlockHeader* newFirstBlock = reinterpret_cast<MemoryBlockHeader*>(addr);
	newFirstBlock->nextBlock = reinterpret_cast<size_t>(firstFreeBlock);
	firstFreeBlock = newFirstBlock;
	++freeBlocksNum;
}

void BlockAllocator::mem_dump() const
{
	printf("BlockAllocator info:\n\ttotalBlocksNum: %d\n\tfreeBlocksNum: %d\n\tblockSize: %d\n",
		totalBlocksNum, freeBlocksNum, blockSize);
}

bool BlockAllocator::isPageFree()
{
	return freeBlocksNum == totalBlocksNum;
}

bool BlockAllocator::isFull()
{
	return freeBlocksNum == 0;
}

std::list<BlockAllocator*>* BlockAllocator::getOwnerList()
{
	return ownerList;
}

void * BlockAllocator::getPagePtr()
{
	return pagePtr;
}

size_t BlockAllocator::getBlockSize()
{
	return blockSize;
}