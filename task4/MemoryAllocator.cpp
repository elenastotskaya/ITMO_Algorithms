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
	//инициализация всех FSA соответствующим размером блока
	int index = 0;
	for (size_t i = MIN_FIXED_SIZE; i <= FIXED_ALLOCATION_THRESHOLD; i *= 2) {
		fsa_[index] = FixedSizeAllocator(i);
		++index;
	}
}

void MemoryAllocator::FixedSizeAllocator::init(void* start)
{
#ifdef _DEBUG
	//сбор статистики - добавление новой страницы
	usedMemory_ += (long)(FILLER_RATIO * FIXED_PAGE_SIZE);
	numFreeBlocks_ += (FILLER_RATIO * FIXED_PAGE_SIZE - sizeof(FSAHeader)) / blockSize_ / FILLER_RATIO;
	//выделение памяти - для отладки в 3 раза больше
	pFirstPage_ = VirtualAlloc(start, FILLER_RATIO * FIXED_PAGE_SIZE, MEM_COMMIT, PAGE_READWRITE);
	//указатель на данные - дополнительное смещение на blockSize_ для контрольного блока
	((FSAHeader*)pFirstPage_)->pBuf_ = (char*)pFirstPage_ + sizeof(FSAHeader) + blockSize_;
#else
	//стандартное выдление страницы и инициализация данных
	pFirstPage_ = VirtualAlloc(start, FIXED_PAGE_SIZE, MEM_COMMIT, PAGE_READWRITE);
	((FSAHeader*)pFirstPage_)->pBuf_ = (char*)pFirstPage_ + sizeof(FSAHeader);
#endif // _DEBUG
	((FSAHeader*)pFirstPage_)->pNextPage_ = nullptr;
	((FSAHeader*)pFirstPage_)->fHead_ = ((FSAHeader*)pFirstPage_)->pBuf_;
	//так как fHead не пустой, 1 элемент инициализирован
	((FSAHeader*)pFirstPage_)->numInitialized_ = 1;
	((FSAHeader*)pFirstPage_)->isInitialized_ = false;
#ifdef _DEBUG
	//заполнение контрольных блоков
	for (int i = 0; i < blockSize_ / sizeof(__int64); ++i) {
		*(__int64*)((char*)(((FSAHeader*)pFirstPage_)->fHead_) - blockSize_ + i * sizeof(__int64)) = fillerBlock_;
		*(__int64*)((char*)(((FSAHeader*)pFirstPage_)->fHead_) + blockSize_ + i * sizeof(__int64)) = fillerBlock_;
	}
	//инициализация массива флагов и установка первого элемента
	((FSAHeader*)pFirstPage_)->usedBlockFlags_ = new bool[(FILLER_RATIO * FIXED_PAGE_SIZE - sizeof(FSAHeader)) / blockSize_ / FILLER_RATIO];
	for (int i = 0; i < (FILLER_RATIO * FIXED_PAGE_SIZE - sizeof(FSAHeader)) / blockSize_ / FILLER_RATIO; ++i) {
		((FSAHeader*)pFirstPage_)->usedBlockFlags_[i] = false;
	}
#endif // _DEBUG
}

void MemoryAllocator::CoalesceAllocator::init(void* start)
{
#ifdef _DEBUG
	//сбор статистики - добавление страницы с одним блоком
	usedMemory_ += (long)COALESCE_PAGE_SIZE;
	numFreeBlocks_ = 1;
#endif // _DEBUG
	//выделение первой страницы
	pFirstPage_ = VirtualAlloc(start, COALESCE_PAGE_SIZE, MEM_COMMIT, PAGE_READWRITE);
	//pBuf_ ставится на заголовок первого блока
	((CoalesceHeader*)pFirstPage_)->pBuf_ = (char*)pFirstPage_ + sizeof(CoalesceHeader);
	((CoalesceHeader*)pFirstPage_)->pNextPage_ = nullptr;
	((CoalesceBlockData*)((CoalesceHeader*)pFirstPage_)->pBuf_)->pPrev = nullptr;
	((CoalesceBlockData*)((CoalesceHeader*)pFirstPage_)->pBuf_)->pNext = nullptr;
	((CoalesceBlockData*)((CoalesceHeader*)pFirstPage_)->pBuf_)->isFree = true;
#ifdef _DEBUG
	//отличие отладочной версии - контрольный блок между заголовком и данными
	((CoalesceHeader*)pFirstPage_)->fHead_ = (char*)((CoalesceHeader*)pFirstPage_)->pBuf_ + sizeof(CoalesceBlockData) + COALESCE_FILLER_SIZE;
	//заполнение контрольных блоков
	for (int i = 0; i < COALESCE_FILLER_SIZE / sizeof(__int64); ++i) {
		*(__int64*)((char*)(((CoalesceHeader*)pFirstPage_)->fHead_) - COALESCE_FILLER_SIZE + i * sizeof(__int64)) = fillerBlock_;
	}
	((CoalesceFreeEl*)((CoalesceHeader*)pFirstPage_)->fHead_)->pPrevFree = nullptr;
	((CoalesceFreeEl*)((CoalesceHeader*)pFirstPage_)->fHead_)->pNextFree = nullptr;
	//размер блока - вся страница без служебных данных
	((CoalesceFreeEl*)((CoalesceHeader*)pFirstPage_)->fHead_)->blockSize =
		COALESCE_PAGE_SIZE - sizeof(CoalesceBlockData) - sizeof(CoalesceHeader) - COALESCE_FILLER_SIZE;
#else
	((CoalesceHeader*)pFirstPage_)->fHead_ = (char*)((CoalesceHeader*)pFirstPage_)->pBuf_ + sizeof(CoalesceBlockData);
	((CoalesceFreeEl*)((CoalesceHeader*)pFirstPage_)->fHead_)->pPrevFree = nullptr;
	((CoalesceFreeEl*)((CoalesceHeader*)pFirstPage_)->fHead_)->pNextFree = nullptr;
	((CoalesceFreeEl*)((CoalesceHeader*)pFirstPage_)->fHead_)->blockSize =
		COALESCE_PAGE_SIZE - sizeof(CoalesceBlockData) - sizeof(CoalesceHeader);
#endif // _DEBUG	
}

void MemoryAllocator::init()
{
#ifdef _DEBUG
	//проверка, что не инициализируем второй раз
	assert(isInitialized_ == false);
	//резервирование памяти для всех аллокаторов
	//для FSA в режиме отладки блок в 3 раза больше, для Coalesce одинаковый
	for (int i = 0; i < N_FIXED_SIZE_ALLOCATORS; ++i) {
		fsa_[i].init(VirtualAlloc(NULL, FILLER_RATIO * FIXED_PAGE_SIZE * ALLOCATOR_PAGES_IN_BLOCK, MEM_RESERVE, PAGE_READWRITE));
	}
	coalesceAllocator_.init(VirtualAlloc(NULL, COALESCE_PAGE_SIZE * ALLOCATOR_PAGES_IN_BLOCK, MEM_RESERVE, PAGE_READWRITE));
	isInitialized_ = true;
#else
	for (int i = 0; i < N_FIXED_SIZE_ALLOCATORS; ++i) {
		fsa_[i].init(VirtualAlloc(NULL, FIXED_PAGE_SIZE * ALLOCATOR_PAGES_IN_BLOCK, MEM_RESERVE, PAGE_READWRITE));
	}
	coalesceAllocator_.init(VirtualAlloc(NULL, COALESCE_PAGE_SIZE * ALLOCATOR_PAGES_IN_BLOCK, MEM_RESERVE, PAGE_READWRITE));
#endif // _DEBUG
}

void MemoryAllocator::FixedSizeAllocator::initElement(FSAHeader* header)
{
	assert(header != nullptr);
#ifdef _DEBUG
	//находим первый неинициализированный блок
	*((void**)((char*)header->pBuf_ + header->numInitialized_ * blockSize_ * FILLER_RATIO)) = header->fHead_;
	//добавляем в голову списка
	header->fHead_ = (char*)(header->pBuf_) + header->numInitialized_ * blockSize_ * FILLER_RATIO;
	//контрольные блоки до и после нового элемента
	for (int i = 0; i < blockSize_ / sizeof(__int64); ++i) {
		*(__int64*)((char*)(header->fHead_) - blockSize_ + i * sizeof(__int64)) = fillerBlock_;
		*(__int64*)((char*)(header->fHead_) + blockSize_ + i * sizeof(__int64)) = fillerBlock_;
	}
	++(header->numInitialized_);
	if (header->numInitialized_ == (FILLER_RATIO * FIXED_PAGE_SIZE - sizeof(FSAHeader)) / blockSize_ / FILLER_RATIO) {
		header->isInitialized_ = true;
	}
#else
	*((void**)((char*)header->pBuf_ + header->numInitialized_ * blockSize_)) = header->fHead_;
	header->fHead_ = (char*)(header->pBuf_) + header->numInitialized_ * blockSize_;
	++(header->numInitialized_);
	if (header->numInitialized_ == (FIXED_PAGE_SIZE - sizeof(FSAHeader)) / blockSize_) {
		header->isInitialized_ = true;
}
#endif // _DEBUG
}

void* MemoryAllocator::FixedSizeAllocator::alloc()
{
	assert(pFirstPage_ != nullptr);
	FSAHeader* pCurrentPage = (FSAHeader*)pFirstPage_;
	FSAHeader* pCurrentBlock = pCurrentPage; // этот указатель нужен, если блок страниц не один
	//поиск незаполненной страницы
	while (pCurrentPage != nullptr) {
		if (pCurrentPage->fHead_ != nullptr || !(pCurrentPage->isInitialized_)) {
			if (!(pCurrentPage->isInitialized_)) {
				//инициализация элемента (чтобы список не был пустым)
				initElement(pCurrentPage);
			}
			//сохранение и смещение головы списка
			void* currentFreeEl = pCurrentPage->fHead_;
			pCurrentPage->fHead_ = *((void**)(pCurrentPage->fHead_));
#ifdef _DEBUG
			++numUsedBlocks_;
			--numFreeBlocks_;
			//обозначение блока как занятого в массиве флагов
			size_t currentIndex = ((char*)currentFreeEl - (char*)(pCurrentPage->pBuf_)) / blockSize_ / FILLER_RATIO;
			pCurrentPage->usedBlockFlags_[currentIndex] = true;
#endif // _DEBUG
			return currentFreeEl;
		}
		if (pCurrentPage->pNextPage_ != nullptr) {
#ifdef _DEBUG
			//проверка, не перейдет ли на следующий блок страниц
			if (pCurrentPage->pNextPage_ != (char*)pCurrentPage + FILLER_RATIO * FIXED_PAGE_SIZE) {
#else
			if (pCurrentPage->pNextPage_ != (char*)pCurrentPage + FIXED_PAGE_SIZE) {
#endif // _DEBUG
				//переключение текущего блока страниц
				pCurrentBlock = (FSAHeader*)(pCurrentPage->pNextPage_);
			}
			pCurrentPage = (FSAHeader*)(pCurrentPage->pNextPage_);
		}
		else {
			break;
		}
	}
#ifdef _DEBUG
	//выделение новой страницы:
	//если есть зарезервированная память
	if ((char*)pCurrentPage < (char*)pCurrentBlock + FILLER_RATIO * FIXED_PAGE_SIZE * (ALLOCATOR_PAGES_IN_BLOCK - 1)) {
		pCurrentPage->pNextPage_ = VirtualAlloc((char*)pCurrentPage + FILLER_RATIO * FIXED_PAGE_SIZE,
			FILLER_RATIO * FIXED_PAGE_SIZE, MEM_COMMIT, PAGE_READWRITE);
	}
	//если зарезервированная память закончилась
	else {
		++numPageBlocks_;
		void* pNewBlock = VirtualAlloc(NULL,
			FILLER_RATIO * FIXED_PAGE_SIZE * ALLOCATOR_PAGES_IN_BLOCK, MEM_RESERVE, PAGE_READWRITE);
		pCurrentPage->pNextPage_ = VirtualAlloc(pNewBlock, FILLER_RATIO * FIXED_PAGE_SIZE, MEM_COMMIT, PAGE_READWRITE);
	}
	usedMemory_ += (long)FILLER_RATIO * FIXED_PAGE_SIZE;
	numFreeBlocks_ += (FILLER_RATIO * FIXED_PAGE_SIZE - sizeof(FSAHeader)) / blockSize_ / FILLER_RATIO - 1;
	++numUsedBlocks_;
	//инициализация страницы сразу с занятым первым блоком
	pCurrentPage = (FSAHeader*)(pCurrentPage->pNextPage_);
	pCurrentPage->pBuf_ = (char*)pCurrentPage + sizeof(FSAHeader) + blockSize_;
	//контрольные блоки
	for (int i = 0; i < blockSize_ / sizeof(__int64); ++i) {
		*(__int64*)((char*)(pCurrentPage->pBuf_) - blockSize_ + i * sizeof(__int64)) = fillerBlock_;
		*(__int64*)((char*)(pCurrentPage->pBuf_) + blockSize_ + i * sizeof(__int64)) = fillerBlock_;
	}
#else
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
	pCurrentPage = (FSAHeader*)(pCurrentPage->pNextPage_);
	pCurrentPage->pBuf_ = (char*)pCurrentPage + sizeof(FSAHeader);
#endif // _DEBUG
	//инициализирован только занятый блок
	pCurrentPage->pNextPage_ = nullptr;
	pCurrentPage->fHead_ = nullptr;
	pCurrentPage->numInitialized_ = 1;
	pCurrentPage->isInitialized_ = false;
#ifdef _DEBUG
	//выделение и инициализация массива флагов для новой страницы
	pCurrentPage->usedBlockFlags_ = new bool[(FILLER_RATIO * FIXED_PAGE_SIZE - sizeof(FSAHeader)) / blockSize_ / FILLER_RATIO];
	pCurrentPage->usedBlockFlags_[0] = true;
	for (int i = 1; i < (FILLER_RATIO * FIXED_PAGE_SIZE - sizeof(FSAHeader)) / blockSize_ / FILLER_RATIO; ++i) {
		pCurrentPage->usedBlockFlags_[i] = false;
	}
#endif // _DEBUG
	return pCurrentPage->pBuf_;
}

void* MemoryAllocator::CoalesceAllocator::alloc(size_t size)
{
	assert(pFirstPage_ != nullptr);
	CoalesceHeader* pCurrentPage = (CoalesceHeader*)pFirstPage_;
	CoalesceHeader* pCurrentBlock = pCurrentPage; //аналогично FSA
	while (pCurrentPage != nullptr) {
		CoalesceFreeEl* currentFreeEl = (CoalesceFreeEl*)(pCurrentPage->fHead_);
		while (currentFreeEl != nullptr) {
			//поиск подходящего по размеру блока
			if (currentFreeEl->blockSize >= size) {
#ifdef _DEBUG
				//помещается ли в блок кроме текущго элемента еще один (хотя бы минимального размера)
				if (currentFreeEl->blockSize - size - sizeof(CoalesceBlockData) - COALESCE_FILLER_SIZE <
					FIXED_ALLOCATION_THRESHOLD + sizeof(CoalesceBlockData) + 2 * COALESCE_FILLER_SIZE) {
#else
				if (currentFreeEl->blockSize - size - sizeof(CoalesceBlockData) <
					FIXED_ALLOCATION_THRESHOLD + sizeof(CoalesceBlockData)) {
#endif // _DEBUG
					//не помещается - просто исключаем блок из списка пустых
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
#ifdef _DEBUG
					//новый блок - оставшаяся свободная часть текущего
					CoalesceFreeEl* newFreeEl = (CoalesceFreeEl*)((char*)currentFreeEl
						+ size + sizeof(CoalesceBlockData) + 2 * COALESCE_FILLER_SIZE);
#else
					CoalesceFreeEl* newFreeEl = (CoalesceFreeEl*)((char*)currentFreeEl + size + sizeof(CoalesceBlockData));
#endif // _DEBUG
					//добавление нового блока в список свободных вместо текущего
					newFreeEl->pPrevFree = currentFreeEl->pPrevFree;
					newFreeEl->pNextFree = currentFreeEl->pNextFree;
					if (currentFreeEl->pPrevFree != nullptr) {
						currentFreeEl->pPrevFree->pNextFree = newFreeEl;
					}
					if (currentFreeEl->pNextFree != nullptr) {
						currentFreeEl->pNextFree->pPrevFree = newFreeEl;
					}
#ifdef _DEBUG
					//заполнение заголовка нового блока
					CoalesceBlockData* currentElData = (CoalesceBlockData*)((char*)currentFreeEl - sizeof(CoalesceBlockData) - COALESCE_FILLER_SIZE);
					CoalesceBlockData* newElData = (CoalesceBlockData*)((char*)newFreeEl - sizeof(CoalesceBlockData) - COALESCE_FILLER_SIZE);
					//контрольные блоки перед новым блоком
					for (int i = 0; i < COALESCE_FILLER_SIZE / sizeof(__int64); ++i) {
						*(__int64*)((char*)newElData + sizeof(CoalesceBlockData) + i * sizeof(__int64)) = fillerBlock_;
					}
					newFreeEl->blockSize = currentFreeEl->blockSize - size - sizeof(CoalesceBlockData) - 2 * COALESCE_FILLER_SIZE;
#else
					CoalesceBlockData* currentElData = (CoalesceBlockData*)((char*)currentFreeEl - sizeof(CoalesceBlockData));
					CoalesceBlockData* newElData = (CoalesceBlockData*)((char*)newFreeEl - sizeof(CoalesceBlockData));
					newFreeEl->blockSize = currentFreeEl->blockSize - size - sizeof(CoalesceBlockData);
#endif // _DEBUG
					//добавление нового блока в список всех блоков после текущего
					if (currentElData->pNext != nullptr) {
						currentElData->pNext->pPrev = newElData;
					}
					newElData->pPrev = currentElData;
					newElData->pNext = currentElData->pNext;
					currentElData->pNext = newElData;
					newElData->isFree = true;
					currentElData->isFree = false;
					if (currentFreeEl == pCurrentPage->fHead_) {
						pCurrentPage->fHead_ = newFreeEl;
					}
				}
#ifdef _DEBUG
				//контрольные блоки после текущего блока
				for (int i = 0; i < COALESCE_FILLER_SIZE / sizeof(__int64); ++i) {
					*(__int64*)((char*)currentFreeEl + size + i * sizeof(__int64)) = fillerBlock_;
				}
				++numUsedBlocks_;
#endif // _DEBUG
				return currentFreeEl;
			}
			currentFreeEl = (CoalesceFreeEl*)(currentFreeEl->pNextFree);
		}
		//переключение страниц и блоков - аналогично FSA
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
	//выделение страницы в текущем блоке
	if ((char*)pCurrentPage < (char*)pCurrentBlock + COALESCE_PAGE_SIZE * (ALLOCATOR_PAGES_IN_BLOCK - 1)) {
		pCurrentPage->pNextPage_ = VirtualAlloc((char*)pCurrentPage + COALESCE_PAGE_SIZE,
			COALESCE_PAGE_SIZE, MEM_COMMIT, PAGE_READWRITE);
	}
	//выделение страницы в новом блоке
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

	//инициализация новой страницы
	pCurrentPage = (CoalesceHeader*)(pCurrentPage->pNextPage_);
	pCurrentPage->pBuf_ = (char*)pCurrentPage + sizeof(CoalesceHeader);
	pCurrentPage->pNextPage_ = nullptr;
	//fHead_ указывает на второй блок, поэтому при отладке отступ на 3 контрольных блока
	pCurrentPage->fHead_ = (char*)(pCurrentPage->pBuf_) + 2 * sizeof(CoalesceBlockData) + size + 3 * COALESCE_FILLER_SIZE;
	//заоголовки блоков
	((CoalesceBlockData*)(pCurrentPage->pBuf_))->pPrev = nullptr;
	((CoalesceBlockData*)(pCurrentPage->pBuf_))->pNext = (CoalesceBlockData*)((char*)(pCurrentPage->fHead_)
		- sizeof(CoalesceBlockData) - COALESCE_FILLER_SIZE);
	((CoalesceBlockData*)(pCurrentPage->pBuf_))->isFree = false;
	((CoalesceBlockData*)((char*)(pCurrentPage->fHead_) - sizeof(CoalesceBlockData) - COALESCE_FILLER_SIZE))->pPrev = (CoalesceBlockData*)(pCurrentPage->pBuf_);
	((CoalesceBlockData*)((char*)(pCurrentPage->fHead_) - sizeof(CoalesceBlockData) - COALESCE_FILLER_SIZE))->pNext = nullptr;
	((CoalesceBlockData*)((char*)(pCurrentPage->fHead_) - sizeof(CoalesceBlockData) - COALESCE_FILLER_SIZE))->isFree = true;
	((CoalesceFreeEl*)(pCurrentPage->fHead_))->pPrevFree = nullptr;
	((CoalesceFreeEl*)(pCurrentPage->fHead_))->pNextFree = nullptr;
	((CoalesceFreeEl*)(pCurrentPage->fHead_))->blockSize =
		COALESCE_PAGE_SIZE - 2 * sizeof(CoalesceBlockData) - size - 3 * COALESCE_FILLER_SIZE;
	//заполнение контрольных блоков
	for (int i = 0; i < COALESCE_FILLER_SIZE / sizeof(__int64); ++i) {
		*(__int64*)((char*)(pCurrentPage->pBuf_) + sizeof(CoalesceBlockData) + i * sizeof(__int64)) = fillerBlock_;
		*(__int64*)((char*)(pCurrentPage->pBuf_) + sizeof(CoalesceBlockData)
			+ COALESCE_FILLER_SIZE + size + i * sizeof(__int64)) = fillerBlock_;
		*(__int64*)((char*)(pCurrentPage->fHead_) - COALESCE_FILLER_SIZE + i * sizeof(__int64)) = fillerBlock_;
	}
	return (char*)(pCurrentPage->pBuf_) + sizeof(CoalesceBlockData) + COALESCE_FILLER_SIZE;
#else
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
#endif // _DEBUG
}

void* MemoryAllocator::alloc(size_t size)
{
#ifdef _DEBUG
	assert(isInitialized_ == true);
#endif // _DEBUG
	//выбор аллокатора в зависимости от размера
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
	//проверка сразу по блокам страниц
	for (int i = 1; i <= numPageBlocks_; ++i) {
#ifdef _DEBUG
		//попадает ли адрес в текущий блок
		if (p >= pCurrentBlock->pBuf_ && p < (char*)(pCurrentBlock->pBuf_)
			+ FILLER_RATIO * FIXED_PAGE_SIZE * ALLOCATOR_PAGES_IN_BLOCK - sizeof(FSAHeader)) {

			//проверка, не повредились ли соседние блоки
			for (int i = 0; i < blockSize_ / sizeof(__int64); ++i) {
				assert(*(__int64*)((char*)p - blockSize_ + i * sizeof(__int64)) == fillerBlock_);
				assert(*(__int64*)((char*)p + blockSize_ + i * sizeof(__int64)) == fillerBlock_);
			}

			//поиск начала страницы, чтобы обновить fHead_
			char* pPageStart = (char*)pCurrentBlock
				+ ((char*)p - (char*)pCurrentBlock) / (FILLER_RATIO * FIXED_PAGE_SIZE) * (FILLER_RATIO * FIXED_PAGE_SIZE);
			
			--numUsedBlocks_;
			++numFreeBlocks_;
			//снятие флага в массиве флагов
			size_t currentIndex = ((char*)p - (char*)(((FSAHeader*)pPageStart)->pBuf_)) / blockSize_ / FILLER_RATIO;
			((FSAHeader*)((char*)pPageStart))->usedBlockFlags_[currentIndex] = false;
#else
		if (p >= pCurrentBlock->pBuf_ && p < (char*)(pCurrentBlock->pBuf_)
			+ FIXED_PAGE_SIZE * ALLOCATOR_PAGES_IN_BLOCK - sizeof(FSAHeader)) {
			char* pPageStart = (char*)pCurrentBlock
				+ ((char*)p - (char*)pCurrentBlock) / FIXED_PAGE_SIZE * FIXED_PAGE_SIZE;
#endif // _DEBUG
			*((void**)p) = ((FSAHeader*)pPageStart)->fHead_;
			pCurrentBlock->fHead_ = p;
			return true;
		}
		if (i < numPageBlocks_) {
#ifdef _DEBUG
			//переход на следующий блок
			pCurrentBlock = (FSAHeader*)((char*)pCurrentBlock
				+ FILLER_RATIO * FIXED_PAGE_SIZE * (ALLOCATOR_PAGES_IN_BLOCK - 1));
#else
			pCurrentBlock = (FSAHeader*)((char*)pCurrentBlock
				+ FIXED_PAGE_SIZE * (ALLOCATOR_PAGES_IN_BLOCK - 1));
#endif // _DEBUG
			pCurrentBlock = (FSAHeader*)(pCurrentBlock->pNextPage_);
		}
		else {
			return false;
		}
	}
	return false; //сюда не должно попадать
}

bool MemoryAllocator::CoalesceAllocator::free(void* p)
{
	assert(pFirstPage_ != nullptr);
	CoalesceHeader* pCurrentBlock = (CoalesceHeader*)pFirstPage_;
	for (int i = 1; i <= numPageBlocks_; ++i) {
		//проверка по блокам аналогично FSA
		if (p >= pCurrentBlock->pBuf_ && p < (char*)(pCurrentBlock->pBuf_)
			+ COALESCE_PAGE_SIZE * ALLOCATOR_PAGES_IN_BLOCK - sizeof(CoalesceHeader)) {
#ifdef _DEBUG
			//проверка, не повредились ли соседние блоки
			for (int i = 0; i < COALESCE_FILLER_SIZE / sizeof(__int64); ++i) {
				assert(*(__int64*)((char*)p - COALESCE_FILLER_SIZE + i * sizeof(__int64)) == fillerBlock_);
				assert(*(__int64*)((char*)(((CoalesceBlockData*)((char*)p
					- sizeof(CoalesceBlockData) - COALESCE_FILLER_SIZE))->pNext) + sizeof(CoalesceBlockData)
					+ i * sizeof(__int64)) == fillerBlock_);
			}

			--numUsedBlocks_;
			CoalesceBlockData* currentElData = (CoalesceBlockData*)((char*)p - sizeof(CoalesceBlockData) - COALESCE_FILLER_SIZE);
			//можно ли объединить с предыдущим блоком
			if (currentElData->pPrev != nullptr && currentElData->pPrev->isFree) {
				//обновление размера предыдущего блока
				((CoalesceFreeEl*)((char*)(currentElData->pPrev) + sizeof(CoalesceBlockData) + COALESCE_FILLER_SIZE))->blockSize +=
					(char*)(currentElData->pNext) - (char*)currentElData;
#else
			CoalesceBlockData* currentElData = (CoalesceBlockData*)((char*)p - sizeof(CoalesceBlockData));
			if (currentElData->pPrev != nullptr && currentElData->pPrev->isFree) {
				((CoalesceFreeEl*)((char*)(currentElData->pPrev) + sizeof(CoalesceBlockData)))->blockSize +=
					(char*)(currentElData->pNext) - (char*)currentElData;
#endif // _DEBUG
				//исключение текущего блока из списка блоков
				currentElData->pPrev->pNext = currentElData->pNext;
				if (currentElData->pNext != nullptr) {
					currentElData->pNext->pPrev = currentElData->pPrev;
				}
				currentElData = currentElData->pPrev;
#ifdef _DEBUG
				//перемещение освобожденного указателя (чтобы можно было объединить со следующим)
				p = (char*)currentElData + sizeof(CoalesceBlockData) + COALESCE_FILLER_SIZE;
#else
				p = (char*)currentElData + sizeof(CoalesceBlockData);
#endif // _DEBUG
			}
			else {
				//если объединение невозможно, просто освобождаем блок и добавляем в список свободных блоков
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
			}
			if (currentElData->pNext != nullptr && currentElData->pNext->isFree) {
				CoalesceFreeEl* nextFreeEl = (CoalesceFreeEl*)((char*)(currentElData->pNext)
					+ sizeof(CoalesceBlockData) + COALESCE_FILLER_SIZE);
				//если можно присоединить следующий блок, прибавляем его размер
				((CoalesceFreeEl*)p)->blockSize += nextFreeEl->blockSize + sizeof(CoalesceBlockData) + 2 * COALESCE_FILLER_SIZE;
#else
			}
			if (currentElData->pNext != nullptr && currentElData->pNext->isFree) {
				CoalesceFreeEl* nextFreeEl = (CoalesceFreeEl*)((char*)(currentElData->pNext) + sizeof(CoalesceBlockData));
				((CoalesceFreeEl*)p)->blockSize += nextFreeEl->blockSize + sizeof(CoalesceBlockData);
#endif // _DEBUG
				//исключение следующего блока из списка блоков
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
	//проверка, каким аллокатором освободить
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
	//освобождение блоков от первого к последнему
	for (int i = 1; i < numPageBlocks_; ++i) {
		FSAHeader* pTempBlock = pCurrentBlock;
#ifdef _DEBUG
		FSAHeader* pCurrentPage = pCurrentBlock;
		while (pCurrentPage != nullptr) {
			delete[](pCurrentPage->usedBlockFlags_);
			pCurrentPage = (FSAHeader*)(pCurrentPage->pNextPage_);
		}
		pCurrentBlock = (FSAHeader*)((char*)pCurrentBlock
			+ FILLER_RATIO * FIXED_PAGE_SIZE * (ALLOCATOR_PAGES_IN_BLOCK - 1));
#else
		pCurrentBlock = (FSAHeader*)((char*)pCurrentBlock
			+ FIXED_PAGE_SIZE * (ALLOCATOR_PAGES_IN_BLOCK - 1));
#endif // _DEBUG
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
	//освобождение блоков от первого к последнему
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
	//проверка, что аллокатор освобожден
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
		//поиск занятых элементов по массиву флагов, вычисление адреса по индексу
		for (int i = 0; i < (FILLER_RATIO * FIXED_PAGE_SIZE - sizeof(FSAHeader)) / blockSize_ / FILLER_RATIO; ++i) {
			if (pCurrentPage->usedBlockFlags_[i]) {
				char* currentAddress = (char*)(pCurrentPage->pBuf_) + i * FILLER_RATIO * blockSize_;
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
			//перебор по всем элементам, отбор занятых и вычисление их размера
			if (!(currentEl->isFree)) {
				size_t currentSize;
				if (currentEl->pNext != nullptr) {
					currentSize = (char*)currentEl->pNext - ((char*)currentEl + sizeof(CoalesceBlockData) + 2 * COALESCE_FILLER_SIZE);
				}
				else {
					currentSize = (char*)pCurrentPage + COALESCE_PAGE_SIZE -
						((char*)currentEl + sizeof(CoalesceBlockData) + 2 * COALESCE_FILLER_SIZE);
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
