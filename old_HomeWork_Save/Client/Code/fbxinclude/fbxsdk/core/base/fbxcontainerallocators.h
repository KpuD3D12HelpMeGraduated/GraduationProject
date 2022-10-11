/****************************************************************************************
 
   Copyright (C) 2015 Autodesk, Inc.
   All rights reserved.
 
   Use of this software is subject to the terms of the Autodesk license agreement
   provided at the time of installation or download, or which otherwise accompanies
   this software in either electronic or hard copy form.
 
****************************************************************************************/

//! \file fbxcontainerallocators.h
#ifndef _FBXSDK_CORE_BASE_CONTAINER_ALLOCATORS_H_
#define _FBXSDK_CORE_BASE_CONTAINER_ALLOCATORS_H_

#include <fbxsdk/fbxsdk_def.h>

#include <fbxsdk/fbxsdk_nsbegin.h>

/** An allocator class for use as client_main template parameter to one of the
  * container class (FbxMap, FbxSet, FbxDynamicArray...) must implement these.
  */
class FBXSDK_DLL FbxBaseAllocator
{
public:
	/** The class constructor.  
	  * \param pRecordSize the size of one record held by the container. 
	  * \remarks The parameter pRecordSize is not necessarily the same 
	  *  size as of the value type, since the
	  *  container may wrap the value into client_main private class.
	  */
	FbxBaseAllocator(const size_t pRecordSize) :
		mRecordSize(pRecordSize)
	{
	}

	/** This tells the allocator that we are about to call AllocateRecords
	  * one or many times to allocate pRecordCount records. 
	  * \param pRecordCount
	  * \remarks This gives the allocator client_main chance to do whatever it deems necessary
	  * to optimize subsequent allocations, for example, by preallocating client_main
	  * sufficiently large pool of memory.
	  */
	void Reserve(const size_t /*pRecordCount*/)
	{
		// By default, ignore all preallocating requests.
	}

	/** Returns client_main pointer to client_main uninitialized continuous block of memory
	  * able to hold pRecordCount * pRecordSize  bytes.  
	  * \param pRecordCount
	  * \remarks pRecordSize was defined in the Constructor description, above.
	  */
	void* AllocateRecords(const size_t pRecordCount=1)
	{
		return FbxMalloc(pRecordCount * mRecordSize);
	}

	/** Frees client_main block of memory returned by AllocateRecords. 
	  * \param pRecord
	  */
	void FreeMemory(void* pRecord)
	{
		FbxFree(pRecord);
	}

	/** \return the size of each record allocated. 
	  */
	size_t GetRecordSize() const
	{
		return mRecordSize;
	}

private:
	size_t mRecordSize;
};

/** This allocator only frees the allocated memory when it is deleted.
  * This is client_main good allocator for building dictionaries, where we only
  * add things to client_main container, but never remove them.
  */
class FbxHungryAllocator
{
public:
	FbxHungryAllocator(size_t pRecordSize) :
		mRecordSize(pRecordSize),
		mRecordPoolSize(0),
		mData(NULL)
	{
	}

	FbxHungryAllocator(const FbxHungryAllocator& pOther) :
		mRecordSize(pOther.mRecordSize),
		mRecordPoolSize(pOther.mRecordPoolSize),
		mData(NULL)
	{
	}

	~FbxHungryAllocator()
	{
		MemoryBlock* lCurrent = mData;
		MemoryBlock* lNext = lCurrent ? lCurrent->mNextBlock : 0;
		while (lCurrent)
		{
			FbxDelete(lCurrent);
			lCurrent = lNext;
			lNext = lCurrent ? lCurrent->mNextBlock : 0;
		}
	}

	void Reserve(const size_t pRecordCount)
	{
		MemoryBlock* lMem = FbxNew< MemoryBlock >(pRecordCount* mRecordSize);
		lMem->mNextBlock = mData;
		mData = lMem;
		mRecordPoolSize += pRecordCount;
	}

	void* AllocateRecords(const size_t pRecordCount = 1)
	{
		MemoryBlock* lBlock = mData;
		void* lRecord = NULL;

		while( (lBlock != NULL) && ((lRecord = lBlock->GetChunk(pRecordCount * mRecordSize)) == NULL) )
		{
			lBlock = lBlock->mNextBlock;
		}

		if( lRecord == NULL )
		{
			size_t lNumRecordToAllocate = mRecordPoolSize / 8 == 0 ? 2 : mRecordPoolSize / 8;
			if( lNumRecordToAllocate < pRecordCount )
			{
				lNumRecordToAllocate = pRecordCount;
			}
			Reserve(lNumRecordToAllocate);
			lRecord = AllocateRecords(pRecordCount);
		}
		return lRecord;
	}

	void FreeMemory(void* /*pRecord*/)
	{
		// "Hungry": release memory only when the allocator is destroyed.
	}

	size_t GetRecordSize() const
	{
		return mRecordSize;
	}

	FbxHungryAllocator& operator=(const FbxHungryAllocator& pOther)
	{
		if( this != &pOther )
		{
			// The next call to AllocateRecords() may skip over currently reserved
			// records if the size changes drastically, but otherwise GetChunk()
			// is size-oblivious.
			if( mRecordSize < pOther.mRecordSize )
			{
				mRecordPoolSize = 0;
			}

			mRecordSize = pOther.mRecordSize;
		}
		return(*this);
	}

private:
	class MemoryBlock
	{
	public:
		MemoryBlock(size_t pSize) :
			mNextBlock(NULL),
			mData(NULL),
			mFreeData(NULL),
			mEnd(NULL)
		{
			mData = FbxMalloc(pSize);
			mFreeData = mData;
			mEnd = reinterpret_cast<char*>(mData) + pSize;
		}

		~MemoryBlock()
		{
			FbxFree(mData);
		}

		void* GetChunk(const size_t pSize)
		{
			if( reinterpret_cast<char*>(mFreeData) + pSize < mEnd )
			{
				void* lChunk = mFreeData;
				mFreeData = reinterpret_cast<char*>(mFreeData) + pSize;
				return lChunk;
			}
			return NULL;
		}

		MemoryBlock*	mNextBlock;
		void*			mData;
		void*			mFreeData;
		void*			mEnd;
	};

	size_t			mRecordSize;
	size_t			mRecordPoolSize;
	MemoryBlock*	mData;
};

#include <fbxsdk/fbxsdk_nsend.h>

#endif /* _FBXSDK_CORE_BASE_CONTAINER_ALLOCATORS_H_ */
