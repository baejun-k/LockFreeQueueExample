#ifndef __INTERFACE_QUEUE_HANDLE_HEADER__
#define __INTERFACE_QUEUE_HANDLE_HEADER__


#include <optional>


namespace container
{

template<typename DataType>
class IQueueHandle
{
public:
	IQueueHandle() {}
	virtual ~IQueueHandle() {}

	virtual bool Enqueue(DataType const& in_item) = 0;
	virtual bool Enqueue(DataType&& in_item) noexcept = 0;
	virtual std::optional<DataType> Dequeue() = 0;
	virtual void Clear() = 0;
};

}


#endif // !__INTERFACE_QUEUE_HANDLE_HEADER__