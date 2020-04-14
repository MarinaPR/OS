#pragma once

#define PAGE_SIZE 128
#define BLOCK_HEADER_SIZE 12

class PageAllocator
{
public:
	/**
	 * Initialize Allocator
	 * @param inMaxChunkSize - maximum size of chunk in bytes
	 */
	explicit PageAllocator(const size_t inMaxChunkSize);

	~PageAllocator();

	/* Get BlockAllocator pointer from page header
	 * @param addr pointer to some area in page
	 */
	class BlockAllocator* getBlockAllocator(void* addr);

	void setBlockAllocator(void* page, BlockAllocator* blockAllocator);

	void* mem_alloc(size_t size);

	void* mem_realloc(void* addr, size_t size);

	void mem_free(void* addr);

	void mem_dump() const;

protected:
	/**
	 * Init chunk
	 * @return pointer to created chunk
	 */
	char* initChunk();
	
	/**
	 * Get number of pages needed for allocating size bytes
	 */
	static size_t toPageNum(const size_t size);

	char* chunk;

	// Size of chunk
	size_t chunkSize;

public:
	struct MemoryBlockHeader
	{
		/**
		 * Get pointer to the next block
		 * @return pointer to the next block
		 *		   or nullptr if next block doesn't exist
		 */
		MemoryBlockHeader* nextBlock();

		/**
		 * Get pointer to the previous block
		 * @return pointer to the previous block
		 *		   or nullptr if previous block doesn't exist
		 */
		MemoryBlockHeader* prevBlock();

		/**
		 * Try to combine current block with previous and next blocks
		 * Must be called only on NOT allocated block
		 * @return new header of combined block
		 */
		MemoryBlockHeader* tryCombine();

		/**
		 * @return size of block after tryCombine() will be called
		 */
		size_t getCombinePageNum();

		static void* getMemPtr(MemoryBlockHeader* block);

		void outputInfo() const;

		/**
		 * Get size of block, which holds pageNum pages
		 * Note: As argument can be passed ONLY pageNum OR prevBlockPageNum
		 */
		static size_t toBytesSize(size_t pageNum);

		// Number of pages in this block
		// Note: pageNum * PAGE_SIZE - is NOT size of this block
		// @see MemoryBlockHeader::toBytesSize function
		size_t pageNum : 31;

		/* Number of pages in the previous block
		 * Note: prevBlockPageNum * PAGE_SIZE - is NOT size of previous block
		 * @see MemoryBlockHeader::toBytesSize function
		 */
		size_t prevBlockPageNum : 31;

		// Is block allocated
		size_t allocated : 1;

		// Pointer to BlockAllocator
		// if == nullptr then this page doesn't hold BlockAllocator
		size_t blockAllocatorPtr : 32;
	};
};
