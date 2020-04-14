#include "stdafx.h"
#include "PageAllocator.h"
#include "BlockAllocator.h"

PageAllocator::PageAllocator(const size_t inMinChunkSize)
	: chunkSize(inMinChunkSize)
{
	// How many pages could be places in chunk
	size_t pageNum = (chunkSize - BLOCK_HEADER_SIZE) / (PAGE_SIZE + BLOCK_HEADER_SIZE);

	if (pageNum < 1)
	{
		printf("\nError: PageAllocator chunkSize is too small\n");
		_getch();
		exit(EXIT_FAILURE);
	}

	chunkSize = (PAGE_SIZE + BLOCK_HEADER_SIZE)*pageNum + BLOCK_HEADER_SIZE;

	initChunk();
}

PageAllocator::~PageAllocator()
{
	delete chunk;
}

BlockAllocator* PageAllocator::getBlockAllocator(void* addr)
{
	size_t chunkPtr = reinterpret_cast<size_t>(chunk);
	size_t addrPtr = reinterpret_cast<size_t>(addr);

	size_t distFromBegin = addrPtr - chunkPtr;

	size_t pageIndex = distFromBegin / (PAGE_SIZE + BLOCK_HEADER_SIZE);

	MemoryBlockHeader* pageHeader = reinterpret_cast<MemoryBlockHeader*>(chunk + (pageIndex * (PAGE_SIZE + BLOCK_HEADER_SIZE)));

	return reinterpret_cast<BlockAllocator*>(pageHeader->blockAllocatorPtr);
}

void PageAllocator::setBlockAllocator(void* page, BlockAllocator* blockAllocator)
{
	MemoryBlockHeader* pageHeader = reinterpret_cast<MemoryBlockHeader*>(
		reinterpret_cast<char*>(page) - BLOCK_HEADER_SIZE);

	pageHeader->blockAllocatorPtr = reinterpret_cast<size_t>(blockAllocator);
}

void* PageAllocator::mem_alloc(size_t size)
{
	size_t newPageNum = toPageNum(size);

	MemoryBlockHeader* curBlock = nullptr;

	curBlock = reinterpret_cast<MemoryBlockHeader*>(chunk);

	while (curBlock->pageNum != 0 && (curBlock->allocated || curBlock->pageNum < newPageNum))
		curBlock = curBlock->nextBlock();


	if (curBlock == nullptr || curBlock->pageNum == 0)
	{
		// If block wasn't found
		// then return nullptr

		return nullptr;
	}

	if (curBlock->pageNum > newPageNum)
	{
		// If curBlock is bigger than needed
		// then split it in two blocks

		const size_t newNextPageNum = curBlock->pageNum - newPageNum;
		curBlock->pageNum = newPageNum;
		MemoryBlockHeader* NewNextBlock = curBlock->nextBlock();
		NewNextBlock->pageNum = newNextPageNum;
		NewNextBlock->allocated = 0;
		NewNextBlock->blockAllocatorPtr = reinterpret_cast<size_t>(nullptr);
		NewNextBlock->prevBlockPageNum = newPageNum;
		NewNextBlock->nextBlock()->prevBlockPageNum = NewNextBlock->pageNum;
	}

	curBlock->allocated = 1;

	void* curBlockMem = MemoryBlockHeader::getMemPtr(curBlock);
	memset(curBlockMem, 0, MemoryBlockHeader::toBytesSize(curBlock->pageNum));

	return curBlockMem;
}

void* PageAllocator::mem_realloc(void* addr, size_t size)
{
	if (addr == nullptr)
		return mem_alloc(size);

	size_t newPageNum = toPageNum(size);

	MemoryBlockHeader* curBlock = reinterpret_cast<MemoryBlockHeader*>(
		reinterpret_cast<char*>(addr) - BLOCK_HEADER_SIZE);

	const size_t combineBlockPageNum = curBlock->getCombinePageNum();

	if (combineBlockPageNum > newPageNum)
	{
		// If newCurBlock is bigger than needed
		// then split it in two blocks

		MemoryBlockHeader* newCurBlock = curBlock->tryCombine();
		newCurBlock->allocated = 1;

		const size_t newNextBlockPageNum = newCurBlock->pageNum - newPageNum;
		newCurBlock->pageNum = newPageNum;

		const size_t copySize = newPageNum < curBlock->pageNum
			? MemoryBlockHeader::toBytesSize(newPageNum)
			: MemoryBlockHeader::toBytesSize(curBlock->pageNum);

		if (newCurBlock != curBlock)
		{
			memmove(MemoryBlockHeader::getMemPtr(newCurBlock), MemoryBlockHeader::getMemPtr(curBlock),
				copySize);
		}

		memset(reinterpret_cast<char*>(newCurBlock) + BLOCK_HEADER_SIZE + copySize, 0, MemoryBlockHeader::toBytesSize(combineBlockPageNum) - copySize);

		MemoryBlockHeader* newNextBlock = newCurBlock->nextBlock();
		newNextBlock->pageNum = newNextBlockPageNum;
		newNextBlock->allocated = 0;
		newNextBlock->prevBlockPageNum = newPageNum;
		newNextBlock->nextBlock()->prevBlockPageNum = newNextBlock->pageNum;

		return MemoryBlockHeader::getMemPtr(newCurBlock);
	}

	if (combineBlockPageNum < newPageNum)
	{
		// If newCurBlock is less than needed
		// then allocate new memory

		void* newAllocMem = mem_alloc(size);

		if (newAllocMem == nullptr)
		{
			return nullptr;
		}

		memmove(newAllocMem, MemoryBlockHeader::getMemPtr(curBlock), MemoryBlockHeader::toBytesSize(curBlock->pageNum));

		mem_free(addr);

		return newAllocMem;
	}

	// If the combineBlockPageNum is exactly what we needed
	// then just return combined block
	MemoryBlockHeader* newCurBlock = curBlock->tryCombine();
	newCurBlock->allocated = 1;

	const size_t copySize = newPageNum < curBlock->pageNum
		? MemoryBlockHeader::toBytesSize(newPageNum)
		: MemoryBlockHeader::toBytesSize(curBlock->pageNum);
	if (newCurBlock != curBlock)
	{
		memmove(MemoryBlockHeader::getMemPtr(newCurBlock), MemoryBlockHeader::getMemPtr(curBlock),
			copySize);
	}
	memset(reinterpret_cast<char*>(newCurBlock) + BLOCK_HEADER_SIZE + copySize, 0, MemoryBlockHeader::toBytesSize(combineBlockPageNum) - copySize);

	return MemoryBlockHeader::getMemPtr(newCurBlock);
}

void PageAllocator::mem_free(void* addr)
{
	MemoryBlockHeader* curBlock = reinterpret_cast<MemoryBlockHeader*>(
		reinterpret_cast<char*>(addr) - BLOCK_HEADER_SIZE);
	curBlock->allocated = 0;
	curBlock->blockAllocatorPtr = reinterpret_cast<size_t>(nullptr);
	curBlock->tryCombine();
}

void PageAllocator::mem_dump() const
{
	printf("PageAllocator info:\n\tmaxChunkSize = %d\n",
		chunkSize);

	size_t allocatedMem = 0;
	size_t unallocatedMem = 0;
	size_t blocksNum = 0;

	printf("==================\n\tChunk:\n");

	size_t curChunkAllocatedMem = 0;
	size_t curChunkUnallocatedMem = 0;

	MemoryBlockHeader* curBlock = reinterpret_cast<MemoryBlockHeader*>(chunk);
	while (curBlock != nullptr)
	{
		printf("\nBlock [%d]:\n", blocksNum);
		curBlock->outputInfo();

		++blocksNum;
		if (curBlock->allocated)
		{
			allocatedMem += curBlock->pageNum;
			curChunkAllocatedMem += curBlock->pageNum;
		}
		else
		{
			unallocatedMem += curBlock->pageNum;
			curChunkUnallocatedMem += curBlock->pageNum;
		}

		curBlock = curBlock->nextBlock();
	}

	printf("\nChunk info:\n\tAllocated memory: %d\n\tUnllocated momory: %d\n==================\n",
		curChunkAllocatedMem, curChunkUnallocatedMem);
}

char* PageAllocator::initChunk()
{
	chunk = new char[chunkSize];

	// How many pages could be places in chunk
	size_t pageNum = (chunkSize - BLOCK_HEADER_SIZE) / (PAGE_SIZE + BLOCK_HEADER_SIZE);

	MemoryBlockHeader* startBlock = reinterpret_cast<MemoryBlockHeader*>(chunk);
	startBlock->pageNum = pageNum;
	startBlock->prevBlockPageNum = 0;
	startBlock->blockAllocatorPtr = 0;
	startBlock->allocated = 0;

	MemoryBlockHeader* endBlock = startBlock->nextBlock();
	endBlock->pageNum = 0;
	endBlock->prevBlockPageNum = startBlock->pageNum;
	endBlock->blockAllocatorPtr = 0;
	endBlock->allocated = 1;

	return chunk;
}

size_t PageAllocator::toPageNum(const size_t size)
{
	const size_t newPageNum = size / PAGE_SIZE;
	return (newPageNum*PAGE_SIZE) < size
		? newPageNum + 1
		: newPageNum;
}

PageAllocator::MemoryBlockHeader* PageAllocator::MemoryBlockHeader::nextBlock()
{
	return reinterpret_cast<MemoryBlockHeader*>(pageNum == 0
		? nullptr
		: reinterpret_cast<char*>(this) + toBytesSize(pageNum) +
		BLOCK_HEADER_SIZE);
}

PageAllocator::MemoryBlockHeader* PageAllocator::MemoryBlockHeader::prevBlock()
{
	return reinterpret_cast<MemoryBlockHeader*>(prevBlockPageNum == 0
		? nullptr
		: reinterpret_cast<char*>(this) - toBytesSize(prevBlockPageNum) -
		BLOCK_HEADER_SIZE
		);
}

PageAllocator::MemoryBlockHeader* PageAllocator::MemoryBlockHeader::tryCombine()
{
	MemoryBlockHeader* next = nextBlock();
	MemoryBlockHeader* prev = prevBlock();

	if (prev != nullptr && !prev->allocated && next != nullptr && !next->allocated)
	{
		// Combine prevoius, current and next blocks

		prev->pageNum += pageNum + next->pageNum;

		MemoryBlockHeader* nextNext = next->nextBlock();
		if (nextNext != nullptr)
			nextNext->prevBlockPageNum = prev->pageNum;

		return prev;
	}

	if (prev != nullptr && !prev->allocated)
	{
		// Combine previous and current blocks

		prev->pageNum += pageNum;

		if (next != nullptr)
			next->prevBlockPageNum = prev->pageNum;

		return prev;
	}

	if (next != nullptr && !next->allocated)
	{
		// Combine current and next blocks

		pageNum += next->pageNum;

		MemoryBlockHeader* nextNext = next->nextBlock();
		if (nextNext != nullptr)
			nextNext->prevBlockPageNum = pageNum;

		return this;
	}

	return this;
}

size_t PageAllocator::MemoryBlockHeader::getCombinePageNum()
{
	MemoryBlockHeader* next = nextBlock();
	MemoryBlockHeader* prev = prevBlock();

	if (prev != nullptr && !prev->allocated && next != nullptr && !next->allocated)
	{
		// Prevoius, current and next blocks can be combined

		return prev->pageNum + pageNum + next->pageNum;
	}

	if (prev != nullptr && !prev->allocated)
	{
		// Previous and current blocks can be combined

		return prev->pageNum + pageNum;
	}

	if (next != nullptr && !next->allocated)
	{
		// Current and next blocks can be combined

		return pageNum + next->pageNum;
	}

	return pageNum;
}

void* PageAllocator::MemoryBlockHeader::getMemPtr(MemoryBlockHeader* block)
{
	return reinterpret_cast<char*>(block) + BLOCK_HEADER_SIZE;
}

void PageAllocator::MemoryBlockHeader::outputInfo() const
{
	printf("\tallocated = %d\n\tpageNum = %d\n\tprevPageNum = %d\n", allocated, pageNum, prevBlockPageNum);
}

size_t PageAllocator::MemoryBlockHeader::toBytesSize(size_t pageNum)
{
	// Block holds pageNum * PAGE_SIZE bytes
	// PLUS size of their headers (EXCLUDING size of THIS MemoryBlockHeader)
	return pageNum * PAGE_SIZE + BLOCK_HEADER_SIZE * (pageNum - 1);
}

