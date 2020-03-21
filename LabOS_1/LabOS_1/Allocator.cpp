#include "stdafx.h"
#include "Allocator.h"
#include <cassert>

using namespace std;

Allocator::Allocator(size_t inputChunkSize)
	
{
	if (inputChunkSize < (2 * HEADER_SIZE + WORD_SIZE))
	{
		cout << endl << " > Error: Stated allocator size is unapropriate! " << endl;
		_getch();
		exit(EXIT_FAILURE);
	}

	minChunkSize = levelOutSize(inputChunkSize);

	addChunk(minChunkSize);
}

Allocator::~Allocator()
{
	for (char* chunk : chunks)
		delete chunk;
}

void* Allocator::mem_alloc(size_t size)
{
	size = levelOutSize(size);

	char* currBlock = nullptr;

	for (char* currChunk : chunks)
	{
		currBlock = currChunk;

		while ( (GET_CURR_SIZE(currBlock) != 0) && ( GET_ALLOC(currBlock) || (GET_CURR_SIZE(currBlock) < size)) )
			currBlock = NEXT_BLOCK(currBlock);

		if (GET_CURR_SIZE(currBlock) != 0) break; // Если не последний блок , то нашли подходящий
	}

	if ( (GET_CURR_SIZE(currBlock - HEADER_SIZE) == 0) || (GET_CURR_SIZE(currBlock) == 0) )
	{
		char* newChunk = addChunk(size + 2 * HEADER_SIZE); // Создаем новую адресную область , если не нашли подходящий блок
		currBlock = newChunk;
	}
	
	if (GET_CURR_SIZE(currBlock) >= size + HEADER_SIZE + WORD_SIZE)
	{
		// Делим ,если блок сильно большой
		size_t newBlockSize = GET_CURR_SIZE(currBlock) - size - HEADER_SIZE;
		PUT(currBlock, size);

		char* newBlock = NEXT_BLOCK(currBlock);

		PUT(newBlock , newBlockSize);
		PUT(newBlock + WORD_SIZE, PACK(size, 0));
		PUT(NEXT_BLOCK(newBlock) + WORD_SIZE, PACK(newBlockSize, GET_ALLOC(NEXT_BLOCK(newBlock))));

	}

	PUT(currBlock + WORD_SIZE, PACK(GET_PREV_SIZE(currBlock),1));

	return GET_MEM_PTR(currBlock);
}

void* Allocator::mem_realloc(void* addr, size_t size)
{
	if (addr == nullptr)
		return mem_alloc(size);

	size = levelOutSize(size);

	char *currBlock = (char*)addr - HEADER_SIZE;

	int mergedBlocksSize = getMergeSize(currBlock);

	if (mergedBlocksSize >= size + HEADER_SIZE + WORD_SIZE)
	{
		// Делим ,если блок сильно большой
		char* newCurrBlock = blockMerge(currBlock);
		PUT(newCurrBlock + WORD_SIZE, PACK(GET_PREV_SIZE(newCurrBlock), 1));

		size_t newBlockSize = GET_CURR_SIZE(newCurrBlock) - size - HEADER_SIZE;
		PUT(newCurrBlock, size);

		size_t amountToCopy = size < GET_CURR_SIZE(currBlock) ? size : GET_CURR_SIZE(currBlock);
		if (newCurrBlock != currBlock)
		{
			memmove(GET_MEM_PTR(newCurrBlock), GET_MEM_PTR(currBlock), amountToCopy);
		}

		char* newNextBlock = NEXT_BLOCK(newCurrBlock);

		PUT(newNextBlock, newBlockSize);
		PUT(newNextBlock + WORD_SIZE, PACK(size, 0));
		PUT(NEXT_BLOCK(newNextBlock) + WORD_SIZE, PACK(newBlockSize, GET_ALLOC(NEXT_BLOCK(newNextBlock))));

		return GET_MEM_PTR(newCurrBlock);
	}
	else if (mergedBlocksSize < size)
	{
		// Делим ,если блок меньше чем надо , то просто выделяем еще память
		void* newCurrBlock = mem_alloc(size);

		if (newCurrBlock == nullptr)
		{
			return nullptr;
		}
		
		memmove(newCurrBlock, GET_MEM_PTR(currBlock) , size);

		mem_free(addr);

		return newCurrBlock;
	}

	// Если слитый блок , подходящего размера , то его и возвращаем
	char* newCurrBlock = blockMerge(currBlock);
	PUT(newCurrBlock + WORD_SIZE, PACK(GET_PREV_SIZE(newCurrBlock), 1));

	size_t amountToCopy = size < GET_CURR_SIZE(currBlock) ? size : GET_CURR_SIZE(currBlock);
	if (newCurrBlock != currBlock)
	{
		memmove(GET_MEM_PTR(newCurrBlock), GET_MEM_PTR(currBlock), amountToCopy);
	}

	return GET_MEM_PTR(newCurrBlock);
}

void Allocator::mem_free(void* addr)
{
	char *currBlock = (char*)addr - HEADER_SIZE;
	PUT(currBlock + WORD_SIZE, PACK(GET_PREV_SIZE(currBlock), 0));
	blockMerge(currBlock);
}

void Allocator::mem_dump()
{
	cout << " > Allocator statistics : " << endl 
		 << "	- Minimum size of chunk = " << minChunkSize << endl 
		 << "	- Amount of chunks = " << chunks.size() << endl;

	unsigned int allocatedMem = 0;
	unsigned int freeMem = 0;
	unsigned int blockNum = 1;
	unsigned int chunkNum = 1;

	for (char* currChunk : chunks)
	{
		cout << endl << " > ----- Chunk #" << chunkNum << " -----" << endl;

		unsigned int currChunkAllocatedMem = 0;
		unsigned int currChunkFreeMem = 0;
		char *currBlock = currChunk;
		
		while (true){
			cout << endl << " > Block #" << blockNum << " : ";

			seeBlockInfo(currBlock);

			if (GET_ALLOC(currBlock))
			{
				allocatedMem += GET_CURR_SIZE(currBlock);
				currChunkAllocatedMem += GET_CURR_SIZE(currBlock);
			}
			else
			{
				freeMem += GET_CURR_SIZE(currBlock);
				currChunkFreeMem += GET_CURR_SIZE(currBlock);
			}

			blockNum++;

			if (GET_CURR_SIZE(currBlock) != 0) currBlock = NEXT_BLOCK(currBlock);
			else break;
		} 

		cout << endl << " > Chunk statistics : " << endl
			<< "	- Allocated memory = " << currChunkAllocatedMem << endl
			<< "	- Free memory = " << currChunkFreeMem << endl;
		
		chunkNum++;
	}

	cout << endl << " > Summary on all chunks : " << endl
		<< "	- Allocated memory = " << allocatedMem << endl
		<< "	- Free memory = " << freeMem << endl;
}

char* Allocator::addChunk(size_t minSize)
{
	size_t newChunkSize = minSize < minChunkSize ? minChunkSize : minSize;
	char* newChunk = new char[newChunkSize];
	chunks.push_back(newChunk);

	char* startBlock = newChunk;
	PUT(startBlock, newChunkSize - 2 * HEADER_SIZE);
	PUT(startBlock + WORD_SIZE, 0);

	char* endBlock = NEXT_BLOCK(startBlock);
	PUT(endBlock , 0);
	PUT(endBlock + WORD_SIZE, PACK(newChunkSize - 2 * HEADER_SIZE, 1));

	return newChunk;
}

size_t Allocator::levelOutSize(size_t size)
{
	return (size % WORD_SIZE == 0) ? size : (size - (size % WORD_SIZE) + WORD_SIZE);
}

char* Allocator::blockMerge(char *currBlock)
{
	char* next = NEXT_BLOCK(currBlock);
	char* prev = PREV_BLOCK(currBlock);

	bool nextAlloc = GET_ALLOC(next);
	bool prevAlloc;
	if (GET_PREV_SIZE(currBlock) == 0) prevAlloc = true;
	else prevAlloc = GET_ALLOC(prev);

	if (prev != nullptr && !prevAlloc && next != nullptr && !nextAlloc)
	{
		PUT(prev, GET_CURR_SIZE(prev) + GET_CURR_SIZE(currBlock) + GET_CURR_SIZE(next) + 2 * HEADER_SIZE);

		PUT(NEXT_BLOCK(next) + WORD_SIZE, PACK(GET_CURR_SIZE(prev), GET_ALLOC(NEXT_BLOCK(next))) );

		return prev;
	}
	else if (prev != nullptr && !prevAlloc)
	{
		PUT(prev, GET_CURR_SIZE(prev) + GET_CURR_SIZE(currBlock) + HEADER_SIZE);

		PUT(next + WORD_SIZE, PACK(GET_CURR_SIZE(prev), GET_ALLOC(next)));

		return prev;
	}
	else if (next != nullptr && !nextAlloc)
	{
		PUT(currBlock , GET_CURR_SIZE(currBlock) + GET_CURR_SIZE(next) + HEADER_SIZE);

		PUT(NEXT_BLOCK(next) + WORD_SIZE, PACK(GET_CURR_SIZE(prev), GET_ALLOC(NEXT_BLOCK(next))));

		return currBlock;
	}

	return currBlock;
}


size_t Allocator::getMergeSize(char* currBlock)
{
	char* next = NEXT_BLOCK(currBlock);
	char* prev = PREV_BLOCK(currBlock);

	bool nextAlloc = GET_ALLOC(next);
	bool prevAlloc;
	if (GET_PREV_SIZE(currBlock) == 0) prevAlloc = true;
	else prevAlloc = GET_ALLOC(prev);

	if (prev != nullptr && !prevAlloc && next != nullptr && !nextAlloc)
	{
		return (GET_CURR_SIZE(prev) + GET_CURR_SIZE(currBlock) + GET_CURR_SIZE(next) + 2 * HEADER_SIZE);
	}

	if (prev != nullptr && !prevAlloc)
	{
		return (GET_CURR_SIZE(prev) + GET_CURR_SIZE(currBlock) + HEADER_SIZE);
	}

	if (next != nullptr && !nextAlloc)
	{
		return (GET_CURR_SIZE(currBlock) + GET_CURR_SIZE(next) + HEADER_SIZE);
	}

	return GET_CURR_SIZE(currBlock);
}

void Allocator::seeBlockInfo(char* currBlock)
{
	cout << "Allocated (" << GET_ALLOC(currBlock) << ") | Size (" << setw(4) << GET_CURR_SIZE(currBlock);
	cout << ") | Prev. Size (" << setw(4) << GET_PREV_SIZE(currBlock) << ")" << endl;
}


void* Allocator::chooseBlock()
{
	cout << " > List of suitable blocks : " << endl;

	unsigned int blockNum = 0;

	for (char* currChunk : chunks)
	{
		char *currBlock = currChunk;

		while (true) {
			blockNum++;

			if (GET_ALLOC(currBlock) && GET_CURR_SIZE(currBlock) != 0) {
				cout << endl << " - Block #" << blockNum << " : ";
				seeBlockInfo(currBlock);
			}

			if (GET_CURR_SIZE(currBlock) != 0) currBlock = NEXT_BLOCK(currBlock);
			else break;
		}
	}

	short block;
	cout << endl << " > Choose your block : " ; cin >> block;
	void* searchedBlock = getBlockPtrByNum(block);

	while (block < 1 || block > blockNum - 1 || (!GET_ALLOC((char*)searchedBlock - HEADER_SIZE)) || (GET_CURR_SIZE((char*)searchedBlock - HEADER_SIZE) == 0)) {
		cout << " > Wrong block chosen!!!" << endl;
		cout << endl << " > Choose your block : " ;	cin >> block;
		searchedBlock = getBlockPtrByNum(block);
	}

	return searchedBlock;
}

void* Allocator::getBlockPtrByNum(short num) {
	unsigned int blockNum = 0;

	for (char* currChunk : chunks)
	{
		char *currBlock = currChunk;

		while (true) {
			blockNum++;

			if (blockNum == num) {return (void*)(currBlock+HEADER_SIZE);  }

			if (GET_CURR_SIZE(currBlock) != 0) currBlock = NEXT_BLOCK(currBlock);
			else break;
		}
	}

	return nullptr;
}
