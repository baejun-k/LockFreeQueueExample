#include "pch.h"
#include "CppUnitTest.h"
#include <thread>
#include <vector>
#include <cassert>
#include <memory>
#include "../LockFreeQueue/container/LockFreeQueue.h"


using namespace Microsoft::VisualStudio::CppUnitTestFramework;


namespace UnitTest1
{
TEST_CLASS(LockFreeQueueUnitTest)
{
public:
	TEST_METHOD(TestMethod1)
	{
		using namespace container;

		std::unique_ptr<IQueueHandle<int>> pQueue =
			std::unique_ptr<IQueueHandle<int>>(new LockFreeQueue<int>());

		const int num_threads = 16;
		const int num_elements = 10000;
		std::atomic<size_t> producerCnt = 0;
		std::atomic<size_t> consumerCnt = 0;

		// 생산자 스레드
		auto producer = [&pQueue, &producerCnt, num_elements](int start) {
			for (int i = start; i < start + num_elements; ++i)
			{
				if (pQueue->Enqueue(i))
				{
					producerCnt.fetch_add(1);
				}
			}
		};

		// 소비자 스레드
		auto consumer = [&pQueue, &consumerCnt, num_elements]() {
			std::stringstream ss;
			for (int i = 0; i < num_elements; ++i)
			{
				std::optional<int> ret;
				while ((ret = pQueue->Dequeue()) != std::nullopt)
				{
					consumerCnt.fetch_add(1);
					std::this_thread::yield();  // 대기 (큐가 비어있을 수 있으므로)
				}
			}
		};

		std::vector<std::thread> threads;

		// 생산자 스레드 시작
		for (int i = 0; i < num_threads; ++i)
		{
			threads.push_back(std::thread(producer, i * num_elements));
		}

		// 소비자 스레드 시작
		for (int i = 0; i < num_threads; ++i)
		{
			threads.push_back(std::thread(consumer));
		}

		// 모든 스레드가 종료될 때까지 대기
		for (auto& t : threads)
		{
			t.join();
		}

		std::stringstream ss;
		ss << producerCnt.load() << " : " << consumerCnt.load() << std::endl;
		Logger::WriteMessage(ss.str().c_str());
		Assert::IsTrue(pQueue->Dequeue() == std::nullopt);
	}
};
}
