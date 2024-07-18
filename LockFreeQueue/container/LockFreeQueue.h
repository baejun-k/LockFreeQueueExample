#ifndef __LOCK_FREE_QUEUE_HEADER__
#define __LOCK_FREE_QUEUE_HEADER__


#include "IQueueHandle.h"
#include <functional>
#include <atomic>
#include <assert.h>


namespace container
{

namespace _private
{

struct DefaultAlloc
{
	void* operator() (const size_t& sz) const
	{
		return std::malloc(sz);
	}
};

struct DefaultDealloc
{
	void operator() (void* ptr) const
	{
		if (nullptr != ptr)
		{
			std::free(ptr);
		}
	}
};

}

template<
	typename DataType,
	typename AllocFn = _private::DefaultAlloc,
	typename DeallocFn = _private::DefaultDealloc
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
		~Node() 
		{ 
			next = nullptr; 
		}
	};

	using AllocFn_t = AllocFn;
	using DeallocFn_t = DeallocFn;

	LockFreeQueue::Node m_head;
	std::atomic<LockFreeQueue::Node*> m_tail;

	AllocFn_t   m_allocFn;
	DeallocFn_t m_deallocFn;

public:
	LockFreeQueue()
		: IQueueHandle<DataType>()
		, m_head()
		, m_tail(&m_head)
		, m_allocFn()
		, m_deallocFn()
	{}
	LockFreeQueue(LockFreeQueue const&) = delete;
	LockFreeQueue(LockFreeQueue&&) noexcept = delete;
	LockFreeQueue& operator=(LockFreeQueue const&) = delete;
	LockFreeQueue& operator=(LockFreeQueue&&) noexcept = delete;

	virtual ~LockFreeQueue() 
	{
		this->Clear();
	}

	virtual bool Enqueue(DataType const& in_item) override
	{
		Node* const pNewNode = reinterpret_cast<Node*>(m_allocFn(sizeof(Node)));
		if (nullptr == pNewNode)
		{
			return false;
		}
		new (pNewNode) Node();
		pNewNode->data = in_item;
		_Enqueue(pNewNode);
		return true;
	}

	virtual bool Enqueue(DataType&& in_item) noexcept override
	{
		Node* const pNewNode = reinterpret_cast<Node*>(m_allocFn(sizeof(Node)));
		if (nullptr == pNewNode)
		{
			return false;
		}
		new (pNewNode) Node();
		pNewNode->data = std::move(in_item);
		_Enqueue(pNewNode);
		return true;
	}

	virtual std::optional<DataType> Dequeue() override
	{
		Node* pResult = _Dequeue();
		if (nullptr == pResult)
		{
			return std::nullopt;
		}
		std::optional<DataType> result = pResult->data;
		m_deallocFn(pResult);
		return result;
	}

	virtual void Clear() override
	{
		Node* pNext = _Dequeue();
		while (nullptr != pNext)
		{
			m_deallocFn(pNext);
			pNext = _Dequeue();
		};
	}

private:
	void _Enqueue(Node* const pNewNode)
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
		m_tail.compare_exchange_weak(pOldTail, pNewNode);
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