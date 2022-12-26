#include "MemoryAllocator.h"
#include <Windows.h>
#include <iostream>
#include <cassert>

MemoryAllocator::FixedSizeAllocator::FixedSizeAllocator() {}

MemoryAllocator::FixedSizeAllocator::FixedSizeAllocator(size_t size)
	: blockSize_(size),
	pFirstPage_(nullptr),
	numPageBlocks_(1)
#ifdef _DEBUG
	, numFreeBlocks_(0),
	numUsedBlocks_(0),
	usedMemory_(0)
#endif // _DEBUG
{}

MemoryAllocator::CoalesceAllocator::CoalesceAllocator()
	: pFirstPage_(nullptr),
	numPageBlocks_(1)
#ifdef _DEBUG
	, numFreeBlocks_(0),
	numUsedBlocks_(0),
	usedMemory_(0)
#endif // _DEBUG
{}

MemoryAllocator::MemoryAllocator()
#ifdef _DEBUG
	: isInitialized_(false)
#endif // _DEBUG
{
	int index = 0;
	for (size_t i = MIN_FIXED_SIZE; i <= FIXED_ALLOCATION_THRESHOLD; i *= 2) {
		fsa_[index] = FixedSizeAllocator(i);
		++index;
	}
}

void MemoryAllocator::FixedSizeAllocator::init(void* start)
{
#ifdef _DEBUG
	usedMemory_ += (long)FIXED_PAGE_SIZE;
	numFreeBlocks_ += (FIXED_PAGE_SIZE - sizeof(FSAHeader)) / blockSize_;
#endif // _DEBUG
	pFirstPage_ = VirtualAlloc(start, FIXED_PAGE_SIZE, MEM_COMMIT, PAGE_READWRITE);
	((FSAHeader*)pFirstPage_)->pBuf_ = (char*)pFirstPage_ + sizeof(FSAHeader);
	((FSAHeader*)pFirstPage_)->pNextPage_ = nullptr;
	((FSAHeader*)pFirstPage_)->fHead_ = ((FSAHeader*)pFirstPage_)->pBuf_;
	((FSAHeader*)pFirstPage_)->numInitialized_ = 1;
	((FSAHeader*)pFirstPage_)->isInitialized_ = false;
#ifdef _DEBUG
	((FSAHeader*)pFirstPage_)->usedBlockFlags_ = new bool[(FIXED_PAGE_SIZE - sizeof(FSAHeader)) / blockSize_];
	for (int i = 0; i < (FIXED_PAGE_SIZE - sizeof(FSAHeader)) / blockSize_; ++i) {
		((FSAHeader*)pFirstPage_)->usedBlockFlags_[i] = false;
	}
#endif // _DEBUG
}

void MemoryAllocator::CoalesceAllocator::init(void* start)
{
	pFirstPage_ = VirtualAlloc(start, COALESCE_PAGE_SIZE, MEM_COMMIT, PAGE_READWRITE);
	((CoalesceHeader*)pFirstPage_)->pBuf_ = (char*)pFirstPage_ + sizeof(CoalesceHeader);
	((CoalesceHeader*)pFirstPage_)->pNextPage_ = nullptr;
	((CoalesceBlockData*)((CoalesceHeader*)pFirstPage_)->pBuf_)->pPrev = nullptr;
	((CoalesceBlockData*)((CoalesceHeader*)pFirstPage_)->pBuf_)->pNext = nullptr;
	((CoalesceBlockData*)((CoalesceHeader*)pFirstPage_)->pBuf_)->isFree = true;
	((CoalesceHeader*)pFirstPage_)->fHead_ = (char*)((CoalesceHeader*)pFirstPage_)->pBuf_ + sizeof(CoalesceBlockData);
	((CoalesceFreeEl*)((CoalesceHeader*)pFirstPage_)->fHead_)->pPrevFree = nullptr;
	((CoalesceFreeEl*)((CoalesceHeader*)pFirstPage_)->fHead_)->pNextFree = nullptr;
	((CoalesceFreeEl*)((CoalesceHeader*)pFirstPage_)->fHead_)->blockSize =
		COALESCE_PAGE_SIZE - sizeof(CoalesceBlockData) - sizeof(CoalesceHeader);
#ifdef _DEBUG
	usedMemory_ += (long)COALESCE_PAGE_SIZE;
	numFreeBlocks_ = 1;
#endif // _DEBUG
}

void MemoryAllocator::init()
{
#ifdef _DEBUG
	assert(isInitialized_ == false);
#endif // _DEBUG
	for (int i = 0; i < N_FIXED_SIZE_ALLOCATORS; ++i) {
		fsa_[i].init(VirtualAlloc(NULL, FIXED_PAGE_SIZE * ALLOCATOR_PAGES_IN_BLOCK, MEM_RESERVE, PAGE_READWRITE));
	}
	coalesceAllocator_.init(VirtualAlloc(NULL, COALESCE_PAGE_SIZE * ALLOCATOR_PAGES_IN_BLOCK, MEM_RESERVE, PAGE_READWRITE));
#ifdef _DEBUG
	isInitialized_ = true;
#endif // _DEBUG
}

void MemoryAllocator::FixedSizeAllocator::initElement(FSAHeader* header)
{
	assert(pFirstPage_ != nullptr);
	*((void**)((char*)header->pBuf_ + header->numInitialized_ * blockSize_)) = header->fHead_;
	header->fHead_ = (char*)(header->pBuf_) + header->numInitialized_ * blockSize_;
	++(header->numInitialized_);
	if (header->numInitialized_ == (FIXED_PAGE_SIZE - sizeof(FSAHeader)) / blockSize_) {
		header->isInitialized_ = true;
	}
}

void* MemoryAllocator::FixedSizeAllocator::alloc()
{
	assert(pFirstPage_ != nullptr);
	FSAHeader* pCurrentPage = (FSAHeader*)pFirstPage_;
	FSAHeader* pCurrentBlock = pCurrentPage;
	while (pCurrentPage != nullptr) {
		if (pCurrentPage->fHead_ != nullptr || !(pCurrentPage->isInitialized_)) {
			if (!(pCurrentPage->isInitialized_)) {
				initElement(pCurrentPage);
			}
			void* currentFreeEl = pCurrentPage->fHead_;
			pCurrentPage->fHead_ = *((void**)(pCurrentPage->fHead_));
			if (!(pCurrentPage->isInitialized_)) {
				initElement(pCurrentPage);
			}
#ifdef _DEBUG
			++numUsedBlocks_;
			--numFreeBlocks_;
			size_t currentIndex = ((char*)currentFreeEl - (char*)(pCurrentPage->pBuf_)) / blockSize_;
			pCurrentPage->usedBlockFlags_[currentIndex] = true;
#endif // _DEBUG
			return currentFreeEl;
		}
		if (pCurrentPage->pNextPage_ != nullptr) {
			if (pCurrentPage->pNextPage_ != (char*)pCurrentPage + FIXED_PAGE_SIZE) {
				pCurrentBlock = (FSAHeader*)(pCurrentPage->pNextPage_);
			}
			pCurrentPage = (FSAHeader*)(pCurrentPage->pNextPage_);
		}
		else {
			break;
		}
	}
	if ((char*)pCurrentPage < (char*)pCurrentBlock + FIXED_PAGE_SIZE * (ALLOCATOR_PAGES_IN_BLOCK - 1)) {
		pCurrentPage->pNextPage_ = VirtualAlloc((char*)pCurrentPage + FIXED_PAGE_SIZE,
			FIXED_PAGE_SIZE, MEM_COMMIT, PAGE_READWRITE);
	}
	else {
		++numPageBlocks_;
		void* pNewBlock = VirtualAlloc(NULL,
			FIXED_PAGE_SIZE * ALLOCATOR_PAGES_IN_BLOCK, MEM_RESERVE, PAGE_READWRITE);
		pCurrentPage->pNextPage_ = VirtualAlloc(pNewBlock, FIXED_PAGE_SIZE, MEM_COMMIT, PAGE_READWRITE);
	}
#ifdef _DEBUG
	usedMemory_ += (long)FIXED_PAGE_SIZE;
	numFreeBlocks_ += (FIXED_PAGE_SIZE - sizeof(FSAHeader)) / blockSize_ - 1;
	++numUsedBlocks_;
#endif // _DEBUG
	pCurrentPage = (FSAHeader*)(pCurrentPage->pNextPage_);
	pCurrentPage->pBuf_ = (char*)pCurrentPage + sizeof(FSAHeader);
	pCurrentPage->pNextPage_ = nullptr;
	pCurrentPage->fHead_ = nullptr;
	pCurrentPage->numInitialized_ = 1;
	pCurrentPage->isInitialized_ = false;
#ifdef _DEBUG
	pCurrentPage->usedBlockFlags_ = new bool[(FIXED_PAGE_SIZE - sizeof(FSAHeader)) / blockSize_];
	pCurrentPage->usedBlockFlags_[0] = true;
	for (int i = 1; i < (FIXED_PAGE_SIZE - sizeof(FSAHeader)) / blockSize_; ++i) {
		pCurrentPage->usedBlockFlags_[i] = false;
	}
#endif // _DEBUG
	return pCurrentPage->pBuf_;
}

void* MemoryAllocator::CoalesceAllocator::alloc(size_t size)
{
	assert(pFirstPage_ != nullptr);
	CoalesceHeader* pCurrentPage = (CoalesceHeader*)pFirstPage_;
	CoalesceHeader* pCurrentBlock = pCurrentPage;
	while (pCurrentPage != nullptr) {
		CoalesceFreeEl* currentFreeEl = (CoalesceFreeEl*)(pCurrentPage->fHead_);
		while (currentFreeEl != nullptr) {
			if (currentFreeEl->blockSize >= size) {
				if (currentFreeEl->blockSize - size - sizeof(CoalesceBlockData) <
					FIXED_ALLOCATION_THRESHOLD + sizeof(CoalesceBlockData)) {
					if (currentFreeEl->pPrevFree != nullptr) {
						currentFreeEl->pPrevFree->pNextFree = currentFreeEl->pNextFree;
					}
					else {
						pCurrentPage->fHead_ = currentFreeEl->pNextFree;
					}
					if (currentFreeEl->pNextFree != nullptr) {
						currentFreeEl->pNextFree->pPrevFree = currentFreeEl->pPrevFree;
					}
					((CoalesceBlockData*)((char*)currentFreeEl - sizeof(CoalesceBlockData)))->isFree = false;
#ifdef _DEBUG
					++numUsedBlocks_;
					--numFreeBlocks_;
#endif // _DEBUG
				}
				else {
					CoalesceFreeEl* newFreeEl = (CoalesceFreeEl*)((char*)currentFreeEl + size + sizeof(CoalesceBlockData));
					newFreeEl->pPrevFree = currentFreeEl->pPrevFree;
					newFreeEl->pNextFree = currentFreeEl->pNextFree;
					if (currentFreeEl->pPrevFree != nullptr) {
						currentFreeEl->pPrevFree->pNextFree = newFreeEl;
					}
					if (currentFreeEl->pNextFree != nullptr) {
						currentFreeEl->pNextFree->pPrevFree = newFreeEl;
					}
					CoalesceBlockData* currentElData = (CoalesceBlockData*)((char*)currentFreeEl - sizeof(CoalesceBlockData));
					CoalesceBlockData* newElData = (CoalesceBlockData*)((char*)newFreeEl - sizeof(CoalesceBlockData));
					if (currentElData->pNext != nullptr) {
						currentElData->pNext->pPrev = newElData;
					}
					newElData->pPrev = currentElData;
					newElData->pNext = currentElData->pNext;
					currentElData->pNext = newElData;
					newElData->isFree = true;
					newFreeEl->blockSize = currentFreeEl->blockSize - size - sizeof(CoalesceBlockData);
					currentElData->isFree = false;
					if (currentFreeEl == pCurrentPage->fHead_) {
						pCurrentPage->fHead_ = newFreeEl;
					}
#ifdef _DEBUG
					++numUsedBlocks_;
#endif // _DEBUG
				}
				return currentFreeEl;
			}
			currentFreeEl = (CoalesceFreeEl*)(currentFreeEl->pNextFree);
		}
		if (pCurrentPage->pNextPage_ != nullptr) {
			if (pCurrentPage->pNextPage_ != (char*)pCurrentPage + COALESCE_PAGE_SIZE) {
				pCurrentBlock = (CoalesceHeader*)(pCurrentPage->pNextPage_);
			}
			pCurrentPage = (CoalesceHeader*)(pCurrentPage->pNextPage_);
		}
		else {
			break;
		}
	}
	if ((char*)pCurrentPage < (char*)pCurrentBlock + COALESCE_PAGE_SIZE * (ALLOCATOR_PAGES_IN_BLOCK - 1)) {
		pCurrentPage->pNextPage_ = VirtualAlloc((char*)pCurrentPage + COALESCE_PAGE_SIZE,
			COALESCE_PAGE_SIZE, MEM_COMMIT, PAGE_READWRITE);
	}
	else {
		++numPageBlocks_;
		void* pNewBlock = VirtualAlloc(NULL,
			COALESCE_PAGE_SIZE * ALLOCATOR_PAGES_IN_BLOCK, MEM_RESERVE, PAGE_READWRITE);
		pCurrentPage->pNextPage_ = VirtualAlloc(pNewBlock, COALESCE_PAGE_SIZE, MEM_COMMIT, PAGE_READWRITE);
	}
#ifdef _DEBUG
	usedMemory_ += (long)COALESCE_PAGE_SIZE;
	++numFreeBlocks_;
	++numUsedBlocks_;
#endif // _DEBUG
	pCurrentPage = (CoalesceHeader*)(pCurrentPage->pNextPage_);
	pCurrentPage->pBuf_ = (char*)pCurrentPage + sizeof(CoalesceHeader);
	pCurrentPage->pNextPage_ = nullptr;
	pCurrentPage->fHead_ = (char*)(pCurrentPage->pBuf_) + 2 * sizeof(CoalesceBlockData) + size;
	((CoalesceBlockData*)(pCurrentPage->pBuf_))->pPrev = nullptr;
	((CoalesceBlockData*)(pCurrentPage->pBuf_))->pNext = (CoalesceBlockData*)((char*)(pCurrentPage->fHead_) - sizeof(CoalesceBlockData));
	((CoalesceBlockData*)(pCurrentPage->pBuf_))->isFree = false;
	((CoalesceBlockData*)((char*)(pCurrentPage->fHead_) - sizeof(CoalesceBlockData)))->pPrev = (CoalesceBlockData*)(pCurrentPage->pBuf_);
	((CoalesceBlockData*)((char*)(pCurrentPage->fHead_) - sizeof(CoalesceBlockData)))->pNext = nullptr;
	((CoalesceBlockData*)((char*)(pCurrentPage->fHead_) - sizeof(CoalesceBlockData)))->isFree = true;
	((CoalesceFreeEl*)(pCurrentPage->fHead_))->pPrevFree = nullptr;
	((CoalesceFreeEl*)(pCurrentPage->fHead_))->pNextFree = nullptr;
	((CoalesceFreeEl*)(pCurrentPage->fHead_))->blockSize =
		COALESCE_PAGE_SIZE - 2 * sizeof(CoalesceBlockData) - size;
	return (char*)(pCurrentPage->pBuf_) + sizeof(CoalesceBlockData);
}

void* MemoryAllocator::alloc(size_t size)
{
#ifdef _DEBUG
	assert(isInitialized_ == true);
#endif // _DEBUG
	if (size <= FIXED_ALLOCATION_THRESHOLD) {
		int index = 0;
		for (size_t i = MIN_FIXED_SIZE; i <= FIXED_ALLOCATION_THRESHOLD; i *= 2) {
			if (size <= i) {
				return fsa_[index].alloc();
			}
			++index;
		}
	}
	else if (size <= COALESCE_ALLOCATION_THRESHOLD) {
		return coalesceAllocator_.alloc(size);
	}
	else {
		return VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	}
}

bool MemoryAllocator::FixedSizeAllocator::free(void* p)
{
	assert(pFirstPage_ != nullptr);
	FSAHeader* pCurrentBlock = (FSAHeader*)pFirstPage_;
	for (int i = 1; i <= numPageBlocks_; ++i) {
		if (p >= pCurrentBlock->pBuf_ && p < (char*)(pCurrentBlock->pBuf_)
			+ FIXED_PAGE_SIZE * ALLOCATOR_PAGES_IN_BLOCK - sizeof(FSAHeader)) {
			char* pPageStart = (char*)(pCurrentBlock->pBuf_)
				+ ((char*)p - (char*)(pCurrentBlock->pBuf_)) / FIXED_PAGE_SIZE * FIXED_PAGE_SIZE;
			*((void**)p) = ((FSAHeader*)pPageStart)->fHead_;
			pCurrentBlock->fHead_ = p;
#ifdef _DEBUG
			--numUsedBlocks_;
			++numFreeBlocks_;
			size_t currentIndex = ((char*)p - (char*)pPageStart) / blockSize_;
			((FSAHeader*)((char*)pPageStart - sizeof(FSAHeader)))->usedBlockFlags_[currentIndex] = false;
#endif // _DEBUG
			return true;
		}
		if (i < numPageBlocks_) {
			pCurrentBlock = (FSAHeader*)((char*)pCurrentBlock
				+ FIXED_PAGE_SIZE * (ALLOCATOR_PAGES_IN_BLOCK - 1));
			pCurrentBlock = (FSAHeader*)(pCurrentBlock->pNextPage_);
		}
		else {
			return false;
		}
	}
	return false;
}

bool MemoryAllocator::CoalesceAllocator::free(void* p)
{
	assert(pFirstPage_ != nullptr);
	CoalesceHeader* pCurrentBlock = (CoalesceHeader*)pFirstPage_;
	for (int i = 1; i <= numPageBlocks_; ++i) {
		if (p >= pCurrentBlock->pBuf_ && p < (char*)(pCurrentBlock->pBuf_)
			+ COALESCE_PAGE_SIZE * ALLOCATOR_PAGES_IN_BLOCK - sizeof(CoalesceHeader)) {
#ifdef _DEBUG
			--numUsedBlocks_;
#endif // _DEBUG
			CoalesceBlockData* currentElData = (CoalesceBlockData*)((char*)p - sizeof(CoalesceBlockData));
			if (currentElData->pPrev != nullptr && currentElData->pPrev->isFree) {
				((CoalesceFreeEl*)((char*)(currentElData->pPrev) + sizeof(CoalesceBlockData)))->blockSize +=
					(char*)(currentElData->pNext) - (char*)currentElData;
				currentElData->pPrev->pNext = currentElData->pNext;
				if (currentElData->pNext != nullptr) {
					currentElData->pNext->pPrev = currentElData->pPrev;
				}
				currentElData = currentElData->pPrev;
				p = (char*)currentElData + sizeof(CoalesceBlockData);
			}
			else {
				currentElData->isFree = true;
				CoalesceHeader* pCurrentPage = pCurrentBlock;
				while (pCurrentPage->pNextPage_ != nullptr && pCurrentPage->pNextPage_ < currentElData) {
					pCurrentPage = (CoalesceHeader*)(pCurrentPage->pNextPage_);
				}
				((CoalesceFreeEl*)p)->pPrevFree = nullptr;
				((CoalesceFreeEl*)p)->pNextFree = (CoalesceFreeEl*)(pCurrentPage->fHead_);
				pCurrentPage->fHead_ = p;
				((CoalesceFreeEl*)p)->pNextFree->pPrevFree = (CoalesceFreeEl*)p;
				((CoalesceFreeEl*)p)->blockSize = (char*)(currentElData->pNext) - (char*)p;
#ifdef _DEBUG
				++numFreeBlocks_;
#endif // _DEBUG
			}
			if (currentElData->pNext != nullptr && currentElData->pNext->isFree) {
				CoalesceFreeEl* nextFreeEl = (CoalesceFreeEl*)((char*)(currentElData->pNext) + sizeof(CoalesceBlockData));
				((CoalesceFreeEl*)p)->blockSize += nextFreeEl->blockSize + sizeof(CoalesceBlockData);
				if (nextFreeEl->pPrevFree != nullptr) {
					nextFreeEl->pPrevFree->pNextFree = nextFreeEl->pNextFree;
				}				
				if (nextFreeEl->pNextFree != nullptr) {
					nextFreeEl->pNextFree->pPrevFree = nextFreeEl->pPrevFree;
				}
				if (currentElData->pNext->pNext != nullptr) {
					currentElData->pNext->pNext->pPrev = currentElData;
				}
				currentElData->pNext = currentElData->pNext->pNext;
#ifdef _DEBUG
				--numFreeBlocks_;
#endif // _DEBUG
			}
			return true;			
		}
		if (i < numPageBlocks_) {
			pCurrentBlock = (CoalesceHeader*)((char*)pCurrentBlock
				+ COALESCE_PAGE_SIZE * (ALLOCATOR_PAGES_IN_BLOCK - 1));
			pCurrentBlock = (CoalesceHeader*)(pCurrentBlock->pNextPage_);
		}
		else {
			return false;
		}
	}
	return false;
}

void MemoryAllocator::free(void* p)
{
#ifdef _DEBUG
	assert(isInitialized_ == true);
#endif // _DEBUG
	for (int i = 0; i < N_FIXED_SIZE_ALLOCATORS; ++i) {
		if (fsa_[i].free(p)) {
			return;
		}
	}
	if (coalesceAllocator_.free(p)) {
		return;
	}
	VirtualFree(p, 0, MEM_RELEASE);
}

void MemoryAllocator::FixedSizeAllocator::destroy()
{
	assert(pFirstPage_ != nullptr);
	FSAHeader* pCurrentBlock = (FSAHeader*)pFirstPage_;
	for (int i = 1; i < numPageBlocks_; ++i) {
		FSAHeader* pTempBlock = pCurrentBlock;
#ifdef _DEBUG
		FSAHeader* pCurrentPage = pCurrentBlock;
		while (pCurrentPage != nullptr) {
			delete[](pCurrentPage->usedBlockFlags_);
			pCurrentPage = (FSAHeader*)(pCurrentPage->pNextPage_);
		}
#endif // _DEBUG
		pCurrentBlock = (FSAHeader*)((char*)pCurrentBlock
			+ FIXED_PAGE_SIZE * (ALLOCATOR_PAGES_IN_BLOCK - 1));
		pCurrentBlock = (FSAHeader*)(pCurrentBlock->pNextPage_);
		VirtualFree(pTempBlock, 0, MEM_RELEASE);
	}
	VirtualFree(pCurrentBlock, 0, MEM_RELEASE);
	pFirstPage_ = nullptr;
}

void MemoryAllocator::CoalesceAllocator::destroy()
{
	assert(pFirstPage_ != nullptr);
	CoalesceHeader* pCurrentBlock = (CoalesceHeader*)pFirstPage_;
	for (int i = 1; i < numPageBlocks_; ++i) {
		CoalesceHeader* pTempBlock = pCurrentBlock;
		pCurrentBlock = pCurrentBlock = (CoalesceHeader*)((char*)pCurrentBlock
			+ COALESCE_PAGE_SIZE * (ALLOCATOR_PAGES_IN_BLOCK - 1));
		pCurrentBlock = (CoalesceHeader*)(pCurrentBlock->pNextPage_);
		VirtualFree(pTempBlock, 0, MEM_RELEASE);
	}
	VirtualFree(pCurrentBlock, 0, MEM_RELEASE);
	pFirstPage_ = nullptr;
}

void MemoryAllocator::destroy()
{
#ifdef _DEBUG
	assert(isInitialized_ == true);
#endif // _DEBUG
	for (int i = 0; i < N_FIXED_SIZE_ALLOCATORS; ++i) {
		fsa_[i].destroy();
	}
	coalesceAllocator_.destroy();
#ifdef _DEBUG
	isInitialized_ = false;
#endif // _DEBUG
}

MemoryAllocator::~MemoryAllocator()
{
#ifdef _DEBUG
	assert(isInitialized_ == false);
#endif // _DEBUG
}

#ifdef _DEBUG
void MemoryAllocator::FixedSizeAllocator::dumpStat() const
{
	std::cout << "Fixed Size Allocator, block size " << blockSize_ << "\n";
	std::cout << numFreeBlocks_ << " free blocks\n";
	std::cout << numUsedBlocks_ << " used blocks\n";
	std::cout << usedMemory_ << " bytes of memory allocated, in " << numPageBlocks_ << " block(s)\n\n";
}

void MemoryAllocator::CoalesceAllocator::dumpStat() const
{
	std::cout << "Coalesce Allocator\n";
	std::cout << numFreeBlocks_ << " free blocks\n";
	std::cout << numUsedBlocks_ << " used blocks\n";
	std::cout << usedMemory_ << " bytes of memory allocated, in " << numPageBlocks_ << " block(s)\n\n";
}

void MemoryAllocator::dumpStat() const
{
	for (int i = 0; i < N_FIXED_SIZE_ALLOCATORS; ++i) {
		fsa_[i].dumpStat();
	}
	coalesceAllocator_.dumpStat();
}

void MemoryAllocator::FixedSizeAllocator::dumpBlocks() const
{
	std::cout << "Fixed Size Allocator, block size " << blockSize_ << "\nUsed blocks:\n";
	assert(pFirstPage_ != nullptr);
	FSAHeader* pCurrentPage = (FSAHeader*)pFirstPage_;
	while (pCurrentPage != nullptr) {
		for (int i = 0; i < (FIXED_PAGE_SIZE - sizeof(FSAHeader)) / blockSize_; ++i) {
			if (pCurrentPage->usedBlockFlags_[i]) {
				char* currentAddress = (char*)(pCurrentPage->pBuf_) + i * blockSize_;
				std::cout << "Block at " << (void*)currentAddress << "\n";
			}
		}
		pCurrentPage = (FSAHeader*)(pCurrentPage->pNextPage_);
	}
}

void MemoryAllocator::CoalesceAllocator::dumpBlocks() const
{
	std::cout << "Coalesce Allocator\nUsed blocks:\n";
	assert(pFirstPage_ != nullptr);
	CoalesceHeader* pCurrentPage = (CoalesceHeader*)pFirstPage_;
	while (pCurrentPage != nullptr) {
		CoalesceBlockData* currentEl = (CoalesceBlockData*)(pCurrentPage->pBuf_);
		while (currentEl != nullptr) {
			if (!(currentEl->isFree)) {
				size_t currentSize;
				if (currentEl->pNext != nullptr) {
					currentSize = (char*)currentEl->pNext - ((char*)currentEl + sizeof(CoalesceBlockData));
				}
				else {
					currentSize = (char*)pCurrentPage + COALESCE_PAGE_SIZE -
						((char*)currentEl + sizeof(CoalesceBlockData));
				}
				std::cout << "Block at " << currentEl << ", size " << currentSize << "\n";
			}
			currentEl = currentEl->pNext;
		}
		pCurrentPage = (CoalesceHeader*)(pCurrentPage->pNextPage_);
	}
}

void MemoryAllocator::dumpBlocks() const
{
	for (int i = 0; i < N_FIXED_SIZE_ALLOCATORS; ++i) {
		fsa_[i].dumpBlocks();
	}
	coalesceAllocator_.dumpBlocks();
}
#endif
