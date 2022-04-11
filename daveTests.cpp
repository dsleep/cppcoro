#include <cppcoro/task.hpp>
#include <cppcoro/io_service.hpp>
#include <cppcoro/sync_wait.hpp>
#include <cppcoro/schedule_on.hpp>
#include <cppcoro/static_thread_pool.hpp>
#include <cppcoro/when_all_ready.hpp>



cppcoro::task<> testValue() noexcept
{
	auto ioThreadId = std::this_thread::get_id();
	// CHECK(ioThreadId != mainThreadId);
	co_return;
};

cppcoro::task<> process_events(cppcoro::io_service& ioService)
{
	// Process events until the io_service is stopped.
	// ie. when the last io_work_scope goes out of scope.
	ioService.process_events();
	co_return;
}

int main()
{
	auto mainThreadId = std::this_thread::get_id();

	std::thread::id ioThreadId;
	cppcoro::io_service ioService;
	cppcoro::static_thread_pool threadPool(3);

	cppcoro::schedule_on(threadPool, testValue());

	auto start = [&]() -> cppcoro::task<> {
		ioThreadId = std::this_thread::get_id();

		co_await ioService.schedule();
		{
			cppcoro::io_work_scope ioScope(ioService);
		}

		co_await threadPool.schedule();

		//CHECK(ioThreadId != mainThreadId);
		co_return;
	};

    cppcoro::sync_wait(cppcoro::when_all_ready(
		[&]() -> cppcoro::task<> {
			// CHECK(std::this_thread::get_id() == mainThreadId);

			co_await cppcoro::schedule_on(threadPool, start());

			// TODO: Uncomment this check once the implementation of task<T>
			// guarantees that the continuation will resume on the same thread
			// that the task completed on. Currently it's possible to resume on
			// the thread that launched the task if it completes on another thread
			// before the current thread could attach the continuation after it
			// suspended. See cppcoro issue #79.
			//
			// The long-term solution here is to use the symmetric-transfer capability
			// to avoid the use of atomics and races, but we're still waiting for MSVC to
			// implement this (doesn't seem to be implemented as of VS 2017.8 Preview 5)
			// CHECK(std::this_thread::get_id() == ioThreadId);
		}(),
		process_events(ioService)));

	
	return 0;
}
