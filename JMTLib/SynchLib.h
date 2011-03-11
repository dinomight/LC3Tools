// Copyright 2002 Ashley Wise
// JediMasterThrash@comcast.net

#ifndef SYNCHLIB_H
#define SYNCHLIB_H

#include <stdexcept>
using namespace std;

/*============================================================================*\
 *	Macros for cross-platform thread-synchronization.
 *
 *	The synchronization overhead can be turned off and on by a single
 *	#define MULTITHREADED
 *
 *	See below for detailed documentation on the Reference class.
\*============================================================================*/

#ifdef MULTITHREADED
	#define R(Expression)	(*Expression)
	#define SYNCH_REF(Type)	Reference<Type>
	#define SYNCH_RETURN(Type, Expression, Count)\
		return Reference<Type>(Expression, Count);

	#if defined WIN32
		#ifndef _WIN32_WINNT
		#define _WIN32_WINNT 0x0500
		#endif
		#include <windows.h>

		#define LOCK_TYPE	LONG volatile

		#define SYNCH_CREATE_LOCK(Lock)\
			Lock = 0;
			
		#define SYNCH_ACQUIRE_LOCK(Lock)\
			while(InterlockedCompareExchange(&Lock, 1, 0) != 0)\
				SYNCH_SLEEP(0)

		#define SYNCH_RELEASE_LOCK(Lock)\
			Lock = 0;

		#define SYNCH_CLOSE_LOCK(Lock)
			
		#define MUTEX_TYPE	HANDLE

		#define SYNCH_CREATE_MUTEX(Mutex)\
			if( !(Mutex = CreateMutex(NULL, false, NULL)) )\
				throw SynchFailed("SynchLib: Failed to create Mutex");\
			
		#define SYNCH_ACQUIRE_MUTEX(Mutex)\
			switch( WaitForSingleObject(Mutex, INFINITE) )\
			{\
			case WAIT_ABANDONED:\
			case WAIT_FAILED:\
			case WAIT_TIMEOUT:\
				throw SynchFailed("SynchLib: Failed to acquire Mutex");\
			case WAIT_OBJECT_0:\
				break;\
			}

		#define SYNCH_RELEASE_MUTEX(Mutex)\
			if( !ReleaseMutex(Mutex) )\
				throw SynchFailed("SynchLib: Failed to release Mutex");

		#define SYNCH_CLOSE_MUTEX(Mutex)\
			if( Mutex && !CloseHandle(Mutex) )\
				throw SynchFailed("SynchLib: Failed to close Mutex");

		#define COUNT_TYPE	LONG volatile

		#define SYNCH_INCREMENT(Count)\
			InterlockedIncrement(&Count);

		#define SYNCH_DECREMENT(Count)\
			InterlockedDecrement(&Count);

	#elif defined UNIX_BUILD
		#include <unistd.h>
		#include <pthread.h>
		#include <asm/atomic.h>

		#define LOCK_TYPE	pthread_spinlock_t

			//Lock = 0;
		#define SYNCH_CREATE_LOCK(Lock)\
			pthread_spin_init(&Lock, 0);

			//while(__cmpxchg(&Lock, 0, 1, 4) != 0)
		#define SYNCH_ACQUIRE_LOCK(Lock)\
			while(pthread_spin_trylock(&Lock))\
				SYNCH_SLEEP(0)

		#define SYNCH_RELEASE_LOCK(Lock)\
			pthread_spin_unlock(&Lock);

		#define SYNCH_CLOSE_LOCK(Lock)\
			pthread_spin_destroy(&Lock);
			
		#define MUTEX_TYPE	pthread_mutex_t

		#define SYNCH_CREATE_MUTEX(Mutex)\
			pthread_mutex_init(&Mutex, NULL);
			
		#define SYNCH_ACQUIRE_MUTEX(Mutex)\
			if(pthread_mutex_lock(&Mutex))\
				throw SynchFailed("SynchLib: Failed to acquire Mutex");\

		#define SYNCH_RELEASE_MUTEX(Mutex)\
			if(pthread_mutex_unlock(&Mutex) )\
				throw SynchFailed("SynchLib: Failed to release Mutex");

		#define SYNCH_CLOSE_MUTEX(Mutex)\
			if(pthread_mutex_destroy(&Mutex))\
				throw SynchFailed("SynchLib: Failed to close Mutex");

		#define COUNT_TYPE	volatile int

		#define SYNCH_INCREMENT(Count)\
			atomic_inc((atomic_t*)&Count);

		#define SYNCH_DECREMENT(Count)\
			atomic_dec((atomic_t*)&Count);
	#else
		#error "Only WIN32 and UNIX Systems Supported"
	#endif
#else
	#define R(Expression)	Expression
	#define SYNCH_REF(Type)	Type &
	#define SYNCH_RETURN(Type, Expression, Count)\
		return Expression;

	#define LOCK_TYPE	long
	#define SYNCH_CREATE_LOCK(Mutex)
	#define SYNCH_ACQUIRE_LOCK(Mutex)
	#define SYNCH_RELEASE_LOCK(Mutex)
	#define SYNCH_CLOSE_LOCK(Mutex)

	#define MUTEX_TYPE	long
	#define SYNCH_CREATE_MUTEX(Mutex)
	#define SYNCH_ACQUIRE_MUTEX(Mutex)
	#define SYNCH_RELEASE_MUTEX(Mutex)
	#define SYNCH_CLOSE_MUTEX(Mutex)

	#define COUNT_TYPE	long
	#define SYNCH_INCREMENT(Count)	Count++;
	#define SYNCH_DECREMENT(Count)	Count--;
#endif

namespace JMT	{

class SynchFailed : public runtime_error
{
public:
	SynchFailed(const char *Str) : runtime_error(Str)	{	}
};

/*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*\
 *	Reference
 *
 *	An instance of this is a reference to a piece of data from a database.
 *
 *	This class is used to provide safe synchronization of data in a
 *	database. This Reference is initially locked when created by the database,
 *	so that nothing else can write to or destroy the referenced data while this
 *	reference exists. Once this reference is out of scope, it unlocks the data.
 *
 *	The reference behaves transparently to the user as if they were having
 *	direct access to the data. The reference simply serves to "intercept"
 *	attempts to read from or write to the data to keep the data synchronized.
 *
 *	For instance:
 *	MyDatabase[255] = 10;
 *	The call to MyDatabase.operator[](255) returns a Reference to the data
 *	at that location. This reference becomes an L-value, which is read and
 *	write. This Reference object intercepts the assignment of 10
 *	to the data. This way the data can stay locked during the write. The
 *	Reference object goes out of scope after this statement is completed
 *	and unlocks the data.
 *
 *	X = MyDatabase[255];
 *	The call to MyDatabase.operator[](255) returns a Reference to the data
 *	at that location. This Reference object becomes an R-value, which is
 *	read only. So it makes a read-only copy of the data which is assigned
 *	to X, and unlocks the protected data.
 *	
 *	Complex expressions involving access to database data can be formed
 *	and will be thread-safe and deadlock-safe. Even if the expression involves
 *	multiple accesses to the same data. This is because of how C++
 *	evaluates expressions. There will be only one L-value at a time, which
 *	needs to remain locked. Other non-L-values will be converted to
 *	read-only copies of the data and unlocked prior to the construction of
 *	the l-value, preventing a deadlock.
 *
 *	There are some operations on data which are impossible to intercept
 *	transparently with a temporary Reference. These include:
 *	member dereferences (. -> *)
 *	array references ( [] )
 *
 *	In order to read member data or array data or call constant functions,
 *	you must explicitly cast the Reference to the data type.
 *	You will then get a read-only copy of the data.
 *	X = ((Type)MyDatabase[255]).x;	or ->x
 *	X = ((Type)MyDatabase[255])[Index]
 *	There are no restrictions on this usage.
 *
 *	If you want read and write access to member data or array data or to call
 *	any functions, you can use the overloaded reference operators.
 *	These require an extra level of indirection.
 *	X = MyDB[255]->x;	or (*MyDB[255]).x //instead of MyDB[255].x
 *	X = (*MyDB[255])->x;	//instead of MyDB[255]->x
 *	X = (*MyDB[255]))[Index];	//instead of MyDB[255][Index]
 *	Additionally, if you use this on a location, you cannot access that
 *	location again in the same expression or statement, otherwise you will
 *	cause a deadlock:
 *	MyDatabase[255]->x = MyDatabase[255]->y+5;	//DEADLOCK!
 *	The deadlock results because the statement requires two locked references
 *	to the same location, when only one locked reference is allowed.
 *
 *	If you want read and write access to member data or array data or to call
 *	any functions or be able to access the same data more than once in an
 *	expression or statement, you must use an explicit Reference instance.
 *	Reference<Type> MyData(MyDatabase[255], true);
 *	The second parameter, true, specifies an explicit Reference. If you
 *	specify false, or neglect the second parameter, a regular non-explicit
 *	Reference will be created. The difference between explicit and
 *	non-explicit References is that non-explicit references are single use
 *	read. That is, once you read from the Reference, the Reference is no
 *	longer valid, and future read or write attempts will throw an exception.
 *	You can write to it many times in one expression though.
 *	Explicit References can be written to or read from multiple times, even
 *	in the same expression, until you explicitly UnLock() or ~destroy() it.
 *
 *	This reference behaves the same as if you were directly accessing the
 *	database.
 *	For instance, they can be used in complex expressions involving other
 *	database accesses, and still require an extra level of indirection:
 *	X = MyData->x;	or (*MyData).x //instead of MyData.x
 *	X = (*MyData)->x;	//instead of MyData->x
 *	X = (*MyData)[Index];	//instead of MyData[Index]
 *	MyData += MyDatabase[10] + MyDatabase[11];
 *
 *	There is one restriction though:
 *	You CANNOT reference the same database location again until the explicit
 *	Reference has been destroyed or unlocked. Doing this WILL cause a deadlock.
 *	Reference<Type> MyData(MyDatabase[255], true);
 *	MyData = MyDatabase[255]+5;	//DEADLOCK!
 *	Instead, in order to access the same locked location mutliple times,
 *	use the same explicit Reference instance:
 *	MyData = MyData+5;	//OK!
 *	
 *	Explicit References will not be unlocked until they go out of scope and are
 *	destroyed, or you call the UnLock method.
 *
 *	If you want your code to be portable between threadsafe and non-threadsafe
 *	compilations, use the R() macro for member references.
 *	R(MyDB[255]).x;
 *	R(MyDB[255])[Index];
 *	Don't use the R() macro for explicit References though.
 *	In a non-threadsafe compilation, Reference instances will still behave
 *	the same, though they are unnecessary. So code that uses explicit
 *	References will still compile and work under a non-threadsafe compilation.
 *
 *	A const Reference works the same, except that it can only be read from;
 *	the data it refers to cannot be modified.
 *	Additionally, if you want to create an explicit const Reference, you must
 *	declare it a const Reference.
 *	const Reference<Type> MyData
 *
 *	Access functions will throw an UnsafeReference() exception if you try
 *	to access unlocked data.
\*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*/
template<class ValueType>
class Reference
{
public:
#ifdef MULTITHREADED
	Reference(ValueType &data, const COUNT_TYPE &count) : Data(data), Count(*const_cast<COUNT_TYPE *>(&count))
	{
		Locked = true;
		Explicit = false;
	}
	Reference(const Reference &Copy, bool expl = false) : Data(Copy.Data), Count(Copy.Count)
	{
		if(!Copy.Locked)
			throw UnsafeReference();
		Locked = true;
		const_cast<Reference *>(&Copy)->Locked = false;
		Explicit = expl;
	}
#else
	Reference(ValueType &data, bool expl = false) : Data(data)
	{
		Locked = true;
		Explicit = expl;
	}
#endif
	operator const ValueType() const
	{
		if(!Locked)
			throw UnsafeReference();
		ValueType data = Data;
		if(!Explicit)
			UnLock();
		return data;
	}
	Reference &operator=(ValueType data)
	{
		if(!Locked)
			throw UnsafeReference();
		Data = data;
		return *this;
	}
#define REF_OP(op)\
	Reference &operator op(ValueType data)\
	{\
		if(!Locked)\
			throw UnsafeReference();\
		Data op data;\
		return *this;\
	}
	REF_OP(+=)
	REF_OP(-=)
	REF_OP(/=)
	REF_OP(%=)
	REF_OP(*=)
	REF_OP(&=)
	REF_OP(|=)
	REF_OP(^=)
	REF_OP(<<=)
	REF_OP(>>=)

	Reference &operator++() 
	{
		if(!Locked)
			throw UnsafeReference();
		++Data;
		return *this;
	}
	const ValueType operator++(int) 
	{
		if(!Locked)
			throw UnsafeReference();
		ValueType data = Data++;
		if(!Explicit)
			UnLock();
		return data;
	}
	Reference &operator--() 
	{
		if(!Locked)
			throw UnsafeReference();
		--Data;
		return *this;
	}
	const ValueType operator--(int) 
	{
		if(!Locked)
			throw UnsafeReference();
		ValueType data = Data--;
		if(!Explicit)
			UnLock();
		return data;
	}
	ValueType &operator*() 
	{
		if(!Locked)
			throw UnsafeReference();
		return Data;
	}
	const ValueType operator*() const
	{
		if(!Locked)
			throw UnsafeReference();
		return Data;
	}
	ValueType *operator->() 
	{
		if(!Locked)
			throw UnsafeReference();
		return &Data;
	}
	const ValueType *operator->() const
	{
		if(!Locked)
			throw UnsafeReference();
		return &Data;
	}
	~Reference()
	{
		UnLock();
	}
	void UnLock() const
	{
		if(Locked)
		{
			SYNCH_DECREMENT(const_cast<Reference *>(this)->Count)
			const_cast<Reference *>(this)->Locked = false;
		}
	}
	ValueType &Data;
	bool Locked, Explicit;
#ifdef MULTITHREADED
	COUNT_TYPE &Count;
#endif
	
	class UnsafeReference : public runtime_error
	{
	public:
		UnsafeReference() : runtime_error("Reference: Non-threadsafe access attempted on a reference.")	{	}
	};
};

}	//namespace JMT

#endif
