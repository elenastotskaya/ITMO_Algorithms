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

	struct FSAHeader
	{
		void* pBuf_;
		void* pNextPage_;
		void* fHead_;
		int numInitialized_;
		bool isInitialized_;
#ifdef _DEBUG
		bool* usedBlockFlags_;
#endif // _DEBUG
	};

	class FixedSizeAllocator
	{
	public:
		FixedSizeAllocator();
		FixedSizeAllocator(size_t size);
		void init(void* start);
		void initElement(FSAHeader* header);
		void destroy();
		void* alloc();
		bool free(void* p);
#ifdef _DEBUG
		void dumpStat() const;
		void dumpBlocks() const;
#endif // _DEBUG
	private:
		size_t blockSize_;		
		void* pFirstPage_;
		int numPageBlocks_;
#ifdef _DEBUG
		int numFreeBlocks_;
		int numUsedBlocks_;
		long usedMemory_;
#endif
	};

	struct CoalesceHeader
	{
		void* pBuf_;
		void* pNextPage_;
		void* fHead_;
	};

	struct CoalesceFreeEl
	{
		CoalesceFreeEl* pPrevFree;
		CoalesceFreeEl* pNextFree;
		int blockSize;
	};

	struct CoalesceBlockData
	{
		CoalesceBlockData* pPrev;
		CoalesceBlockData* pNext;
		bool isFree;
	};

	class CoalesceAllocator
	{
	public:
		CoalesceAllocator();
		void init(void* start);
		void destroy();
		void* alloc(size_t size);
		bool free(void* p);
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
		long usedMemory_;
#endif
	};

private:
	static constexpr int N_FIXED_SIZE_ALLOCATORS = 6;
	static constexpr int MIN_FIXED_SIZE = 16;
	static constexpr int FIXED_ALLOCATION_THRESHOLD = 512;
	static constexpr int COALESCE_ALLOCATION_THRESHOLD = 10 * 1024 * 1024;
	static constexpr int ALLOCATOR_PAGES_IN_BLOCK = 16;
	static constexpr int FIXED_PAGE_SIZE = 1024 * 1024;
	static constexpr int COALESCE_PAGE_SIZE = 16 * 1024 * 1024;
	FixedSizeAllocator fsa_[N_FIXED_SIZE_ALLOCATORS];
	CoalesceAllocator coalesceAllocator_;
#ifdef _DEBUG
	bool isInitialized_;
#endif // _DEBUG
};

#endif
