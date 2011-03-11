// Copyright 2002 Ashley Wise
// JediMasterThrash@comcast.net

#ifndef JMTLIB_H
#define JMTLIB_H

#pragma warning (disable:4786)
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include "JMTSys.h"
#include "SynchLib.h"

using namespace std;


namespace JMT	{

//**********************//
//*** General Macros ***//
//**********************//

	#ifndef BITS_PER_BYTE
		#define BITS_PER_BYTE 8
	#endif

	#ifndef MAX
		#define MAX(a,b)    (((a) > (b)) ? (a) : (b))
	#endif
	#ifndef MIN
		#define MIN(a,b)    (((a) < (b)) ? (a) : (b))
	#endif

	/**************************************************************************\
		EndianCheck( )
		
		Tests to see if the platform is little or big endian.

		Returns true for little endian, false for big endian.
	\******/
	bool EndianCheck();

	/**************************************************************************\
		ToLower( [in] string )
		
		It will convert the string to lowercase.
		Text that appears within C-style strings and character constants
		will be unaffected.
		For instance:
		HeLLo "World \"Dude\"" '!';
		Will be converted to:
		hello "World \"Dude\"" '!';

		It returns the result.
	\******/
	string ToLower(const string &);

//**************************************************************//
//*** 64-bit datatype stuff for cross-platform compatibility ***//
//**************************************************************//

	//*NOTE: GCC is lame and requies an 'll' suffix on 64-bit constants.
	//Since MSVC is lame and uses an incompatible syntax, just make with
	//-fpermissive flag

	//*NOTE: MSVC doesn't let "unsigned" work with typdef'd int64.
	#if defined _MSC_VER
		typedef __int64 int64;
		typedef unsigned __int64 uint64;
	#elif defined GPLUSPLUS
		typedef long long int64;
		typedef unsigned long long uint64;
	#else
		#error "Only MSVC and GCC Compilers Supported"
	#endif

	//*NOTE: Conversion from unsigned __int64 to double not implemented in MSVC
	#define UINT64_TO_DOUBLE(Num) ( ((double)((int64)(Num >> 1))) * 2 + (double)((int64)(Num & 1)) )

	typedef union Real_t
	{
		double DBL;
		uint64 UI64;
	} Real;

	typedef union ByteData_t
	{
		uint64 UI64;
		unsigned char Bytes[sizeof(uint64)];
	} ByteData;

//********************************************************//
//*** Miscellaneous cross-platform compatibility stuff ***//
//********************************************************//

	//*NOTE: Some bug in MSVC causes dynamic_cast to throw an exception
	//everywhere it's used on polymorphic classes, so reinterpret case is
	//used instead.

	//*NOTE: MSVC doesn't allow the typename in a template argument
	#if defined _MSC_VER
		#define STDTYPENAME
	#elif defined GPLUSPLUS
		#define STDTYPENAME typename
	#else
		#error "Only MSVC and GCC Compilers Supported"
	#endif

	#if defined UNIX_BUILD
		const char PATH_MARKER = '/';
	#elif defined MAC_BUILD
		const char PATH_MARKER = ':';
	#else
		const char PATH_MARKER = '\\';
	#endif

	//*NOTE: For some reason gcc uses the non-standard strcasecmp instead of
	//stricmp
	#if defined GPLUSPLUS && !defined stricmp
		#define stricmp strcasecmp
	#endif

//**************************//
//*** Filename Utilities ***//
//**************************//

	struct FileName
	{
		FileName();
		FileName(const string &);
		FileName &operator=(const string &);
		FileName &Set(const string &);
		string Full, Name, Path, Bare, Ext;
	};

	/**************************************************************************\
		CreateStandardPath( [in] path+file name )
		
		Changes all "\" to "/" in the path.
	\******/
	string CreateStandardPath(const string &);

	/**************************************************************************\
		CreateCompactFileName( [in] path+file name )
		
		Removes any unnessassary "blah/../" in the file.
	\******/
	string CreateCompactFileName(const string &);

	/**************************************************************************\
		CreateFileNameRelative( [in] current path+file name,
			[in] path+file name relative to current)
		
		Current path+file name is a path relative to the working directory
		or an absolute path.

		The second path+file name is a path relative to the "current" or
		an absolute path

		Returns the most compact path+file name that points to the file in
		the second parameter using the same relativeness of the file in the
		first parameter.

		If the second name is an absolute path, it uses that.
		Removes any unnessassary "blah/../" in the files.
	\******/
	string CreateFileNameRelative(const string &, const string &);

	/**************************************************************************\
		CreateRelativeFileName( [in] current path+file name 1,
			[in] current path+file name 2)
		
		The first path+file name is a path relative to the working
		directory or an absolute path.

		The second path+file name is a path in the same relativeness as
		the first.

		Returns the most compact path+file name that points to the file in
		the second parameter relative to the file in the first parameter.

		Removes any unnessassary "blah/../" in the files.
	\******/
	string CreateRelativeFileName(const string &, const string &);

//*************************//
//*** LEXER/TOKEN Stuff ***//
//*************************//

	//The working directory for the current operation
	extern string sWorkingDir;
	//A list of filenames being worked on
	extern vector<string> InputList;

	//The datatype for file locations <InputList index, line number>
	typedef vector< pair<unsigned int, unsigned int> > LocationVector;
	//*NOTE: Bug in GCC causes weird errors if LocationVector() is used for
	//parameter to program here (even though it works fine in other places).
	extern const LocationVector NullLocationStack;

	//Different numerical bases
	enum BaseEnum {Bin, Oct, Dec, Hex};

	//These are the different types of callback messages
	enum MessageEnum {Info, Warning, Error, Fatal, Exception, Breakpoint, Check};
	//Prototype for outputting a message related to an InputList file
	//(message type, message, file location)
	#define CallBackFunction bool CallBack(MessageEnum, const string &, const LocationVector &)

//**************************************************//
//*** Stream operators for bytes and 64-bit data ***//
//**************************************************//

	#ifndef NO_CHAR_NUMBERS
	/**************************************************************************\
		operator>>( [in] input stream, [out] byte )
		operator<<( [in-out] output stream, [in] byte )

		If you want to use unsigned/signed chars as BYTE data instead of
		character data, you'll run into some problems with stream I/O.
		These functions solve the problem with BYTEs being read as characters
		(ie, without parsing as a number). If you wish to use unsigned
		or signed chars as characters for I/O, then remove these functions.
	\******/
	ostream &operator<<(ostream &Output, unsigned char UC);
	ostream &operator<<(ostream &Output, signed char SC);
	istream &operator>>(istream &Input, unsigned char &UC);
	istream &operator>>(istream &Input, signed char &SC);
	#endif

	#if (_MSC_VER < 1300) && !defined GPLUSPLUS
	/**************************************************************************\
		operator>>( [in-out] input stream, [out] 64-bit )
		operator<<( [in-out] output stream, [in] 64-bit )

		There is no standard C++ stream support for 64-bit integers. In order
		to use 64-bit integers in stream I/O, you must use these overloads.
		As long as you're in the JMT namespace, these overloads will be called
		automatically as part of a stream statement when necessary.
	\******/
	ostream &operator<<(ostream &Output, uint64 UI64);
	ostream &operator<<(ostream &Output, int64 SI64);
	istream &operator>>(istream &Input, uint64 &UI64);
	istream &operator>>(istream &Input, int64 &SI64);
	#endif
	/**************************************************************************\
		LexNumber( [in] input stream )

		This is used by the operator>>(istream, 64-bit) functions.

		scanf only works on pre-obtained buffers, or the console input.
		A stream could be from anywhere, however, and there's no way to
		extract *only* the required 64-bit number without doing some lexing.
		This lexing is equivalent to what's needed to build the number anyway,
		so instead of using a scanf, I'm using this function.
	\******/
	int64 LexNumber(istream &);

//**********************//
//*** Memory Manager ***//
//**********************//

	/*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*\
		NodePool

		This is a pool of nodes. This is used as a storage location for
		the memory of unused nodes.

		malloc and free are very slow functions. If you need dynamic memory
		in a performance intensive section of code, and you're going to be
		constantly allocating and deallocating the same memory structure,
		it makes more sense to just store the unused piece of memory somewhere
		until it can be reused again.

		NodePool can be a pool of any data structure or class. The only
		requirement is that it contain a pNext pointer to another Node.
		This way the pool can be maintained as a singly linked list.

		If you want to free a Node, call Push(Node). If you want to grab
		a node from the pool, call Pop(). If the pool is empty, Pop()
		will call malloc to get a node. If the pool is too big, Push(Node)
		will call free on some nodes.

		An instance of NodePool should be shared amongst all instances of
		the class that uses it. Each instance that uses the NodePool should
		increment the OwnerCount so that the pool knows how many potential
		node users exist. Similarly, each instance should decrement the
		OwnerCount when they are deleted.

		The class should overload the new and delete operators for the NodeType.
		This assures that a popped node, regardless of whether it came from
		the pool or from malloc, will have the Node's constructor re-run on
		the Node memory.

		If MULTITHREADED is defined, the NodePool will be threadsafe.
		It uses SynchLib for threadsafe synchronization.
	\*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*/
	template<class NodeType>
	class NodePool
	{
	public:
		//Constructor. Initializes the mutex
		NodePool(unsigned int nodetoownerratio = 0x100, unsigned int initialpool = 0x100, bool fnail = false)
		{
			SYNCH_CREATE_LOCK(Mutex)
			pNodePool = NULL;
			NodeCount = 0;
	#ifdef _DEBUG
			NewCount = DeleteCount = MallocCount = FreeCount = 0;
	#endif
			OwnerCount = 0;
			NodeToOwnerRatio = nodetoownerratio;
			InitialPool = initialpool;
			fNail = fnail;
			//Create an initial pool
			NodeType *pTempNode;
			while(NodeCount < InitialPool)
			{
	#ifdef _DEBUG
				MallocCount++;
				DeleteCount--;
	#endif
				pTempNode = (NodeType *)malloc( MAX(sizeof(NodeType), sizeof(NodePointer)) );
	#ifdef NODEPOOL_DEBUG
				NodePoolInsIter NPI = NPM.insert(make_pair((void *)pTempNode, NPPop));
				if(NPI.second = false)
				{
					sprintf(sMessageBuffer, "NodePool::NodePool(): Address %X already allocated as %s", pTempNode, NodePoolStates[NPI.first->second]);
					throw runtime_error(sMessageBuffer);
				}
	#endif
				if(fNail)
					NAIL_MEMORY(pTempNode, MAX(sizeof(NodeType), sizeof(NodePointer)))
				Push(pTempNode);
			}
		}

		//Destructor. Closes the mutex.
		~NodePool()
		{
			NodeType *pTempNode;
			while(NodeCount)
			{
	#ifdef _DEBUG
				FreeCount++;
	#endif
				pTempNode = Pop();
				if(fNail)
					UNNAIL_MEMORY(pTempNode, MAX(sizeof(NodeType), sizeof(NodePointer)))
	#ifdef NODEPOOL_DEBUG
				NodePoolIter NPI = NPM.find(pTempNode);
				if(NPI == NPM.end())
				{
					sprintf(sMessageBuffer, "NodePool::~NodePool(): Address %X not previously allocated", pTempNode);
					throw runtime_error(sMessageBuffer);
				}
				else if(NPI->second != NPPop)
				{
					sprintf(sMessageBuffer, "NodePool::~NodePool(): Address %X allocated as %s, should be %s", pTempNode, NodePoolStates[NPI->second], NodePoolStates[NPPush]);
					throw runtime_error(sMessageBuffer);
				}
				NPI->second = NPFree;
	#endif
				free(pTempNode);
			}
			SYNCH_CLOSE_LOCK(Mutex)
		}
		
		/**********************************************************************\
			Push( [in] old node )
		
			Adds the old node to the node pool.
			If there are too many nodes in the pool, it frees some.
		\******/
		void Push(NodeType *poldnode)
		{
			NodePointer *pOldNode = (NodePointer *)poldnode;
			if(!pOldNode)
				return;
			
	#ifdef NODEPOOL_DEBUG
			memset(pOldNode, 0xCD, MAX(sizeof(NodeType), sizeof(NodePointer)));
	#endif
	#ifdef NODEPOOL_DEBUG
			NodePoolIter NPI = NPM.find(pOldNode);
			if(NPI == NPM.end())
			{
				sprintf(sMessageBuffer, "NodePool::Push(): Address %X not previously allocated", pOldNode);
				throw runtime_error(sMessageBuffer);
			}
			else if(NPI->second != NPPop)
			{
				sprintf(sMessageBuffer, "NodePool::Push(): Address %X allocated as %s, should be %s", pOldNode, NodePoolStates[NPI->second], NodePoolStates[NPPop]);
				throw runtime_error(sMessageBuffer);
			}
			NPI->second = NPPush;
	#endif

			SYNCH_ACQUIRE_LOCK(Mutex)

			//Add the segment to the list
			pOldNode->pNext = pNodePool;
			pNodePool = pOldNode;

			NodeCount++;
	#ifdef _DEBUG
			DeleteCount++;
	#endif

			//Free up some memory
			NodePointer *pTemp;
			while(NodeCount > NodeToOwnerRatio * OwnerCount * 2 + InitialPool)
			{
				pTemp = pNodePool;
				pNodePool = pNodePool->pNext;
				if(fNail)
					UNNAIL_MEMORY(pTemp, MAX(sizeof(NodeType), sizeof(NodePointer)))
	#ifdef NODEPOOL_DEBUG
				NodePoolIter NPI = NPM.find(pTemp);
				if(NPI == NPM.end())
				{
					sprintf(sMessageBuffer, "NodePool::Push(): Address %X not previously allocated", pTemp);
					throw runtime_error(sMessageBuffer);
				}
				else if(NPI->second != NPPush)
				{
					sprintf(sMessageBuffer, "NodePool::Push(): Address %X allocated as %s, should be %s", pTemp, NodePoolStates[NPI->second], NodePoolStates[NPPush]);
					throw runtime_error(sMessageBuffer);
				}
				NPI->second = NPFree;
	#endif
				free(pTemp);
				NodeCount--;
	#ifdef _DEBUG
				FreeCount++;
	#endif
			}
			SYNCH_RELEASE_LOCK(Mutex)
		}

		/**********************************************************************\
			Pop( )
		
			Returns a node off the head of the pool..
			If there aren't any nodes left, it allocates a new one.
		\******/
		NodeType *Pop()
		{
			NodePointer *pTemp;

			SYNCH_ACQUIRE_LOCK(Mutex)
	#ifdef _DEBUG
			NewCount++;
	#endif
			if(pNodePool)
			{
				//Grab a segment off the list
				pTemp = pNodePool;
				pNodePool = pNodePool->pNext;
				NodeCount--;
				SYNCH_RELEASE_LOCK(Mutex)
	#ifdef NODEPOOL_DEBUG
				NodePoolIter NPI = NPM.find(pTemp);
				if(NPI == NPM.end())
				{
					sprintf(sMessageBuffer, "NodePool::Pop(): Address %X not previously allocated", pTemp);
					throw runtime_error(sMessageBuffer);
				}
				else if(NPI->second != NPPush)
				{
					sprintf(sMessageBuffer, "NodePool::Pop(): Address %X allocated as %s, should be %s", pTemp, NodePoolStates[NPI->second], NodePoolStates[NPPush]);
					throw runtime_error(sMessageBuffer);
				}
				NPI->second = NPPop;
	#endif
			
				return (NodeType *)pTemp;
			}
			pTemp = (NodePointer *)malloc( MAX(sizeof(NodeType), sizeof(NodePointer)) );
			if(fNail)
				NAIL_MEMORY(pTemp, MAX(sizeof(NodeType), sizeof(NodePointer)))
	#ifdef _DEBUG
			MallocCount++;
	#endif
			SYNCH_RELEASE_LOCK(Mutex)
	#ifdef NODEPOOL_DEBUG
			NodePoolInsIter NPI = NPM.insert(make_pair((void *)pTemp, NPPop));
			if(NPI.second = false && NPI.first->second != NPFree)
			{
				sprintf(sMessageBuffer, "NodePool::Pop(): Address %X already allocated as %s", pTemp, NodePoolStates[NPI.first->second]);
				throw runtime_error(sMessageBuffer);
			}
			NPI.first->second = NPPop;
	#endif

			return (NodeType *)pTemp;
		}

		unsigned int IncOwnerCount()
		{
			SYNCH_INCREMENT(OwnerCount)
			return OwnerCount;
		}
		unsigned int DecOwnerCount()
		{
			SYNCH_DECREMENT(OwnerCount)
			return OwnerCount;
		}
		unsigned int SetNodeToOwnerRatio(unsigned int Ratio)
		{
			SYNCH_ACQUIRE_LOCK(Mutex)
			NodeToOwnerRatio = Ratio;
			SYNCH_RELEASE_LOCK(Mutex)
		}

		//The number of nodes in the pool.
		unsigned int NodeCount;
	#ifdef _DEBUG
		unsigned int NewCount, DeleteCount, MallocCount, FreeCount;
	#endif
		//The number of currently existing owners. An owner is any instance that
		//could potentially use a node. If no more owners exist, we can free up
		//all the memory in the node pool.
		COUNT_TYPE OwnerCount;

	protected:
		struct NodePointer
		{
			NodePointer *pNext;
		};

		LOCK_TYPE Mutex;
		//Points to the head of the node list (singly-linked)
		NodePointer *pNodePool;
		//The number of nodes allowed in the pool per owner. Increase the value
		//to have fewer allocations/deallocations. Decrease the value to
		//conserve more run-time memory.
		unsigned int NodeToOwnerRatio;
		//The number of nodes in the initial pool that is pre-created before
		//allocation begins.
		unsigned int InitialPool;
		//True if memory should be nailed into physical RAM
		bool fNail;
	#ifdef NODEPOOL_DEBUG
		enum NodePoolStateEnum {NPPush, NPPop, NPFree};
		static const char *const NodePoolStates[3];
		typedef map<void *, NodePoolStateEnum> NodePoolMap;
		typedef NodePoolMap::iterator NodePoolIter;
		typedef pair<NodePoolMap::iterator, bool> NodePoolInsIter;
		NodePoolMap NPM;
		char sMessageBuffer[256];
	#endif
	};
	#ifdef NODEPOOL_DEBUG
		template<class NodeType>
		const char *const NodePool<NodeType>::NodePoolStates[3] = {"NPPush", "NPPop", "NPFree"};
	#endif
}	//namespace JMT

//*********************//
//*** STL Additions ***//
//*********************//

//*NOTE: GCC croaks when this is declared in namespace other than STD???
namespace std	{

	#if (_MSC_VER >= 1300)
	template<class T1, class T2, class T3>
	struct triple;
	template<class T1, class T2, class T3>
	bool operator==(const triple<T1,T2,T3> &X, const triple<T1,T2,T3> &Y);
	template<class T1, class T2, class T3>
	bool operator!=(const triple<T1,T2,T3> &X, const triple<T1,T2,T3> &Y);
	template<class T1, class T2, class T3>
	bool operator<(const triple<T1,T2,T3> &X, const triple<T1,T2,T3> &Y);
	template<class T1, class T2, class T3>
	bool operator>(const triple<T1,T2,T3> &X, const triple<T1,T2,T3> &Y);
	template<class T1, class T2, class T3>
	bool operator<=(const triple<T1,T2,T3> &X, const triple<T1,T2,T3> &Y);
	template<class T1, class T2, class T3>
	bool operator>=(const triple<T1,T2,T3> &X, const triple<T1,T2,T3> &Y);
	template<class T1, class T2, class T3>
	triple<T1,T2,T3> make_triple(const T1 &X, const T2 &Y, const T3 &Z);
	#endif

	/*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*\
		triple

		This does the same thing as the STL "pair" object, except it has 3
		members instead of 2. The code is directly expanded from the pair code.
		
		The three members can be of any type. Create one using this:
		make_triple(X,Y,Z);
		Access the members using .first, .second, and .third.
	\*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*/
	template<class T1, class T2, class T3>
	struct triple
	{
		typedef T1 first_type;
		typedef T2 second_type;
		typedef T3 third_type;
		T1 first;
		T2 second;
		T3 third;
		triple()
			: first(T1()), second(T2()), third(T3()) {}
		triple(const T1 &V1, const T2 &V2, const T3 &V3)
			: first(V1), second(V2), third(V3) {}
		template<class U, class V, class W> triple(const triple<U, V, W> &p)
			: first(p.first), second(p.second), third(p.third) {}

	#if (_MSC_VER >= 1300)
		friend bool operator== <T1,T2,T3>(const triple &X, const triple &Y);
		friend bool operator!= <T1,T2,T3>(const triple &X, const triple &Y);
		friend bool operator< <T1,T2,T3>(const triple &X, const triple &Y);
		friend bool operator> <T1,T2,T3>(const triple &X, const triple &Y);
		friend bool operator<= <T1,T2,T3>(const triple &X, const triple &Y);
		friend bool operator>= <T1,T2,T3>(const triple &X, const triple &Y);
		friend triple make_triple<T1,T2,T3>(const T1 &X, const T2 &Y, const T3 &Z);
	#else
		friend bool operator==(const triple &X, const triple &Y)
		{	return (X.first == Y.first && X.second == Y.second && X.third == Y.third);	}

		friend bool operator!=(const triple &X, const triple &Y)
		{	return (!(X == Y));	}

		friend bool operator<(const triple &X, const triple &Y)
		{	return (X.first < Y.first || X.first == Y.first &&
				(X.second < Y.second || X.second == Y.second && X.third < Y.third) );	}

		friend bool operator>(const triple &X, const triple &Y)
		{	return (Y < X);	}

		friend bool operator<=(const triple &X, const triple &Y)
		{	return (!(Y < X));	}

		friend bool operator>=(const triple &X, const triple &Y)
		{	return (!(X < Y));	}

		friend triple make_triple(const T1 &X, const T2 &Y, const T3 &Z)
		{	return (triple(X, Y, Z));	}
	#endif

	};

	#if (_MSC_VER >= 1300)
	template<class T1, class T2, class T3>
	bool operator==(const triple<T1,T2,T3> &X, const triple<T1,T2,T3> &Y)
	{	return (X.first == Y.first && X.second == Y.second && X.third == Y.third);	}

	template<class T1, class T2, class T3>
	bool operator!=(const triple<T1,T2,T3> &X, const triple<T1,T2,T3> &Y)
	{	return (!(X == Y));	}

	template<class T1, class T2, class T3>
	bool operator<(const triple<T1,T2,T3> &X, const triple<T1,T2,T3> &Y)
	{	return (X.first < Y.first || X.first == Y.first &&
			(X.second < Y.second || X.second == Y.second && X.third < Y.third) );	}

	template<class T1, class T2, class T3>
	bool operator>(const triple<T1,T2,T3> &X, const triple<T1,T2,T3> &Y)
	{	return (Y < X);	}

	template<class T1, class T2, class T3>
	bool operator<=(const triple<T1,T2,T3> &X, const triple<T1,T2,T3> &Y)
	{	return (!(Y < X));	}

	template<class T1, class T2, class T3>
	bool operator>=(const triple<T1,T2,T3> &X, const triple<T1,T2,T3> &Y)
	{	return (!(X < Y));	}

	template<class T1, class T2, class T3>
	triple<T1,T2,T3> make_triple(const T1 &X, const T2 &Y, const T3 &Z)
	{	return (triple<T1,T2,T3>(X, Y, Z));	}
	#endif

	#if (_MSC_VER >= 1300)
	template<class T1, class T2, class T3, class T4>
	struct tetra;
	template<class T1, class T2, class T3, class T4>
	bool operator==(const tetra<T1,T2,T3,T4> &X, const tetra<T1,T2,T3,T4> &Y);
	template<class T1, class T2, class T3, class T4>
	bool operator!=(const tetra<T1,T2,T3,T4> &X, const tetra<T1,T2,T3,T4> &Y);
	template<class T1, class T2, class T3, class T4>
	bool operator<(const tetra<T1,T2,T3,T4> &X, const tetra<T1,T2,T3,T4> &Y);
	template<class T1, class T2, class T3, class T4>
	bool operator>(const tetra<T1,T2,T3,T4> &X, const tetra<T1,T2,T3,T4> &Y);
	template<class T1, class T2, class T3, class T4>
	bool operator<=(const tetra<T1,T2,T3,T4> &X, const tetra<T1,T2,T3,T4> &Y);
	template<class T1, class T2, class T3, class T4>
	bool operator>=(const tetra<T1,T2,T3,T4> &X, const tetra<T1,T2,T3,T4> &Y);
	template<class T1, class T2, class T3, class T4>
	tetra<T1,T2,T3,T4> make_tetra(const T1 &W, const T2 &X, const T3 &Y, const T4 &Z);
	#endif

	/*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*\
		tetra

		This does the same thing as the STL "pair" object, except it has 4
		members instead of 2. The code is directly expanded from the pair code.
		
		The four members can be of any type. Create one using this:
		make_tetra(X,Y,Z);
		Access the members using .first, .second, .third, and .fouth.

		*NOTE: This was supposed to be named "quad", but it conflicts with
		another "quad" defined in GCC.
	\*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*/
	template<class T1, class T2, class T3, class T4>
	struct tetra
	{
		typedef T1 first_type;
		typedef T2 second_type;
		typedef T3 third_type;
		typedef T4 fourth_type;
		T1 first;
		T2 second;
		T3 third;
		T4 fourth;
		tetra()
			: first(T1()), second(T2()), third(T3()), fourth(T4()) {}
		tetra(const T1 &V1, const T2 &V2, const T3 &V3, const T4 &V4)
			: first(V1), second(V2), third(V3), fourth(V4) {}
		template<class U, class V, class W, class X> tetra(const tetra<U, V, W, X> &p)
			: first(p.first), second(p.second), third(p.third), fourth(p.fourth) {}

	#if (_MSC_VER >= 1300)
		friend bool operator== <T1,T2,T3,T4>(const tetra &X, const tetra &Y);
		friend bool operator!= <T1,T2,T3,T4>(const tetra &X, const tetra &Y);
		friend bool operator< <T1,T2,T3,T4>(const tetra &X, const tetra &Y);
		friend bool operator> <T1,T2,T3,T4>(const tetra &X, const tetra &Y);
		friend bool operator<= <T1,T2,T3,T4>(const tetra &X, const tetra &Y);
		friend bool operator>= <T1,T2,T3,T4>(const tetra &X, const tetra &Y);
		friend tetra make_tetra<T1,T2,T3,T4>(const T1 &W, const T2 &X, const T3 &Y, const T4 &Z);
	#else
		friend bool operator==(const tetra &X, const tetra &Y)
		{	return (X.first == Y.first && X.second == Y.second && X.third == Y.third && X.fourth == Y.fourth);	}

		friend bool operator!=(const tetra &X, const tetra &Y)
		{	return (!(X == Y));	}

		friend bool operator<(const tetra &X, const tetra &Y)
		{	return (X.first < Y.first || X.first == Y.first &&
				(X.second < Y.second || X.second == Y.second &&
				(X.third < Y.third || X.third == Y.third && X.fourth < Y.fourth) ) );	}

		friend bool operator>(const tetra &X, const tetra &Y)
		{	return (Y < X);	}

		friend bool operator<=(const tetra &X, const tetra &Y)
		{	return (!(Y < X));	}

		friend bool operator>=(const tetra &X, const tetra &Y)
		{	return (!(X < Y));	}

		friend tetra make_tetra(const T1 &W, const T2 &X, const T3 &Y, const T4 &Z)
			{	return (tetra(W, X, Y, Z));	}
	#endif
	};

	#if (_MSC_VER >= 1300)
	template<class T1, class T2, class T3, class T4>
	bool operator==(const tetra<T1,T2,T3,T4> &X, const tetra<T1,T2,T3,T4> &Y)
	{	return (X.first == Y.first && X.second == Y.second && X.third == Y.third && X.fourth == Y.fourth);	}

	template<class T1, class T2, class T3, class T4>
	bool operator!=(const tetra<T1,T2,T3,T4> &X, const tetra<T1,T2,T3,T4> &Y)
	{	return (!(X == Y));	}

	template<class T1, class T2, class T3, class T4>
	bool operator<(const tetra<T1,T2,T3,T4> &X, const tetra<T1,T2,T3,T4> &Y)
	{	return (X.first < Y.first || X.first == Y.first &&
			(X.second < Y.second || X.second == Y.second &&
			(X.third < Y.third || X.third == Y.third && X.fourth < Y.fourth) ) );	}

	template<class T1, class T2, class T3, class T4>
	bool operator>(const tetra<T1,T2,T3,T4> &X, const tetra<T1,T2,T3,T4> &Y)
	{	return (Y < X);	}

	template<class T1, class T2, class T3, class T4>
	bool operator<=(const tetra<T1,T2,T3,T4> &X, const tetra<T1,T2,T3,T4> &Y)
	{	return (!(Y < X));	}

	template<class T1, class T2, class T3, class T4>
	bool operator>=(const tetra<T1,T2,T3,T4> &X, const tetra<T1,T2,T3,T4> &Y)
	{	return (!(X < Y));	}

	template<class T1, class T2, class T3, class T4>
	tetra<T1,T2,T3,T4> make_tetra(const T1 &W, const T2 &X, const T3 &Y, const T4 &Z)
		{	return (tetra<T1,T2,T3,T4>(W, X, Y, Z));	}
	#endif
}	//namespace std

#endif	//JMTLIB_H
