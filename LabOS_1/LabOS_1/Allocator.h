#pragma once

#define WORD_SIZE 4
#define HEADER_SIZE 8

#define PACK(size , alloc) (size | alloc) // Сойденяем размер прошлого блока и признак алоцированости

#define GET(pointer) (*(size_t*) (pointer)) // Получаем слово данных
#define PUT(pointer , value) ( *(size_t*) (pointer) = (value) ) // Помещаем слово данных

#define GET_CURR_SIZE(bp) (GET(bp)) // Получаем размер текущего блока
#define GET_PREV_SIZE(bp) (GET(bp + WORD_SIZE) & ~0x1)  // Получаем размер прошлого блока
#define GET_ALLOC(bp) (GET(bp + WORD_SIZE) & 0x1)  // Получаем признак алоцированости

#define HDRP(bp) ((char*)bp - HEADER_SIZE) // Получаем адрес хедера по указателю на полезную область

#define NEXT_BLOCK(bp) ((char*)bp + HEADER_SIZE + GET_CURR_SIZE(bp)) // Получаем хедер следуйщего блока
#define PREV_BLOCK(bp) ((char*)bp - HEADER_SIZE - GET_PREV_SIZE(bp)) // Получаем хедер прошлого блока

#define GET_MEM_PTR(bp) ((void*)(bp + HEADER_SIZE)) // Получаем указатель на полезную память из адреса хедера


class Allocator
{
public:
	Allocator(size_t inMaxChunkSize);

	~Allocator();

	void* mem_alloc(size_t size); // Выделеление памяти

	void* mem_realloc(void* addr, size_t size); // Изменение размера блока (или перераспределение)

	void mem_free(void* addr); // Освобождение памяти

	void mem_dump(); // Вывод сводки по алокатору

	void* chooseBlock(); // Позволяет выбрать требуемый блок для mem_realloc и mem_free

protected:
	size_t minChunkSize; // Минимальный возможный размер для чанка (адресного пространства)

	std::list<char*> chunks{}; // Наш список чанков (адресных пространств)

	char* addChunk(const size_t minSize); // Добавление нового чанка

	static size_t levelOutSize(const size_t size); // Подгоняем размер блока под размер слова

	char* blockMerge(char* currBlock); // Сливаем свободные блоки в один если возможно

	size_t getMergeSize(char* currBlock); // Получаем размер сойдененных блоков (или поданого блока , если невозможно)

	void seeBlockInfo(char* currBlock); // Получаем сводку по блоку

	void* getBlockPtrByNum(short num); // Получаем блок за его порядковым номером
};
