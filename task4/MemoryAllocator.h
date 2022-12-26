#ifndef MEMORYALLOCATOR_H
#define MEMORYALLOCATOR_H

class MemoryAllocator
{
public:
	MemoryAllocator();
	virtual ~MemoryAllocator();
	virtual void init();
	virtual void destroy();
	virtual void* alloc(size_t size);
	virtual void free(void* p);
#ifdef _DEBUG
	virtual void dumpStat() const;
	virtual void dumpBlocks() const;
#endif // _DEBUG

	struct FSAHeader //заголовок страницы Fixed-Size аллокатора
	{
		void* pBuf_; //указатель на данные
		void* pNextPage_; //следующая страница
		void* fHead_; //первый свободный блок
		int numInitialized_; //число инициализированных блоков (добавленных в список свободных блоков)
		bool isInitialized_; //все ли блоки инициализированы
#ifdef _DEBUG
		bool* usedBlockFlags_; //массив флагов занят/свободен для каждого блока (для dumpBlocks)
#endif // _DEBUG
	};

	class FixedSizeAllocator
	{
	public:
		FixedSizeAllocator(); //нужен для инициализации основного MemoryAllocator
		FixedSizeAllocator(size_t size);
		void init(void* start);
		void initElement(FSAHeader* header); //инициализация блока при первом заполнении страницы
		void destroy();
		void* alloc();
		bool free(void* p); //возвращает bool, т.к. проверяет, относится ли блок к этому аллокатору
#ifdef _DEBUG
		void dumpStat() const;
		void dumpBlocks() const;
#endif // _DEBUG
	private:
		size_t blockSize_; //размер блока
		void* pFirstPage_; //первая страница
		int numPageBlocks_; //число блоков страниц (находящихся в разных частях памяти)
#ifdef _DEBUG
		int numFreeBlocks_;
		int numUsedBlocks_;
		long usedMemory_; //показатели для dumpStat
#endif
	};

	struct CoalesceHeader //заголовок страницы Coalesce аллокатора
	{
		void* pBuf_; //указатель на данные
		void* pNextPage_; //следующая страница
		void* fHead_; //первый свободный блок
	};

	struct CoalesceFreeEl //структура хранится внутри свободного блока Coalesce аллокатора
	{
		CoalesceFreeEl* pPrevFree; //предыдущий свободный блок
		CoalesceFreeEl* pNextFree; //следующий свободный блок
		int blockSize; //размер блока (для простоты поиска)
	};

	struct CoalesceBlockData //заголовок блока Coalesce аллокатора
	{
		CoalesceBlockData* pPrev; //предыдущий блок
		CoalesceBlockData* pNext; //следующий блок
		bool isFree; //свободен ли блок
	};

	class CoalesceAllocator
	{
	public:
		CoalesceAllocator();
		void init(void* start);
		void destroy();
		void* alloc(size_t size);
		bool free(void* p); //аналогично FSA
#ifdef _DEBUG
		void dumpStat() const;
		void dumpBlocks() const;
#endif // _DEBUG
	private:
		void* pFirstPage_; 
		int numPageBlocks_;
#ifdef _DEBUG
		int numFreeBlocks_;
		int numUsedBlocks_;
		long usedMemory_; //поля аналогично FSA
#endif
	};

private:
	static constexpr int N_FIXED_SIZE_ALLOCATORS = 6; //количество FSA с разным размером блока
	static constexpr int MIN_FIXED_SIZE = 16; //минимальный размер блока FSA
	static constexpr int FIXED_ALLOCATION_THRESHOLD = 512; //максимальный размер блока FSA
	static constexpr int COALESCE_ALLOCATION_THRESHOLD = 10 * 1024 * 1024; //максимальный размер блока Coalesce аллокатора
	static constexpr int ALLOCATOR_PAGES_IN_BLOCK = 16; //число страниц, резервируемых за один раз
	static constexpr int FIXED_PAGE_SIZE = 1024 * 1024; //размер страницы FSA
	static constexpr int COALESCE_PAGE_SIZE = 16 * 1024 * 1024; //размер страницы Coalesce аллокатора
	FixedSizeAllocator fsa_[N_FIXED_SIZE_ALLOCATORS]; //массив FSA
	CoalesceAllocator coalesceAllocator_; //Coalesce аллокатор
#ifdef _DEBUG
	bool isInitialized_; //для проверки при инициализации и уничтожении объекта
	static constexpr __int64 fillerBlock_ = 1111111111111111111; //для заполнения пустых блоков памяти при отладке
	static constexpr int FILLER_RATIO = 3; //коэффициент увеличения страницы FSA в отладочном режиме
	static constexpr int COALESCE_FILLER_SIZE = 4096; //размер контрольного пустого блока для Coalesce аллокатора
#endif // _DEBUG
};

#endif
