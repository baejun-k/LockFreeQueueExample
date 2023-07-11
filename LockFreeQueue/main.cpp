#include <iostream>
#include "container/LockFreeQueue.h"
#include <thread>
#include <string>


void PrintDequeuOne(container::IQueueHandle<int>& queueHandle)
{
	int result = 0;
	if (queueHandle.Dequeue(result))
	{
		std::cout << "pop: " << result << std::endl;
	}
	else
	{
		std::cout << "pop: fail" << std::endl;
	}
}

using QueueHandle = container::IQueueHandle<std::string>;
using LockFreeQueue = container::LockFreeQueue<std::string>;

int main(int argc, char** argv)
{
	QueueHandle* pQueue = dynamic_cast<QueueHandle*>(new LockFreeQueue());

	assert(nullptr != pQueue);

	static constexpr int NumTestThreads = 5;
	std::thread testThreads[NumTestThreads];
	for (int i = 0; i < NumTestThreads; ++i)
	{
		testThreads[i] = std::thread([&](int idx) {
			std::string threadName = std::to_string(idx);

			for (int n = 0; n < 10000; ++n)
			{
				std::string str = threadName + " : " + std::to_string(n);
				pQueue->Enqueue(std::move(str));
			}
		}, i);
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(2));

	std::thread outThreads[2];
	for (int i = 0; i < 2; ++i)
	{
		outThreads[i] = std::thread([&](int idx) {
			const int MaxCount = NumTestThreads * 10000;
			std::string res;
			for (int i = 0; i < MaxCount / 2; ++i)
			{
				if (pQueue->Dequeue(res))
				{
					printf("%s\n", res.c_str());
				}
				else
				{
					printf("faild\n");
				}
			}
		}, i);
	}
	
	for (int i = 0; i < NumTestThreads; ++i)
	{
		testThreads[i].join();
	}
	for (int i = 0; i < 2; ++i)
	{
		outThreads[i].join();
	}

	return 0;
}