#pragma once

#define WORD_SIZE 4
#define HEADER_SIZE 8

#define PACK(size , alloc) (size | alloc) // ��������� ������ �������� ����� � ������� ��������������

#define GET(pointer) (*(size_t*) (pointer)) // �������� ����� ������
#define PUT(pointer , value) ( *(size_t*) (pointer) = (value) ) // �������� ����� ������

#define GET_CURR_SIZE(bp) (GET(bp)) // �������� ������ �������� �����
#define GET_PREV_SIZE(bp) (GET(bp + WORD_SIZE) & ~0x1)  // �������� ������ �������� �����
#define GET_ALLOC(bp) (GET(bp + WORD_SIZE) & 0x1)  // �������� ������� ��������������

#define HDRP(bp) ((char*)bp - HEADER_SIZE) // �������� ����� ������ �� ��������� �� �������� �������

#define NEXT_BLOCK(bp) ((char*)bp + HEADER_SIZE + GET_CURR_SIZE(bp)) // �������� ����� ���������� �����
#define PREV_BLOCK(bp) ((char*)bp - HEADER_SIZE - GET_PREV_SIZE(bp)) // �������� ����� �������� �����

#define GET_MEM_PTR(bp) ((void*)(bp + HEADER_SIZE)) // �������� ��������� �� �������� ������ �� ������ ������


class Allocator
{
public:
	Allocator(size_t inMaxChunkSize);

	~Allocator();

	void* mem_alloc(size_t size); // ����������� ������

	void* mem_realloc(void* addr, size_t size); // ��������� ������� ����� (��� �����������������)

	void mem_free(void* addr); // ������������ ������

	void mem_dump(); // ����� ������ �� ���������

	void* chooseBlock(); // ��������� ������� ��������� ���� ��� mem_realloc � mem_free

protected:
	size_t minChunkSize; // ����������� ��������� ������ ��� ����� (��������� ������������)

	std::list<char*> chunks{}; // ��� ������ ������ (�������� �����������)

	char* addChunk(const size_t minSize); // ���������� ������ �����

	static size_t levelOutSize(const size_t size); // ��������� ������ ����� ��� ������ �����

	char* blockMerge(char* currBlock); // ������� ��������� ����� � ���� ���� ��������

	size_t getMergeSize(char* currBlock); // �������� ������ ����������� ������ (��� �������� ����� , ���� ����������)

	void seeBlockInfo(char* currBlock); // �������� ������ �� �����

	void* getBlockPtrByNum(short num); // �������� ���� �� ��� ���������� �������
};
