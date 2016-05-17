// mutex.h: interface for the mutex class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MY_MUTEX_H__E9C53005_869C_4F43_B05C_58E5596EC023__INCLUDED_)
#define AFX_MY_MUTEX_H__E9C53005_869C_4F43_B05C_58E5596EC023__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class my_mutex  
{
public:
	my_mutex()
	{
		::InitializeCriticalSection(&_critical_section);
	}
	virtual ~my_mutex()
	{
		::DeleteCriticalSection(&_critical_section);
	}
	void lock()
	{
		::EnterCriticalSection( &_critical_section);
	}
	void unlock()
	{
		::LeaveCriticalSection(&_critical_section);
	}

private:
	CRITICAL_SECTION _critical_section;
};

//可以判断当前是否能够尝试拿到锁的类 tyh
class my_mutexex  
{
public:
	my_mutexex()
	{
		::InitializeCriticalSection(&_critical_section);
	}
	virtual ~my_mutexex()
	{
		::DeleteCriticalSection(&_critical_section);
	}
	void lock()
	{
		::EnterCriticalSection( &_critical_section);
	}

	bool trylock()
	{
		BOOL bRet = ::TryEnterCriticalSection(&_critical_section);
		return !!bRet;
	}

	void unlock()
	{
		::LeaveCriticalSection(&_critical_section);
	}

private:
	CRITICAL_SECTION _critical_section;
};

class my_scoped_lock
{
public:
	my_scoped_lock( my_mutex& mutex_obj ) : _mutex_obj(mutex_obj)
	{
		_mutex_obj.lock();
	}
	~my_scoped_lock()
	{
		_mutex_obj.unlock();
	}
private:
	my_mutex& _mutex_obj;
};

template <class T>
class my_scoped_lock_ex
{
public:
	my_scoped_lock_ex( T& mutexex_obj , bool bautolock = false) : _mutexex_obj(mutexex_obj), _lock_in(false)
	{
		if (bautolock)
		{
			_mutexex_obj.lock();
			_lock_in = true;
		}
		
	}

	bool trylock()
	{
		//已经加锁了，就不能重入了
		if (_lock_in)
		{
			return !_lock_in;
		}

		_lock_in = _mutexex_obj.trylock();
		return _lock_in;
	}

	~my_scoped_lock_ex()
	{
		if (_lock_in)
		{
			_mutexex_obj.unlock();
		}
	}
private:
	T& _mutexex_obj;
	bool _lock_in;

private:
	my_scoped_lock_ex() {}
};

#endif // !defined(AFX_MY_MUTEX_H__E9C53005_869C_4F43_B05C_58E5596EC023__INCLUDED_)
