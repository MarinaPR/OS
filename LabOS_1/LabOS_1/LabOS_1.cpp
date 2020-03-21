#include "stdafx.h"
#include "Allocator.h"

using namespace std;

int main()
{
	
	size_t size;
	cout << " > First chunk creation. Size : "; cin >> size;
	Allocator myAllocator(size);
	int cond;
	cout << " > Available options :" << endl;
	cout << "	1) Allocate block ;" << endl;
	cout << "	2) Reallocate block ;" << endl;
	cout << "	3) Delete block ;" << endl;
	cout << "	4) Memory dump ;" << endl;
	cout << "	5) Exit program ;" << endl;
	cout << " > Choose option: "; cin >> cond;
	while (cond != 5)
	{
		if (cond == 1)
		{
			size_t tempSize;
			cout << " > Input wanted size (in bytes): "; cin >> tempSize;
			char* ptr = (char*)(myAllocator.mem_alloc(sizeof(char)*tempSize));
		}
		if (cond == 2)
		{
			size_t tempSize;
			char* ptr = (char*)(myAllocator.chooseBlock());
			if (ptr == nullptr) cout << " > Error : All blocks are free!" << endl;
			else
			{
				cout << " > Input wanted new size (in bytes) : "; cin >> tempSize;
				myAllocator.mem_realloc(ptr, tempSize);
			}
		}
		if (cond == 3)
		{
			char* ptr = (char*)(myAllocator.chooseBlock());
			if (ptr == nullptr) cout << " > Error : All blocks are free!" << endl;
			else myAllocator.mem_free(ptr);
		}
		if (cond == 4)
		{
			myAllocator.mem_dump();
		}
		cout << endl << " > Choose option: "; cin >> cond;
	}

	_getch();
	return 0;
}
