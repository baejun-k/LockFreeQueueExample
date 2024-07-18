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

		// ������ ������
		auto producer = [&pQueue, &producerCnt, num_elements](int start) {
			for (int i = start; i < start + num_elements; ++i)
			{
				if (pQueue->Enqueue(i))
				{
					producerCnt.fetch_add(1);
				}
			}
		};

		// �Һ��� ������
		auto consumer = [&pQueue, &consumerCnt, num_elements]() {
			std::stringstream ss;
			for (int i = 0; i < num_elements; ++i)
			{
				std::optional<int> ret;
				while ((ret = pQueue->Dequeue()) != std::nullopt)
				{
					consumerCnt.fetch_add(1);
					std::this_thread::yield();  // ��� (ť�� ������� �� �����Ƿ�)
				}
			}
		};

		std::vector<std::thread> threads;

		// ������ ������ ����
		for (int i = 0; i < num_threads; ++i)
		{
			threads.push_back(std::thread(producer, i * num_elements));
		}

		// �Һ��� ������ ����
		for (int i = 0; i < num_threads; ++i)
		{
			threads.push_back(std::thread(consumer));
		}

		// ��� �����尡 ����� ������ ���
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
