// Lab2.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "Allocator.h"
#define _CRTDBG_MAP_ALLOC  
#include <crtdbg.h> 

struct AllocatedVar
{
	void* ptr;
	size_t size;
};

int main()
{
	{
		std::vector<AllocatedVar> lst;

		size_t bufferSize;
		printf("Enter buffer size: ");
		scanf_s("%d", &bufferSize);

		Allocator allocator(bufferSize);

		int choose;

		do
		{
			allocator.mem_dump();
			printf("\n\n");

			printf("List of allocated variables:\n");
			int i = 0;
			for (AllocatedVar curVar : lst)
			{
				printf("Variable [%d]\n\tVariable size: %d\n\n", i++, curVar.size);
			}

			printf("Choose action ([0] - allocate new variable, [1] - unallocate variable, [2] - reallocate variable, [3] - exit): ");
			scanf_s("%d", &choose);

			std::vector<AllocatedVar>::iterator iter;
			int varIndex;
			void* newPtr;

			switch (choose)
			{
			case 0:
				AllocatedVar newVar;
				printf("Enter new variable size: ");
				scanf_s("%d", &newVar.size);
				newVar.ptr = allocator.mem_alloc(newVar.size);

				lst.push_back(newVar);

				break;
			case 1:
				printf("Enter variable index: ");
				scanf_s("%d", &varIndex);

				iter = lst.begin() + varIndex;

				allocator.mem_free(iter->ptr);

				lst.erase(iter);
				break;
			case 2:
				printf("Enter variable index: ");
				scanf_s("%d", &varIndex);

				iter = lst.begin() + varIndex;

				size_t newSize;
				printf("Enter new size: ");
				scanf_s("%d", &newSize);

				newPtr = allocator.mem_realloc(iter->ptr, newSize);

				if (newPtr == nullptr)
				{
					printf("Can't reallocated variable.\n");
				}
				else
				{
					iter->ptr = newPtr;
					iter->size = newSize;
				}


				break;
			default:
				break;
			}
		} while (choose != 3);
	}



	_CrtDumpMemoryLeaks();

	_getch();

	return 0;
}

