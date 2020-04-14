#pragma once
class BlockAllocator
{
public:
	BlockAllocator(char* inPagePtr, size_t pageSize, size_t inBlockSize, std::list<BlockAllocator*>* inOwnerList);
	~BlockAllocator();

	void* mem_alloc();
	void mem_free(void* addr);
	void mem_dump() const;

	bool isPageFree();

	bool isFull();

	std::list<BlockAllocator*>* getOwnerList();
	void* getPagePtr();
	size_t getBlockSize();

protected:
	struct MemoryBlockHeader
	{
		size_t nextBlock : 32;
	};
	
	
	std::list<BlockAllocator*>* ownerList;
	char* pagePtr;
	MemoryBlockHeader* firstFreeBlock;
	size_t totalBlocksNum;
	size_t freeBlocksNum;
	size_t blockSize;

	
};

