#ifndef __LOCK_FREE_QUEUE_HEADER__
#define __LOCK_FREE_QUEUE_HEADER__


#include "IQueueHandle.h"
#include <atomic>
#include <assert.h>


namespace container
{

namespace _private
{

template<typename DataType>
struct DefaultAllocator
{
	DataType* operator() ()
	{
		return new DataType();
	}
};

template<typename DataType>
struct DefaultDeallocator
{
	void operator() (DataType*& ptr)
	{
		if (nullptr != ptr)
		{
			delete ptr;
			ptr = nullptr;
		}
	}
};

}

template<
	typename DataType,
	template<typename Ty> typename Allocator = _private::DefaultAllocator,
	template<typename Ty> typename Deallocator = _private::DefaultDeallocator
>
class LockFreeQueue : public IQueueHandle<DataType>
{
private:
	struct Node
	{
		DataType data;
		std::atomic<LockFreeQueue::Node*> next;
		
		Node()
			: data()
			, next(nullptr)
		{}
	};

	using Allocator_t = Allocator<LockFreeQueue::Node>;
	using Deallocator_t = Deallocator<LockFreeQueue::Node>;

	LockFreeQueue::Node m_head;
	std::atomic<LockFreeQueue::Node*> m_tail;

	Allocator_t   m_allocator;
	Deallocator_t m_deallocator;

public:
	LockFreeQueue()
		: IQueueHandle<DataType>()
		, m_head()
		, m_tail(&m_head)
		, m_allocator()
		, m_deallocator()
	{}
	LockFreeQueue(LockFreeQueue const&) = delete;
	LockFreeQueue& operator=(LockFreeQueue const&) = delete;

	virtual ~LockFreeQueue() 
	{
		this->Clear();
	}

	virtual bool Enqueue(DataType const& in_item) override
	{
		Node* const pNewNode = m_allocator();
		if (nullptr == pNewNode)
		{
			return false;
		}
		pNewNode->data = in_item;
		return _Enqueue(pNewNode);
	}
	virtual bool Enqueue(DataType&& in_item) override
	{
		Node* const pNewNode = m_allocator();
		if (nullptr == pNewNode)
		{
			return false;
		}
		pNewNode->data = std::move(in_item);
		return _Enqueue(pNewNode);
	}
	virtual bool Dequeue(DataType& out_item) override
	{
		Node* pResult = _Dequeue();
		if (nullptr == pResult)
		{
			return false;
		}
		out_item = pResult->data;
		m_deallocator(pResult);
		return true;
	}
	virtual void Clear() override
	{
		Node* pNext = _Dequeue();
		while (nullptr != pNext)
		{
			m_deallocator(pNext);
			pNext = _Dequeue();
		};
	}

private:
	bool _Enqueue(Node* const pNewNode)
	{
		Node* pOldTail = m_tail.load();
		Node* pOldTailNext = pOldTail->next;
		while (true)
		{
			if (nullptr == pOldTailNext)
			{
				if (pOldTail->next.compare_exchange_weak(pOldTailNext, pNewNode))
				{
					break;
				}
			}
			else
			{
				m_tail.compare_exchange_weak(pOldTail, pOldTailNext);
				pOldTail = m_tail.load();
				pOldTailNext = pOldTail->next;
			}
		}
		return m_tail.compare_exchange_weak(pOldTail, pNewNode);
	}
	Node* _Dequeue()
	{
		Node* const pHead = &m_head;
		Node* pOldHeadNext = pHead->next.load();
		Node* pOldTail = m_tail.load();
		while (true)
		{
			if (nullptr == pOldHeadNext)
			{
				break;
			}
			else if (pHead->next.compare_exchange_weak(pOldHeadNext, pOldHeadNext->next.load()))
			{
				break;
			}
			pOldHeadNext = pHead->next;
			pOldTail = m_tail.load();
		}
		Node* pResult = pOldHeadNext;
		m_tail.compare_exchange_weak(pOldHeadNext, pHead);
		return pResult;
	}
};

}


#endif // !__LOCK_FREE_QUEUE_HEADER__