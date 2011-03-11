// Copyright 2002 Ashley Wise
// JediMasterThrash@comcast.net

#ifndef SPARSEARRAY_H
#define SPARSEARRAY_H

#pragma warning (disable : 4786)
#pragma warning (disable : 4800)
#include <vector>
#include <stdexcept>
#include <iostream>
#include "JMTLib.h"

using namespace std;

namespace JMT	{

//Re-define this to change the default memory manager parameters
// node-to-owner ratio, initial pre-allocation pool, true if nail memory to physical RAM
//*NOTE: If these are left as empty #defines, the compiler fails to
//use them as empty parameters to the MemoryManager constructor.
#ifndef SPARSEARRAY_MM_INIT
#define SPARSEARRAY_MM_INIT 0x40, 0x100, false /*uses defaults*/
#endif
#ifndef SPARSEARRAYNODE_MM_INIT
#define SPARSEARRAYNODE_MM_INIT 0x40, 0x100, false /*uses defaults*/
#endif

/*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*\
	SparseArray class
	 ________
	|Overview|
	
	This class is used to create large indexible arrays where it is known that
	access will be sparse or of limited range.

	For instance, say you want to model an entire 64-bit memory space, and
	allow indexed read/write access to any byte. This would require 18
	quintillion (10^18) bytes of storage.

	However, if only say 10^6 of those possible addresses will actually be used,
	then you can save space by only allocating the parts of the greater memory
	space that are used.

	What constitutes sparse? In order for this class to function efficiently,
	if the indexable space is 2^n, then only about 2^(n/2) of the space should
	be accessed. In order for the data to be able to fit in computer RAM,
	regardless of the indexable space, at most about 2^20 of space should be
	accessed.
	 _________________
	|Internal Struture|

	SparseArray partitions the indexable space into bit divisions.
	For instance, a 32-bit index space could be divided into 4 8-bit index
	sub-ranges: [31:24][23:16][15:8][7:0]
	
	Each bit-range is stored as a node which holds an array of pointers.
	The last used bit-range is a node which holds an array of data.
	
	The index space is arranged into a tree. In the above example, the root
	node would have an array of 256 pointers [31:24]. Each of those would
	point to a node with 256 pointers [23:16]. Each of those would point to a
	node with 256 pointers [15:8], and each of those would point to a leaf with
	256 data [7:0].

	Each node (array of pointers) and leaf (array of data) is only created when
	it is indexed. So if you index 0x00000001, then in the above example,
	[0][0][0][0-255] is allocated. But [1-255][x][x][x] is not, and neither is
	[0][1-255][x][x], etc. Total memory allocation is 256*4 = 1024 versus
	4.3 billion.

	Unused index sub-ranges (bit-ranges) are skipped over by the nodes.
	They are not allocated. They serve as "don't cares".
	 __________
	|Data Types|

	IndexType is used for index space addresses and masks.
	unsigned int is used for real array addresses.
	unsigned int is used for counting how many bits are set in a mask/ division
	 _____
	|Usage|

	The indexability of the data is the first template parameter. This is the
	type of the index. For instance: unsigned char for an 8-bit index space,
	unsigned int for a 32-bit index space, etc. Intermediate spaces can be
	obtained by setting the index range and unused bit parameters.

	The type of the data is the second template parameter. It could be an
	integral type, a class type, a pointer type, etc. Values will
	always be initialized to the default construction, DataType().

	The "shape" of the object is defined by the indexable range and the index
	bit divisions. The shape of an object can be tweaked to fine-tune
	performance. See the overloaded constructor for more information.

	Any type of intrinsic or user defined data can be used. Even pointer types.
	The only restrictions are that the following operators must be defined:
	ValueType()				//Default construction
	operator<<(ostream, ValueType)	//Only if you intend to use stream
	operator>>(istream, ValueType)	//	I/O capability
	operator=(ValueType)	//	""	""	"" or CopyTo()
	operator==(ValueType)	//	""	""	"" or CopyTo() or Compact()

	The underlying structure operates transparently to the user. All the user
	has to do is create an instance:
	SparseArray<IndexType, DataType> MyArray; //MyArray(Optional Parameters)
	And access it like any other array:
	MyArray[1023]++;
	Y = MyArray[X];
	 ______________
	|Large Database|

	SparseArray can actually be used to create a large database in a similar
	structure to a B-Tree. This can be done by using SparseArray as the datatype
	SparseArray<unsigned int, SparseArray<unsigned int, Record> > MyDatabase;
	You can then store sub-trees off to disk to using the output capabilities:
	oFile << MyDatabase[1023];
	And clear the memory
	MyDatabase[1023] = SparseArray<unsigned int, Record>();
	And load a new sub-tree from disk into memory
	iFile >> MyDatabase[1024];

	A better approach would be to store the name of the file in the SparseArray.
	Then you can easily have as many levels deep of disk pages as you need.

	Each individual SparseArray is limited to being stored in memory. But
	by chaining SparseArray and using I/O, you can expand the database size to
	anything.
	 _______________
	|Multi-Threading|

	SparseArray comes in both a single-threaded and multi-threaded library
	version. The single-threaded version lacks internal synchronization, and
	is faster. The multi-threaded version is slower but is thread-safe. To
	use the multi-threaded version, #define MULTITHREADED
	
	Data can be accessed concurrently by multiple threads through operator[],
	Read(), operator>>, operator<<, and CopyTo().

	Accesses to Compact(), destructor, and operator= will wait until no more
	threads are accessing this subtree.
	
	In addition to other exceptions, when Multi-Threading is enabled, any
	function could also throw a SynchFailed exception. Also, References could
	throw an UnsafeReference exception.

	Only the subtrees and data locations are protected.
	You will need to implement additional synchronization if your data
	involves pointers to other data, and you allow multiple references to
	that external data.

	While Compact() is a fully threadsafe function, the destructor and operator=
	are not. They will wait until all references obtained prior to calling them
	are released. However, they cannot protect against accessing a SafeArray
	after it has been destroyed. It is your job to make sure noone will access
	it after it is destroyed, and that it is not destroyed until after all
	accesses are completed. Remember that operator= destroys the current
	SparseArray before assigning it to the new one.

	Although not implemented, subtree access is threadsafe, provided the
	subtree reference is obtained in the same manner as data references.
	 _____
	|Notes|

	The goal of this class is to maintain speed of indexability while using as
	little memory as possible. The speed is maintained by using direct indexing
	into the sub-range arrays, and allowing the bypass of unused sub-ranges.

	The structure could be optimized by allowing dynamic balancing of the
	bit-range allocations of nodes anywhere in the tree. But this would require
	a lot of work and probably wouldn't give that much extra performance.

	The structure could be made more configurable by allowing the index
	sub-ranges to be any any modulos value instead of just powers of 2
	(number of bits). This could allow for things like optimizing for decimal
	index accesses. However, this would make the indexed access much slower
	since it would require a modulos/division at each dereference, instead
	of just a bit shift/mask.

	Memory could be optimized by unallocating nodes/leaves when it is detected
	that all the values are at the default constructed value. However, it would
	require too much overhead to detect this, and we could easily run into a
	situation where a field is toggled between 0 and 1 and thus incurrs an
	allocate/ deallocate at every other access. I decided upon an explicit
	Compact() function instead.

	The functionality is built in to allow people to make copies of sub-trees
	and use them as new trees. However, I haven't added an interface for this
	yet, as I'm still not sure how to assure thread safety when people could
	have pointers to subtrees floating around.
\*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*/
template<class IndexType, class ValueType>
class SparseArray  
{
public:

	#define SA_MAX_ARRAY_BITS 8
	#define SA_MAX_ARRAY_SIZE (1 << SA_MAX_ARRAY_BITS)

	/*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*\
	 *	NodeP, DataP
	 *
	 *	This class holds an array of pointers, which allows us to control
	 *	allocation of an array of pointers (otherwise not possible with normal
	 *	overloaded operator new/delete).
	\*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*/
	struct NodeP
	{
		SparseArray *pNode;

		struct One { NodeP p[2]; };
		struct Two { NodeP p[4]; };
		struct Three { NodeP p[8]; };
		struct Four { NodeP p[16]; };
		struct Five { NodeP p[32]; };
		struct Six { NodeP p[64]; };
		struct Seven { NodeP p[128]; };
		struct Eight { NodeP p[256]; };

		static NodePool<One> MemoryManager2;
		static NodePool<Two> MemoryManager4;
		static NodePool<Three> MemoryManager8;
		static NodePool<Four> MemoryManager16;
		static NodePool<Five> MemoryManager32;
		static NodePool<Six> MemoryManager64;
		static NodePool<Seven> MemoryManager128;
		static NodePool<Eight> MemoryManager256;

		//*NOTE: GCC adds an extra sizeof(size_t = long) bytes to the AllocBlock to use
		//for storing the size of the array. GCC will store the the size for you in the first
		//bytes of the address you return, and then will increment the return address
		//when it gives it back to the code that called new[].
		//I've chosen to bypass this entirely by having SparseArray always call these
		//overloaded operators directly, rather than using the real new/delete operators.
		//Note that the constructor will not get called this way.
		static void *operator new[](size_t AllocBlock)
		{
			unsigned int Length = AllocBlock / sizeof(NodeP);
			void *pNode = NULL;
			if(Length <= 2)
				pNode = MemoryManager2.Pop();
			else if(Length <= 4)
				pNode = MemoryManager4.Pop();
			else if(Length <= 8)
				pNode = MemoryManager8.Pop();
			else if(Length <= 16)
				pNode = MemoryManager16.Pop();
			else if(Length <= 32)
				pNode = MemoryManager32.Pop();
			else if(Length <= 64)
				pNode = MemoryManager64.Pop();
			else if(Length <= 128)
				pNode = MemoryManager128.Pop();
			else if(Length <= 256)
				pNode = MemoryManager256.Pop();
			if(!pNode)
				throw OutOfMemory();
			return pNode;
		}
		static void operator delete[](void *pNodeP, size_t AllocBlock)
		{
			unsigned int Length = AllocBlock / sizeof(NodeP);
			if(Length <= 2)
				MemoryManager2.Push((One *)pNodeP);
			else if(Length <= 4)
				MemoryManager4.Push((Two *)pNodeP);
			else if(Length <= 8)
				MemoryManager8.Push((Three *)pNodeP);
			else if(Length <= 16)
				MemoryManager16.Push((Four *)pNodeP);
			else if(Length <= 32)
				MemoryManager32.Push((Five *)pNodeP);
			else if(Length <= 64)
				MemoryManager64.Push((Six *)pNodeP);
			else if(Length <= 128)
				MemoryManager128.Push((Seven *)pNodeP);
			else if(Length <= 256)
				MemoryManager256.Push((Eight *)pNodeP);
		}

		SparseArray &operator *()	{	return *pNode;	}
		SparseArray *operator ->()	{	return pNode;	}
		operator bool()	{	return (bool)pNode;	}
		bool operator !()	{	return !pNode;	}
//		NodeP()	{	pNode = NULL;	}
		NodeP &operator =(SparseArray *pSA)	{	pNode = pSA;	return *this;	}
		operator SparseArray *()	{	return pNode;	}
	};
	struct DataP
	{
		ValueType pData;

		struct One { DataP p[2]; };
		struct Two { DataP p[4]; };
		struct Three { DataP p[8]; };
		struct Four { DataP p[16]; };
		struct Five { DataP p[32]; };
		struct Six { DataP p[64]; };
		struct Seven { DataP p[128]; };
		struct Eight { DataP p[256]; };

		static NodePool<One> MemoryManager2;
		static NodePool<Two> MemoryManager4;
		static NodePool<Three> MemoryManager8;
		static NodePool<Four> MemoryManager16;
		static NodePool<Five> MemoryManager32;
		static NodePool<Six> MemoryManager64;
		static NodePool<Seven> MemoryManager128;
		static NodePool<Eight> MemoryManager256;

		static void *operator new[](size_t AllocBlock)
		{
			unsigned int Length = AllocBlock / sizeof(DataP);
			void *pDataP = NULL;
			if(Length <= 2)
				pDataP = MemoryManager2.Pop();
			else if(Length <= 4)
				pDataP = MemoryManager4.Pop();
			else if(Length <= 8)
				pDataP = MemoryManager8.Pop();
			else if(Length <= 16)
				pDataP = MemoryManager16.Pop();
			else if(Length <= 32)
				pDataP = MemoryManager32.Pop();
			else if(Length <= 64)
				pDataP = MemoryManager64.Pop();
			else if(Length <= 128)
				pDataP = MemoryManager128.Pop();
			else if(Length <= 256)
				pDataP = MemoryManager256.Pop();
			if(!pDataP)
				throw OutOfMemory();
			return pDataP;
		}
		static void operator delete[](void *pDataP, size_t AllocBlock)
		{
			unsigned int Length = AllocBlock / sizeof(DataP);
			if(Length <= 2)
				MemoryManager2.Push((One *)pDataP);
			else if(Length <= 4)
				MemoryManager4.Push((Two *)pDataP);
			else if(Length <= 8)
				MemoryManager8.Push((Three *)pDataP);
			else if(Length <= 16)
				MemoryManager16.Push((Four *)pDataP);
			else if(Length <= 32)
				MemoryManager32.Push((Five *)pDataP);
			else if(Length <= 64)
				MemoryManager64.Push((Six *)pDataP);
			else if(Length <= 128)
				MemoryManager128.Push((Seven *)pDataP);
			else if(Length <= 256)
				MemoryManager256.Push((Eight *)pDataP);
		}

//		DataP()	{	pData = Default;	}
		DataP &operator =(ValueType pVal)	{	pData = pVal;	return *this;	}
		operator ValueType &()	{	return pData;	}
	};

protected:
	/*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*\
	 *	LevelData
	 *
	 *	This represents the common data that each tree level needs. Since all
	 *	nodes/leaves of the same tree level have the exact same data, instead
	 *	of allocating the data every time for each node, we can just allocate
	 *	the data once per node level.
	 *
	 *	One instance of a vector of Level Data will always be associated with
	 *	each SparseArray tree.
	\*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*/
	struct LevelData
	{
		//Specifies the level number. 0 is the root.
		unsigned int Level;
		//Specifies the indexability of this level. Ie, the array length.
		unsigned int Size;
		//The beginning and ending indexes in the indexable range
		IndexType Begin, End;
		//Specifies which bits of the [index] to use for this node level
		IndexType Mask;	//Specified bits are 1's
		//Specifies the MSBs which represent upper-level node bits
		IndexType UpperMask;	//Specified bits are 1's
		//Specifies the MSB which represent lower-level node bits
		IndexType LowerMask;	//Specified bits are 1's
		//Specifies how many bits to use for this node level
		unsigned int NumBits;
		//The amount to shift the index bits right so that the used
		//bits are right-aligned for indexing into the sub-array.
		unsigned int Shamt;
		//Specifies the used bit ranges above this level. Used bits are 1's
		IndexType UsedMask;
		//If true, this represents an unused bit division, so it's level
		//can be skipped.
		bool Skip;
		//If true, this represents the first used bit division, so it's
		//the root of a tree and controls the level data vector.
		bool Root;
		//If true, this represents the last used bit division, so it's
		//a leaf of the tree and holds data instead of node pointers.
		bool Leaf;
		//Extra text to store before and after the index,value in the serialized file, and the delimiter inbetween the index and value.
		const char *sPreFix, *sDelim, *sPostFix;
		//True if all the index values are printed to the serialized file, including ones with default values
		bool fPrintAll;
		//Constructor
		LevelData(unsigned int level, unsigned int size, IndexType begin, IndexType end, IndexType mask, IndexType uppermask, IndexType lowermask, unsigned int numbits, unsigned int shamt, IndexType usedmask, bool skip, bool root, bool leaf)
		{
			Level = level;
			Size = size;
			Begin = begin;
			End = end;
			Mask = mask;
			UpperMask = uppermask;
			LowerMask = lowermask;
			NumBits = numbits;
			Shamt = shamt;
			UsedMask = usedmask;
			Skip = skip;
			Root = root;
			Leaf = leaf;
			sPreFix = "";
			sDelim = ",";
			sPostFix = "\n";
			fPrintAll = false;
		}
	};

public:
	
	/**************************************************************************\
	 *	SparseArray()
	 *
	 *	Default constructor. Start and end index range set at 0 to (unsigned)-1.
	 *	Index divisions set to 8-bits each (or whatever the size of a Byte is
	 *	for the target platform).
	 *	A node constructed with this is the root node of a tree.
	 *
	 *	Calls: Initialize()
	 *
	 *	Exceptions: See Initialize()
	\******/
	SparseArray()
	{
		Initialize(0, -1, vector<short>(sizeof(IndexType), BITS_PER_BYTE));
	}

	/**************************************************************************\
	 *	SparseArray( [in] Index Begin, [in] Index End,
	 *		[in] Array of bit divisions )
	 *
	 *	Overloaded constructor. The first two parameters define the start and
	 *	end index range. Indexes outside of this range will throw an exception.
	 *	Index bit divisions are set to the values in the array, with Array[0]
	 *	being the number of MSBs for the first division, and Array[len-1]
	 *	being the number of LSBs for the last division.
	 *
	 *	Example:
	 *	SparseArray<uint, uint>(0x10000000, 0x1FFFFFFF, {4,8,16,4})
	 *	Builds an object which is indexable from 268435456 to 536870911, and the index
	 *	is divided into uint[31:0] = [31:28][27:20][19:4][3:0]
	 *
	 *	To specify an unused range, use the negative of the number. For
	 *	instance:
	 *	SparseArray<uint64, uint64>(0x0, 0xFFFFFFFFF, {-28,4,8,8,8,6,-2})
	 *	Builds an object which is 36-bit indexable, and the index
	 *	is divided into uint64[63:0] =
	 *	[unused][35:32][31:24][23:16][15:8][7:2][unused]
	 *	The unused range in the MSB allows there to be no extra overhead (from
	 *	pointer dereferencing or memory allocation) for the unused top 28 bits.
	 *	The unused range in the LSB serves as a "don't care". For instance,
	 *	this memory space might only care about DWORD aligned accesses.
	 *	In this example, 0x0, 0x1, 0x2, and 0x3 would all return a reference
	 *	to the same data. A "don't care" range could also be specified in the
	 *	middle of the bit divisions.
	 *
	 *	The maximum bit range is defined by MaxBitRange.
	 *	MaxBitRange is defined so that 2^MaxBitRange fits in an
	 *	unsigned int (ie, the size of the associated array). This is 31 bits on
	 *	most platforms.
	 *	There is no limit for the unused bit range (the fact that it must fit
	 *	into a signed short limits it to -32768. A bit range cannot be 0.
	 *	A bit range defines the size of sub-arrays which will be allocated.
	 *	So a 8-bit division will require 256 cells of memory, and a 31-bit
	 *	division will require 2147483648 cells of memory. It is not recommended
	 *	to use a division greater than 20 bits.
	 *
	 *	A node constructed with this is the root node of a tree.
	 *
	 *	Calls: Initialize()
	 *
	 *	Exceptions: See Initialize()
	\******/
	SparseArray(IndexType begin, IndexType end, const vector<signed short> &BitDivisions)
	{
		Initialize(begin, end, BitDivisions);
	}

	/**************************************************************************\
	 *	SparseArray( [in] object to copy )
	 *
	 *	Copy constructor. The new and copy object must have the same indextype
	 *	template, but can have different ValueTypes.
	 *	A node constructed with this is the root node of a tree.
	 *
	 *	If I don't provide the specialized version as well, the VC compiler
	 *	uses the default binary copy constructor instead.
	 *
	 *	Calls: Copy()
	 *
	 *	Exceptions: See Copy()
	\******/
	template<class SourceIndexType, class SourceValueType>
	SparseArray(const SparseArray<SourceIndexType, SourceValueType> &Source)
	{
		Copy(Source);
	}
	SparseArray(const SparseArray &Source)
	{
		Copy(Source);
	}

	/**************************************************************************\
	 *	operator=( [in] object to copy )
	 *
	 *	Copy operator. The new and copy object must have the same indextype
	 *	template, but can have different ValueTypes.
	 *	A node constructed with this is the root node of a tree.
	 *
	 *	Returns a reference to this object.
	 *
	 *	If I don't provide the specialized version as well, the VC compiler
	 *	uses the default binary assignment operator instead.
	 *
	 *	Calls: Destroy(), Copy()
	 *
	 *	Exceptions: See Copy()
	\******/
	template<class SourceIndexType, class SourceValueType>
	SparseArray &operator=(const SparseArray<SourceIndexType, SourceValueType> &Source)
	{
		//Destroy data
		Destroy();

		Copy(Source);

		return *this;
	}
	SparseArray &operator=(const SparseArray &Source)
	{
		//Destroy data
		Destroy();

		Copy(Source);

		return *this;
	}


	/**************************************************************************\
	 *	~SparseArray()
	 *
	 *	Destructor.
	 *
	 *	Calls: Destroy()
	\******/
	~SparseArray()
	{
		Destroy();
	}

	/**************************************************************************\
	 *	operator[]( [in] index )
	 *
	 *	Returns a reference to the value at the indexed location.
	 *	If the indexed location is out of bounds, an exception is thrown.
	 *	If the indexed location has not yet been created, it is created and the
	 *	location is initialized to the default construction of ValueType
	 *	(ie, ValueType())
	 *
	 *	Calls: Private overloaded constructor
	 *
	 *	Exceptions:
	 *		OutOfBounds	-Index not in range Begin to End
	 *		OutOfMemory -Not enough memory to allocate more nodes.
	\******/
	ValueType &operator[](IndexType Index)
	{
	#ifndef SA_NO_BOUNDS_CHECK
		//bounds check
		if( Index < pLevelData->Begin || Index > pLevelData->End
			||
			(Index & pLevelData->UsedMask) < ThisIndex
			||
			(Index & pLevelData->UsedMask) > (IndexType)(ThisIndex | ~pLevelData->UpperMask)	)
			throw OutOfBounds();
	#endif

		unsigned int NodeIndex = (unsigned int)((Index & pLevelData->Mask) >> pLevelData->Shamt);

		if(!pLevelData->Leaf)
		{	//This is a node
			if(!pNodes[NodeIndex])
			{
				//This node doesn't exist. Create it.

				//Find out which level the next node should be at
				unsigned int NextLevel = pLevelData->Level + 1;
				while( (*pLevelDataVector)[NextLevel].Skip)
					NextLevel++;	//Skip this level

				if( !(pNodes[NodeIndex] = new SparseArray(ThisIndex + ((IndexType)NodeIndex << pLevelData->Shamt), pLevelDataVector, NextLevel)) )
					throw OutOfMemory();
			}
			return (*pNodes[NodeIndex])[Index];
		}
		else
		{	//This is a leaf
			//Return the reference
			#ifdef NODEPOOL_DEBUG
			if((ValueType &)pData[NodeIndex] == (ValueType)0xCDCDCDCDCDCDCDCD)
				throw runtime_error("SparseArray::Operator[]() pData == CD");
			#endif
			return pData[NodeIndex];
		}
	}

	/**************************************************************************\
	 *	operator[]( [in] index )
	 *	Read( [in] index )
	 *
	 *	Returns a const reference to the value at the indexed location.
	 *	If the indexed location is out of bounds, an exception is thrown.
	 *	If the indexed location has not yet been created, a reference to a
	 *	dummy default construction of ValueType is returned (ie, ValueType()).
	 *	
	 *	The advantage of this function is that it only traverses existing nodes.
	 *	It does not have the overhead of creating nodes. If the location hasn't
	 *	been created yet, it will return fast.
	 *
	 *	The constant version of operator[] will always be used when called from
	 *	a constant function. To purposfully get quicker, read-only access, the
	 *	Read function is also supplied, which is identical.
	 *
	 *	Exceptions:
	 *		OutOfBounds	-Index not in range Begin to End
	\******/
	const ValueType &operator[](IndexType Index) const
	{
		return Read(Index);
	}
	const ValueType &Read(IndexType Index) const
	{
	#ifndef SA_NO_BOUNDS_CHECK
		//bounds check
		if( Index < pLevelData->Begin || Index > pLevelData->End
			||
			(Index & pLevelData->UsedMask) < ThisIndex
			||
			(Index & pLevelData->UsedMask) > (IndexType)(ThisIndex | ~pLevelData->UpperMask)	)
			throw OutOfBounds();
	#endif

		unsigned int NodeIndex = (unsigned int)((Index & pLevelData->Mask) >> pLevelData->Shamt);
		if(!pLevelData->Leaf)
		{	//This is a node
			if(!pNodes[NodeIndex])
			{
				//Return default value if the subtree doesn't exist
				return Default;
			}
			return pNodes[NodeIndex]->Read(Index);
		}
		else
		{	//This is a leaf
			//Return the reference
			#ifdef NODEPOOL_DEBUG
			if((ValueType &)pData[NodeIndex] == (ValueType)0xCDCDCDCDCDCDCDCD)
				throw runtime_error("SparseArray::Operator[]() pData == CD");
			#endif
			return pData[NodeIndex];
		}
	}

	/**************************************************************************\
	 *	Compact()
	 *
	 *	Compacts the tree to use as little memory as possible.
	 *	Removes unnecessary nodes.
	 *	A necessary node is one who's subtree contains a leaf with altered
	 *	data.
	 *	An unnecessary node is one who's subtree contains no leaves or nodes,
	 *	or whose leaves contain data which is equal to the default construction
	 *	of that data.
	 *	Three cases of data and their default constructions:
	 *		1. Intrinsic type (int, float, etc).
	 *			It will compare to (example) int() := 0.
	 *		2. Pointer. It will compare to (example) int*() := NULL.
	 *		3. Object. It will require the == operator to be defined. It will
	 *			compare: Data == Object(). Object() is the default construciton
	 *			of the object. If the comparison returns true to every element
	 *			in the node, the node will be compacted and all objects deleted.
	 *			Make sure there are no outstanding references to this object.
	 *			The best way to do this is to implement reference counting.
	 *
	 *	Returns true if the node is necessary (somewhere down the subtree it
	 *	points to modified data). False if the node is unnecessary (can be
	 *	safely deleted).
	\******/
	bool Compact()
	{
		bool Necessary = false;
		if(!pLevelData->Leaf)
		{	//This is a node
			//Run depth-first sub-tree node compaction
			//Check to see if this node is necessary
			//This node is necessary if any of the subtrees are necessary.
			bool SubNecessary;
			for(unsigned int i = 0; i < pLevelData->Size; i++)
			{
				if(pNodes[i])
				{
					if( !(SubNecessary = pNodes[i]->Compact()) )
					{
						//The subtree isn't necessary anymore
						delete pNodes[i];
						pNodes[i] = NULL;
					}
					Necessary |= SubNecessary;
				}
			}
		}
		else
		{	//This is a leaf
			//Check to see if this node is necessary
			//This node is necessary if any data is not the default value.
			for(unsigned int i = 0; i < pLevelData->Size; i++)
				if( !((ValueType &)pData[i] == Default) )
				{
					Necessary = true;
					break;
				}
		}
		return Necessary;
	}

	/**************************************************************************\
	 *	Clear( )
	 *	Clear( [in] Begin index, [in] End index )
	 *
	 *	This will remove all nodes and leaves which are contained within the
	 *	range Begin to End. Additionally, all values within that range that are
	 *	in a leaf that is only partially contained in that range will be reset
	 *	to the default value. This way, a future call to Compact() might be
	 *	able to remove the leaf. Clear() will not do a full reduction of nodes
	 *	if the range bisects any leaves. Call Compact() after Clear() to assure
	 *	a minimum number of allocated nodes.
	 *
	 *	Clear is relatively fast, because there are no compares to default
	 *	values, the entire tree is not traversed, and entire arrays
	 *	are not traversed. Clear directly targets the begin to end range.
	 *
	 *	Exceptions:
	 *		OutOfBounds	-Index not in range Begin to End
	\******/
	void Clear()
	{
		Clear(pLevelData->Begin, pLevelData->End);
	}
	void Clear(IndexType Begin, IndexType End)
	{
	#ifndef SA_NO_BOUNDS_CHECK
		//bounds check
		if( Begin < pLevelData->Begin || End > pLevelData->End || Begin > End )
 			throw OutOfBounds();
	#endif

		unsigned int i = (unsigned int)(( MAX(Begin & pLevelData->UsedMask, ThisIndex) & pLevelData->Mask ) >> pLevelData->Shamt),
			Final = (unsigned int)(( MIN(End & pLevelData->UsedMask, ThisIndex | pLevelData->Mask) & pLevelData->Mask ) >> pLevelData->Shamt);
		if(!pLevelData->Leaf)
		{	//This is a node
			//Loop through all the nodes which are touched by the clear range
			for(; i <= Final; i++)
			{
				if(pNodes[i])
				{
					if( pNodes[i]->ThisIndex >= (Begin & pNodes[i]->pLevelData->UsedMask)
						&&
						(pNodes[i]->ThisIndex | pNodes[i]->pLevelData->Mask) <= (End & pNodes[i]->pLevelData->UsedMask) )
					{
						//This node is completely contained within the range
						delete pNodes[i];
						pNodes[i] = NULL;
					}
					else
						//This node is only partially covered by the range
						pNodes[i]->Clear(Begin, End);
				}
			}
		}
		else
		{	//This is a leaf
			//Loop through all the data which are touched by the clear range
			for(; i <= Final; i++)
			{
				pData[i] = Default;
			}
		}
	}

	/**************************************************************************\
	 *	CopyTo( [in] Destination object)
	 *
	 *	The destination object can have a different indextype and valuetype.
	 *	This object's data will be copied into the destination object.
	 *	Only the data that has been accessed (exists and is not the default
	 *	value) will be copied.
	 *
	 *	This will achieve the same affect as a Reshape() function. The data
	 *	from this object will be reshaped into the structure of the destination
	 *	object.
	 *
	 *	This will also achieve the same affect as outputting this object to
	 *	a stream, and then inputting the stream to the destination object.
	 *	Except this is much faster.
	 *
	 *	If the destination is already populated, that data will remain. Only
	 *	data indices used by this object will be overwritten with the copied
	 *	values.
	 *
	 *	Warning: If this object has a don't care range, the index bits
	 *	associated with that range will be treated as "0"s when copied to the
	 *	same index location of the destination object.
	 *
	 *	Calls: operator[] on Dest
	 *
	 *	Exceptions: See operator[]
	\***********/
	template<class SourceIndexType, class SourceValueType>
	void CopyTo(SparseArray<SourceIndexType, SourceValueType> &Dest) const
	{
		if(!pLevelData->Leaf)
		{	//This is a node
			//Run depth-first sub-tree traversal.
			for(unsigned int i = 0; i < pLevelData->Size; i++)
				if(pNodes[i])
					pNodes[i]->CopyTo(Dest);
		}
		else
		{	//This is a leaf
			//Only copy data that isn't the default value.
			for(unsigned int i = 0; i < pLevelData->Size; i++)
				if( !((ValueType &)pData[i] == Default) )
					Dest[ThisIndex + ((IndexType)i << pLevelData->Shamt)] = pData[i];
		}
	}

	/**************************************************************************\
	 *	operator<<( [in-out] output stream, [in] SparseArray )
	 *	operator>>( [in-out] output stream )
	 *
	 *	Outputs the sparse array index space in the following format:
	 *	Index,Value\n
	 *	Index,Value\n
	 *	...
	 *
	 *	Does a depth-first traversal of the tree. The addresses will always be
	 *	in logical order (though not necessarily continuous).
	 *
	 *	Addresses for nodes and values that haven't been created will not be
	 *	output. Only those leaves which have been accessed will be output.
	 *
	 *	Value is whatever the result of ostream<<(ValueType) is. If
	 *	ValueType is an object, the user should define this function. If
	 *	ValueType is a pointer, the user should define the << operator
	 *	for the pointer type as well so that the address doesn't get written.
	 *	Objects may wish to output in binary format instead of ascii.
	 *
	 *	Warning: If this object has a don't care range, the index bits
	 *	associated with that range will be treated as "0"s when the address
	 *	is written to the output.
	 *
	 *	It is recommended that Compact() be called before this.
	 *	
	 *	A second SparseArray-oriented version is also provided.
	\******/
	friend ostream &operator<<(ostream &Output, SparseArray<IndexType, ValueType> &Source)
	{
		Source >> Output;
		return Output;
	}
	friend ostream &operator<<(ostream &Output, const SparseArray<IndexType, ValueType> &Source)
	{
		Source >> Output;
		return Output;
	}
	template<class OutType>
	SparseArray &operator>>(OutType &Output)
	{
		if(!pLevelData->Leaf)
		{	//This is a node
			//Run depth-first sub-tree traversal
			for(unsigned int i = 0; i < pLevelData->Size; i++)
			{
				if(pNodes[i])
					*pNodes[i] >> Output;
				else if(pLevelData->fPrintAll)
				{
					IndexType UsedMask = UsedBits();
					for(IndexType j = 0; j <= pLevelData->LowerMask; j++)
					{
						if(j & ~UsedMask)
							j = j + (j & ~UsedMask) - 1;
						else
						{
							char *EndLine = strrchr(pLevelData->sPostFix, '\n');
							if(!i && !j || EndLine != NULL)
								Output << pLevelData->sPreFix;
							if(EndLine != NULL)
								Output << (ThisIndex | ((IndexType)i << pLevelData->Shamt) | j);
							Output << pLevelData->sDelim << Default << pLevelData->sPostFix;
						}
					}
				}
			}
		}
		else
		{	//This is a leaf
			//Only output data that isn't the default value
			for(unsigned int i = 0; i < pLevelData->Size; i++)
				if( !((ValueType &)pData[i] == Default) || pLevelData->fPrintAll)
				{
					char *EndLine = strrchr(pLevelData->sPostFix, '\n');
					if(!i || EndLine != NULL)
						Output << pLevelData->sPreFix;
					if(EndLine != NULL)
						Output << (ThisIndex | ((IndexType)i << pLevelData->Shamt));
					Output << pLevelData->sDelim << (ValueType &)pData[i] << pLevelData->sPostFix;
				}
		}
		return *this;
	}
	template<class OutType>
	const SparseArray &operator>>(OutType &Output) const
	{
		if(!pLevelData->Leaf)
		{	//This is a node
			//Run depth-first sub-tree traversal
			for(unsigned int i = 0; i < pLevelData->Size; i++)
			{
				if(pNodes[i])
					(const SparseArray &)*pNodes[i] >> Output;
				else if(pLevelData->fPrintAll)
				{
					IndexType UsedMask = UsedBits();
					for(IndexType j = 0; j <= pLevelData->LowerMask; j++)
					{
						if(j & ~UsedMask)
							j = j + (j & ~UsedMask) - 1;
						else
						{
							char *EndLine = strrchr(pLevelData->sPostFix, '\n');
							if(!i && !j || EndLine != NULL)
								Output << pLevelData->sPreFix;
							if(EndLine != NULL)
								Output << (ThisIndex | ((IndexType)i << pLevelData->Shamt) | j);
							Output << pLevelData->sDelim << Default << pLevelData->sPostFix;
						}
					}
				}
			}
		}
		else
		{	//This is a leaf
			//Only output data that isn't the default value
			for(unsigned int i = 0; i < pLevelData->Size; i++)
				if( !((ValueType &)pData[i] == Default) || pLevelData->fPrintAll)
				{
					char *EndLine = strrchr(pLevelData->sPostFix, '\n');
					if(!i || EndLine != NULL)
						Output << pLevelData->sPreFix;
					if(EndLine != NULL)
						Output << (ThisIndex | ((IndexType)i << pLevelData->Shamt));
					Output << pLevelData->sDelim << (ValueType &)pData[i] << pLevelData->sPostFix;
				}
		}
		return *this;
	}

	/**************************************************************************\
	 *	operator>>( [in-out] input stream, [in-out] SparseArray )
	 *	operator<<( [in-out] input stream )
	 *
	 *	Inputs the sparse array index space in the following format:
	 *	Index,Value\n	//Comma "," can be any single-character delimiter
	 *	Index,Value\n	//that isn't parsed as part of Index
	 *	...
	 *
	 *	Value must be in a format that istream>>(ValueType) can understand.
	 *	If ValueType is an object, the user should define this function. If
	 *	ValueType is a pointer, the user should define the >> operator
	 *	for the pointer type as well so that the data isn't read as an address.
	 *	Objects may wish to input in binary format instead of ascii.
	 *
	 *	If the SparseArray is already populated, that data will remain. Only
	 *	data addressed in the input stream will be overwritten to the new
	 *	value.
	 *	
	 *	A second SparseArray-oriented version is also provided.
	 *
	 *	Calls: operator[]
	 *
	 *	Exceptions: See operator[]
	\******/
	friend istream &operator>>(istream &Input, SparseArray<IndexType, ValueType> &Dest)
	{
		Dest << Input;
		return Input;
	}
	template<class InType>
	SparseArray &operator<<(InType &Input)
	{
		IndexType Index;
		ValueType Value;
		while(true)
		{
			Input.ignore(strlen(pLevelData->sPreFix));
			Input >> Index;
			Input.ignore(strlen(pLevelData->sDelim)); //"," or "/t"
			Input >> Value;
			if(!Input.good())
				break;
			if(!(Value == Default))
				(*this)[Index] = Value;
			Input.ignore(strlen(pLevelData->sPostFix));
			//*NOTE: PostFix now holds \n, so the next line isnt necessary. But not sure if this works yet.
			//Input.ignore((unsigned int)-2 >> 1, '\n'); //ignore post data
		}
		return *this;
	}

	/**************************************************************************\
	 *	Begin()
	 *
	 *	Returns the start of the indexable range.
	\******/
	IndexType Begin() const
	{
		return pLevelData->Begin;
	}

	/**************************************************************************\
	 *	End()
	 *
	 *	Returns the end of the indexable range.
	\******/
	IndexType End() const
	{
		return pLevelData->End;
	}

	/**************************************************************************\
	 *	UsedIndex()
	 *
	 *	Returns a mask where used bits are 1's.
	\******/
	IndexType UsedBits() const
	{
		return (*pLevelDataVector)[pLevelDataVector->size()-1].UsedMask;
	}

	/**************************************************************************\
	 *	PrePostFix( [in] prefix, [in] delimiter, [in] postfix )
	 *
	 *	Sets the prefix and postfix strings which are printed before and after
	 *	the index,value in the serialized file.
	 *  Also sets the delimiter which is printed inbetween index and value
	 *  in the serialized file.
	 *	The only way to reliably read back data that has pre/delim/post fixes
	 *  is to set them identical when reading as when they were written.
	 *	sPostFix must end in a '\n' newline character for normal output:
	 *	sPreFix<Index>sDelim<Value>sPostFix\n
	 *	If sPostFix does not end in a newline, then the Index is not printed,
	 *	and all values are printed sequentially in one line separated by the
	 *	sDelim:
	 *	sPreFix<>sDelim<Value>sDelim...<Value>sPostFix
	 *	A file output in this alternate format cannot be used as stream input
	 *	to a SparseArray.
	 *
	 *	These store pointers to the strings, they don't copy them.
	\******/
	void PrePostFix(const char *sPreFix, const char *sDelim, const char *sPostFix)
	{
		if(!sPreFix)
			sPreFix = "";
		if(!sDelim)
			sDelim = "";
		if(!sPostFix)
			sPostFix = "";
		for(unsigned int i = 0; i < pLevelDataVector->size(); i++)
		{
			(*pLevelDataVector)[i].sPreFix = sPreFix;
			(*pLevelDataVector)[i].sDelim = sDelim;
			(*pLevelDataVector)[i].sPostFix = sPostFix;
		}
	}

	/**************************************************************************\
	 *	PrintAll( [in] true if print all )
	 *
	 *	Specifies to output all indices to the serialized file, including
	 *	ones that still hold the default value and ones that don't
	 *	even have nodes existing for them.
	\******/
	void PrintAll(bool fprintall)
	{
		for(unsigned int i = 0; i < pLevelDataVector->size(); i++)
			(*pLevelDataVector)[i].fPrintAll = fprintall;
	}

protected:

	/**************************************************************************\
	 *	SparseArray( [in] This Index, [in] level data vector, [in] level)
	 *
	 *	Overloaded constructor.
	 *
	 *	A node constructed with this is a sub-node, not a root node. It will
	 *	use the specified Level Data Vector for level information.
	 *	The tree level of this node is specified in Level.
	 *
	 *	Exceptions:
	 *		OutOfMemory -Not enough memory to allocate this node.
	\******/
	SparseArray(IndexType thisindex, vector<LevelData> *pLDV, unsigned int Level)
	{
		//This function should only be called by a valid SparseArray, so
		//it doesn't need to recheck parameters and templates.
		pNodes = NULL;
		pData = NULL;
		pLevelDataVector = NULL;
		pLevelData = NULL;

		//Copy data
		ThisIndex = thisindex;
		pLevelDataVector = pLDV;
		pLevelData = &(*pLevelDataVector)[Level];

		//Allocate the node array.
		if(!pLevelData->Leaf)
		{
			//This is a node
			if( !(pNodes = (NodeP *)NodeP::operator new[](pLevelData->Size * sizeof(NodeP))) )
				throw OutOfMemory();

			//Initialize array.
//			for(unsigned int i = 0; i < pLevelData->Size; i++)
//				pNodes[i] = NULL;
			memset(pNodes, NULL, pLevelData->Size * sizeof(NodeP));
		}
		else
		{
			//This is a leaf
			if( !(pData = (DataP *)DataP::operator new[](pLevelData->Size * sizeof(DataP))) )
				throw OutOfMemory();

			//Initialize data.
//			if(Default != 0)
				for(unsigned int i = 0; i < pLevelData->Size; i++)
					pData[i] = Default;
//			else
//				memset(pData, 0, pLevelData->Size * sizeof(DataP));
		}
	}

	/**************************************************************************\
	 *	Initialize( [in] Index Begin,  [in] Index End,
	 *		[in] Array of bit divisions )
	 *
	 *	See overloaded constructor for parameter description.
	 *
	 *	Initializes the class data and the array of level data.
	 *	Checks to make sure this is a valid type of SparseArray.
	 *	This is the only time and place where LevelData is created. All other
	 *	nodes have pointers to this data or copies of this data.
	 *	The node constructed with this is the root node of a tree.
	 *
	 *	Exceptions:
	 *		InvalidIndexType	-Index type is a signed or float type, it should be
	 *			an unsigned integral type.
	 *		InvalidShape	-Specified bit divisions don't cover the entire
	 *				index. The bit divisions must add up to the number of bits
	 *				in IndexType. Also, if end is less than begin, or if there are
	 *				no used bit ranges (entire range is unused).
	 *		OutOfMemory		-Not enough memory to allocate level data vector
	 *
	 *	If Initialize fails (throws an exception), then calling any other
	 *	functions on this instance will cause a segfault.
	 *	The only things you can safely do with a failed instance is:
	 *		delete it (~SparseArray)
	 *		reassign it to a valid instance (operator=) (unless InvalidIndexType)
	\******/
	void Initialize(IndexType begin, IndexType end, const vector<signed short> &BitDivisions)
	{
		MemoryManager.IncOwnerCount();
		pNodes = NULL;
		pData = NULL;
		pLevelDataVector = NULL;
		pLevelData = NULL;

		//Make sure the index type is an unsigned integral type
		IndexType SignTest1 = 0, SignTest2 = -1;
		if(SignTest1 > SignTest2)
			throw InvalidIndexType("SparseArray: IndexType not an unsigned integral type.");
		//Make sure the number of bits in the index type will fit
		//into an unsigned int
		if( sizeof(IndexType) > (1<<(sizeof(unsigned int)*BITS_PER_BYTE-1))/BITS_PER_BYTE )
			throw InvalidIndexType("SparseArray: IndexType holds more bits than can be counted in an unsigned int.");

		//Make sure the indexable range is valid
		if(begin > end)
			throw InvalidShape("SparseArray: Begin index after End index.");

		//Make sure the BitDivisions are valid
		unsigned int BitsRequired = BITS_PER_BYTE*sizeof(IndexType), BitsSpecified = 0, RootIndex, LeafIndex, i;
		bool RootFound = false;
		signed short CurrentBits;
		for(i = 0; i < BitDivisions.size(); i++)
		{
			CurrentBits = BitDivisions[i];
			BitsSpecified += abs(CurrentBits);
			if(CurrentBits == 0)	//meaningless division
				throw InvalidShape("SparseArray: Index bit division is 0.");
			if(CurrentBits > MaxBitRange)	//too big to be physically indexed
				throw InvalidShape("SparseArray: Bit range is too large to be physically indexable.");
			if(CurrentBits > SA_MAX_ARRAY_BITS)	//too big for new nodepool implementation
				throw InvalidShape("SparseArray: Bit range is too large to use pre-allocated buffers.");
			if(BitsSpecified > BitsRequired)	//too many bits specified
				throw InvalidShape("SparseArray: Index bit divisions specify more bits than there are in the index.");

			//Find root and leaf levels while we're at it
			if(CurrentBits > 0)
			{
				//In the end, this will point to the first used bit range
				if(!RootFound)
				{
					RootIndex = i;
					RootFound = true;
				}
				//In the end, this will point to the last used bit range
				LeafIndex = i;
			}
		}
		if(BitsRequired != BitsSpecified)
			throw InvalidShape("SparseArray: Index bit divisions specify fewer bits than there are in the index.");
		if(!RootFound)
			throw InvalidShape("SparseArray: No used bit range specified.");

		//Initialize data
		ThisIndex = 0;

		//Build the Level information
		unsigned int BitAccumulator = 0;
		unsigned short AbsCurrentBits;
		IndexType MaskAccumulator = -1, UsedMask = 0;
		if( !(pLevelDataVector = new vector<LevelData>) )
			throw OutOfMemory();
		for(i = 0; i < BitDivisions.size(); i++)
		{
			CurrentBits = BitDivisions[i];
			AbsCurrentBits = abs(CurrentBits);
			BitAccumulator += AbsCurrentBits;
			if(CurrentBits > 0)
				UsedMask |= /*mask*/ MaskAccumulator ^ (MaskAccumulator >> AbsCurrentBits);
			
			pLevelDataVector->push_back( LevelData(
				i,									//level #
				(unsigned int)1 << AbsCurrentBits,	//indexability
				begin,								//begin
				end,								//end
				MaskAccumulator ^ (MaskAccumulator >> AbsCurrentBits),	//mask
				~MaskAccumulator,					//upper mask
				MaskAccumulator >> AbsCurrentBits,	//lower mask
				AbsCurrentBits,						//num bits
				BitsRequired - BitAccumulator,		//used bits
				UsedMask,							//used mask
				CurrentBits < 0,					//skip
				i == RootIndex,						//root
				i == LeafIndex	)	);				//leaf

			MaskAccumulator >>= AbsCurrentBits;

			if(CurrentBits > 0)
			{
				unsigned int Length = (unsigned int)1 << AbsCurrentBits;
				if(i != LeafIndex)
				{
					if(Length <= 2)
						NodeP::MemoryManager2.IncOwnerCount();
					else if(Length <= 4)
						NodeP::MemoryManager4.IncOwnerCount();
					else if(Length <= 8)
						NodeP::MemoryManager8.IncOwnerCount();
					else if(Length <= 16)
						NodeP::MemoryManager16.IncOwnerCount();
					else if(Length <= 32)
						NodeP::MemoryManager32.IncOwnerCount();
					else if(Length <= 64)
						NodeP::MemoryManager64.IncOwnerCount();
					else if(Length <= 128)
						NodeP::MemoryManager128.IncOwnerCount();
					else if(Length <= 256)
						NodeP::MemoryManager256.IncOwnerCount();
				}
				else
				{
					if(Length <= 2)
						DataP::MemoryManager2.IncOwnerCount();
					else if(Length <= 4)
						DataP::MemoryManager4.IncOwnerCount();
					else if(Length <= 8)
						DataP::MemoryManager8.IncOwnerCount();
					else if(Length <= 16)
						DataP::MemoryManager16.IncOwnerCount();
					else if(Length <= 32)
						DataP::MemoryManager32.IncOwnerCount();
					else if(Length <= 64)
						DataP::MemoryManager64.IncOwnerCount();
					else if(Length <= 128)
						DataP::MemoryManager128.IncOwnerCount();
					else if(Length <= 256)
						DataP::MemoryManager256.IncOwnerCount();
				}
			}
		}

		//Assign the level for the root node.
		pLevelData = &(*pLevelDataVector)[RootIndex];

		//Allocate the node array.
		if(!pLevelData->Leaf)
		{
			//This is a node
			if( !(pNodes = (NodeP *)NodeP::operator new[](pLevelData->Size * sizeof(NodeP))) )
				throw OutOfMemory();

			//Initialize array.
//			for(unsigned int i = 0; i < pLevelData->Size; i++)
//				pNodes[i] = NULL;
			memset(pNodes, NULL, pLevelData->Size * sizeof(NodeP));
		}
		else
		{
			//This is a leaf
			if( !(pData = (DataP *)DataP::operator new[](pLevelData->Size * sizeof(DataP))) )
				throw OutOfMemory();

			//Initialize data.
//			if(Default != 0)
				for(unsigned int i = 0; i < pLevelData->Size; i++)
					pData[i] = Default;
//			else
//				memset(pData, 0, pLevelData->Size * sizeof(DataP));
		}
	}

	/**************************************************************************\
	 *	Copy( [in] object to copy, [in] Level Data Vector )
	 *
	 *	Creates a complete copy of the input object.
	 *	If Level Data Vector is NULL, this is the root of a tree, and a new
	 *	LDV is created. Otherwise, this is a sub-node, and it uses the given
	 *	LDV.
	 *
	 *	Calls: CopyTo() on Source
	 *
	 *	Exceptions:
	 *		InvalidIndexType	-Index type is a signed or float type, it should be
	 *			an unsigned integral type.
	 *		OutOfMemory		-Not enough memory to allocate more nodes.
	\******/
	template<class SourceValueType>
	void Copy(const SparseArray<IndexType, SourceValueType> &Source, vector<LevelData> *pLDV = NULL)
	{
		pNodes = NULL;
		pData = NULL;
		pLevelDataVector = NULL;
		pLevelData = NULL;

		//Make sure the index type is an unsigned integral type
		IndexType SignTest1 = 0, SignTest2 = -1;
		if(SignTest1 > SignTest2)
			throw InvalidIndexType("SparseArray: IndexType not an unsigned integral type.");
		//Make sure the number of bits in the index type will fit
		//into an unsigned int
		if( sizeof(IndexType) > (1<<(sizeof(unsigned int)*BITS_PER_BYTE-1))/BITS_PER_BYTE )
			throw InvalidIndexType("SparseArray: IndexType holds more bits than can be counted in an unsigned int.");

		//We assume that the source is a valid, uncorrupted SparseArray

		//Copy data
		ThisIndex = Source.ThisIndex;

		//Use the given Level Data Vector, or make a copy of it the sources.
		if(pLDV)
			pLevelDataVector = pLDV;
		else if( !(pLevelDataVector = new vector<LevelData>(*Source.pLevelDataVector)) )
			throw OutOfMemory();
		pLevelData = &(*pLevelDataVector)[Source.pLevelData->Level];
		if(!pLDV)
		{
			//Force it to be a root node
			pLevelData->Root = true;
			MemoryManager.IncOwnerCount();
			for(typename vector<LevelData>::iterator pLDVIter = pLevelDataVector->begin(); pLDVIter != pLevelDataVector->end(); pLDVIter++)
			{
				if(!pLDVIter->Skip)
				{
					unsigned int Length = pLDVIter->Size;
					if(!pLDVIter->Leaf)
					{
						if(Length <= 2)
							NodeP::MemoryManager2.IncOwnerCount();
						else if(Length <= 4)
							NodeP::MemoryManager4.IncOwnerCount();
						else if(Length <= 8)
							NodeP::MemoryManager8.IncOwnerCount();
						else if(Length <= 16)
							NodeP::MemoryManager16.IncOwnerCount();
						else if(Length <= 32)
							NodeP::MemoryManager32.IncOwnerCount();
						else if(Length <= 64)
							NodeP::MemoryManager64.IncOwnerCount();
						else if(Length <= 128)
							NodeP::MemoryManager128.IncOwnerCount();
						else if(Length <= 256)
							NodeP::MemoryManager256.IncOwnerCount();
					}
					else
					{
						if(Length <= 2)
							DataP::MemoryManager2.IncOwnerCount();
						else if(Length <= 4)
							DataP::MemoryManager4.IncOwnerCount();
						else if(Length <= 8)
							DataP::MemoryManager8.IncOwnerCount();
						else if(Length <= 16)
							DataP::MemoryManager16.IncOwnerCount();
						else if(Length <= 32)
							DataP::MemoryManager32.IncOwnerCount();
						else if(Length <= 64)
							DataP::MemoryManager64.IncOwnerCount();
						else if(Length <= 128)
							DataP::MemoryManager128.IncOwnerCount();
						else if(Length <= 256)
							DataP::MemoryManager256.IncOwnerCount();
					}
				}
			}
		}

		//Allocate the node array.
		if(!pLevelData->Leaf)
		{
			//This is a node
			if( !(pNodes = (NodeP *)NodeP::operator new[](pLevelData->Size * sizeof(NodeP))) )
				throw OutOfMemory();

			//Initialize array.
//			for(unsigned int i = 0; i < pLevelData->Size; i++)
//				pNodes[i] = NULL;
			memset(pNodes, NULL, pLevelData->Size * sizeof(NodeP));
		}
		else
		{
			//This is a leaf
			if( !(pData = (DataP *)DataP::operator new[](pLevelData->Size * sizeof(DataP))) )
				throw OutOfMemory();

			//Initialize data.
//			if(Default != 0)
				for(unsigned int i = 0; i < pLevelData->Size; i++)
					pData[i] = Default;
//			else
//				memset(pData, 0, pLevelData->Size * sizeof(DataP));
		}

		//Copy the tree
		Source.CopyTo(*this);
	}

	/**************************************************************************\
	 *	Destroy()
	 *
	 *	Deletes all the data associated with this tree.
	 *	Should only be used when this instance is about to be destroyed
	 *	or reassigned.
	\******/
	void Destroy()
	{
		if(!pLevelData->Leaf)
		{
			for(unsigned int i = 0; i < pLevelData->Size; i++)
				if(pNodes[i])
					delete pNodes[i];
			NodeP::operator delete [](pNodes, (pLevelData->Size * sizeof(NodeP)));
		}
		else
		{
			DataP::operator delete [](pData, (pLevelData->Size * sizeof(DataP)));
		}

		if(pLevelData)
			if(pLevelData->Root)
			{
				MemoryManager.DecOwnerCount();
				for(typename vector<LevelData>::iterator pLDVIter = pLevelDataVector->begin(); pLDVIter != pLevelDataVector->end(); pLDVIter++)
				{
					if(!pLDVIter->Skip)
					{
						unsigned int Length = pLDVIter->Size;
						if(!pLDVIter->Leaf)
						{
							if(Length <= 2)
								NodeP::MemoryManager2.DecOwnerCount();
							else if(Length <= 4)
								NodeP::MemoryManager4.DecOwnerCount();
							else if(Length <= 8)
								NodeP::MemoryManager8.DecOwnerCount();
							else if(Length <= 16)
								NodeP::MemoryManager16.DecOwnerCount();
							else if(Length <= 32)
								NodeP::MemoryManager32.DecOwnerCount();
							else if(Length <= 64)
								NodeP::MemoryManager64.DecOwnerCount();
							else if(Length <= 128)
								NodeP::MemoryManager128.DecOwnerCount();
							else if(Length <= 256)
								NodeP::MemoryManager256.DecOwnerCount();
						}
						else
						{
							if(Length <= 2)
								DataP::MemoryManager2.DecOwnerCount();
							else if(Length <= 4)
								DataP::MemoryManager4.DecOwnerCount();
							else if(Length <= 8)
								DataP::MemoryManager8.DecOwnerCount();
							else if(Length <= 16)
								DataP::MemoryManager16.DecOwnerCount();
							else if(Length <= 32)
								DataP::MemoryManager32.DecOwnerCount();
							else if(Length <= 64)
								DataP::MemoryManager64.DecOwnerCount();
							else if(Length <= 128)
								DataP::MemoryManager128.DecOwnerCount();
							else if(Length <= 256)
								DataP::MemoryManager256.DecOwnerCount();
						}
					}
				}
				//Only the root node can delete this
				delete pLevelDataVector;
			}
	}

	/**************************************************************************\
	 *	operator==( [in] object to compare )
	 *
	 *	This is only used for comparing to the default construction of
	 *	SparseArray. This is only implemented so that SparseArray can
	 *	be the ValueType for another SparseArray. The only time SparseArray
	 *	internally calls operator== is when comparing to Default.
	 *
	 *	If this object and the compare object are empty, regardless of shape,
	 *	it returns true. Otherwise, it returns false.
	\******/
	bool operator==(const SparseArray &Other) const
	{
		if(!pLevelData->Leaf)
		{
			//this is a node
			for(unsigned int i = 0; i < pLevelData->Size; i++)
				if(pNodes[i])
					return false;
		}
		else
		{
			//this is a leaf
			for(unsigned int i = 0; i < pLevelData->Size; i++)
				if( !((ValueType &)pData[i] == Default) )
					return false;
		}
		return true;
	}

	//The maximum bits for a used range. This is limited by the max size
	//of a physical memory array.
	static const signed short MaxBitRange;

	//The root of the memory subtree.
	NodeP *pNodes;
	DataP *pData;
	//The index that leads to this node (irrelevant lower bits are zeros)
	IndexType ThisIndex;
	//The array of data for each level of the tree.
	vector<LevelData> *pLevelDataVector;
	//The level data for this level
	LevelData *pLevelData;
	//A default construction of ValueType
	static const ValueType Default;

public:
	/*========================================================================*\
	 *	Exceptions
	 *
	 *	These are thrown by the SparseArray functions.
	 *
	 *	You can catch the base class exception, and get a message through
	 *	the what() function.
	\*========================================================================*/

	class InvalidIndexType : public runtime_error
	{
	public:
		InvalidIndexType(const char *Str) : runtime_error(Str)	{	}
	};

	class InvalidShape : public runtime_error
	{
	public:
		InvalidShape(const char *Str) : runtime_error(Str)	{	}
	};

	class OutOfBounds : public runtime_error
	{
	public:
		OutOfBounds() : runtime_error("SparseArray: Index out of bounds.")	{	}
	};

	class OutOfMemory : public runtime_error
	{
	public:
		OutOfMemory() : runtime_error("SparseArray: Out of memory. Indexed access not sparse enough? Not compacting often enough?")	{	}
	};

	//Decrease the allocation/deallocation overhead of the segment list by providing a segment pool to enable reuse of segment memory.
	static NodePool<SparseArray> MemoryManager;
	//Control alloc/dealloc
	static void *operator new(size_t AllocBlock)
	{ 
		return (void *)MemoryManager.Pop();
	}
	static void operator delete(void *pOldSA)
	{
		MemoryManager.Push((SparseArray *)pOldSA);
	}
};

/*||\\--//||\\--//||\\--//||\\--- Data ---//||\\--//||\\--//||\\--//||\\--//||\\
\\||//--\\||//--\\||//--\\||//----++++----\\||//--\\||//--\\||//--\\||//--\\||*/

template<class IndexType, class ValueType>
const ValueType SparseArray<IndexType, ValueType>::Default = ValueType();
template<class IndexType, class ValueType>
const signed short SparseArray<IndexType, ValueType>::MaxBitRange = sizeof(int)*BITS_PER_BYTE-1;
template<class IndexType, class ValueType>
NodePool< SparseArray<IndexType, ValueType> > SparseArray<IndexType, ValueType>::MemoryManager(SPARSEARRAY_MM_INIT);

template<class IndexType, class ValueType>
NodePool< typename SparseArray<IndexType, ValueType>::NodeP::One > SparseArray<IndexType, ValueType>::NodeP::MemoryManager2(SPARSEARRAYNODE_MM_INIT);
template<class IndexType, class ValueType>
NodePool< typename SparseArray<IndexType, ValueType>::NodeP::Two > SparseArray<IndexType, ValueType>::NodeP::MemoryManager4(SPARSEARRAYNODE_MM_INIT);
template<class IndexType, class ValueType>
NodePool< typename SparseArray<IndexType, ValueType>::NodeP::Three > SparseArray<IndexType, ValueType>::NodeP::MemoryManager8(SPARSEARRAYNODE_MM_INIT);
template<class IndexType, class ValueType>
NodePool< typename SparseArray<IndexType, ValueType>::NodeP::Four > SparseArray<IndexType, ValueType>::NodeP::MemoryManager16(SPARSEARRAYNODE_MM_INIT);
template<class IndexType, class ValueType>
NodePool< typename SparseArray<IndexType, ValueType>::NodeP::Five > SparseArray<IndexType, ValueType>::NodeP::MemoryManager32(SPARSEARRAYNODE_MM_INIT);
template<class IndexType, class ValueType>
NodePool< typename SparseArray<IndexType, ValueType>::NodeP::Six > SparseArray<IndexType, ValueType>::NodeP::MemoryManager64(SPARSEARRAYNODE_MM_INIT);
template<class IndexType, class ValueType>
NodePool< typename SparseArray<IndexType, ValueType>::NodeP::Seven > SparseArray<IndexType, ValueType>::NodeP::MemoryManager128(SPARSEARRAYNODE_MM_INIT);
template<class IndexType, class ValueType>
NodePool< typename SparseArray<IndexType, ValueType>::NodeP::Eight > SparseArray<IndexType, ValueType>::NodeP::MemoryManager256(SPARSEARRAYNODE_MM_INIT);

template<class IndexType, class ValueType>
NodePool< typename SparseArray<IndexType, ValueType>::DataP::One > SparseArray<IndexType, ValueType>::DataP::MemoryManager2(SPARSEARRAYNODE_MM_INIT);
template<class IndexType, class ValueType>
NodePool< typename SparseArray<IndexType, ValueType>::DataP::Two > SparseArray<IndexType, ValueType>::DataP::MemoryManager4(SPARSEARRAYNODE_MM_INIT);
template<class IndexType, class ValueType>
NodePool< typename SparseArray<IndexType, ValueType>::DataP::Three > SparseArray<IndexType, ValueType>::DataP::MemoryManager8(SPARSEARRAYNODE_MM_INIT);
template<class IndexType, class ValueType>
NodePool< typename SparseArray<IndexType, ValueType>::DataP::Four > SparseArray<IndexType, ValueType>::DataP::MemoryManager16(SPARSEARRAYNODE_MM_INIT);
template<class IndexType, class ValueType>
NodePool< typename SparseArray<IndexType, ValueType>::DataP::Five > SparseArray<IndexType, ValueType>::DataP::MemoryManager32(SPARSEARRAYNODE_MM_INIT);
template<class IndexType, class ValueType>
NodePool< typename SparseArray<IndexType, ValueType>::DataP::Six > SparseArray<IndexType, ValueType>::DataP::MemoryManager64(SPARSEARRAYNODE_MM_INIT);
template<class IndexType, class ValueType>
NodePool< typename SparseArray<IndexType, ValueType>::DataP::Seven > SparseArray<IndexType, ValueType>::DataP::MemoryManager128(SPARSEARRAYNODE_MM_INIT);
template<class IndexType, class ValueType>
NodePool< typename SparseArray<IndexType, ValueType>::DataP::Eight > SparseArray<IndexType, ValueType>::DataP::MemoryManager256(SPARSEARRAYNODE_MM_INIT);

#undef SPARSEARRAY_MM_INIT
#undef SPARSEARRAYNODE_MM_INIT

}	//namespace JMT

#endif
