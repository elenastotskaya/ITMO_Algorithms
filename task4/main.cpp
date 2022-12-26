#include "MemoryAllocator.h"
#include <iostream>
#include <algorithm>
#include <random>

int main() {
	MemoryAllocator allocator;
	allocator.init();
	int* pi = (int*)allocator.alloc(sizeof(int));
	*pi = 15;
	*pi += 2;
	std::cout << *pi;
	double* pd = (double*)allocator.alloc(sizeof(double));
	int* pa = (int*)allocator.alloc(10 * sizeof(int));
	//allocator.dumpStat();
	//allocator.dumpBlocks();
	allocator.free(pa);
	allocator.free(pd);
	allocator.free(pi);

	int* pa_big[32769];
	int indices[32769];
	for (int i = 0; i < 32769; ++i) {
		pa_big[i] = (int*)(allocator.alloc(512));
		indices[i] = i;
	}
	allocator.dumpStat();
	std::random_device rd;
	std::mt19937 g(rd());

	std::shuffle(indices, indices + 32769, g);
	for (int i : indices) {
		allocator.free(pa_big[i]);
	}
	allocator.dumpStat();

	int* p_coalesce_0 = (int*)allocator.alloc(1024);
	int* p_coalesce_1 = (int*)allocator.alloc(2048);
	int* p_coalesce_2 = (int*)allocator.alloc(1024);
	allocator.dumpStat();
	allocator.dumpBlocks();
	allocator.free(p_coalesce_2);
	allocator.dumpStat();
	allocator.dumpBlocks();
	allocator.free(p_coalesce_0);
	allocator.dumpStat();
	allocator.dumpBlocks();
	allocator.free(p_coalesce_1);
	allocator.dumpStat();
	allocator.dumpBlocks();

	int* p_coalesce_big[17];
	int indices_coalesce[17];
	for (int i = 0; i < 17; ++i) {
		p_coalesce_big[i] = (int*)(allocator.alloc(10*1024*1024));
		indices_coalesce[i] = i;
	}
	allocator.dumpStat();
	std::shuffle(indices_coalesce, indices_coalesce + 17, g);
	for (int i : indices_coalesce) {
		allocator.free(p_coalesce_big[i]);
	}
	allocator.dumpStat();

	allocator.destroy();
}
