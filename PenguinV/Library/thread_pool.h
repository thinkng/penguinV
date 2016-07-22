#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <list>
#include <mutex>
#include <thread>
#include <vector>

namespace Thread_Pool
{
	class ThreadPool;

	// General abstract class to work with thread pool
	class AbstractTaskProvider
	{
	public:
		friend class ThreadPool;
		friend class TaskProvider;
		friend class TaskProviderSingleton;

		AbstractTaskProvider();
		AbstractTaskProvider(const AbstractTaskProvider &);
		virtual ~AbstractTaskProvider();

		AbstractTaskProvider & operator=(const AbstractTaskProvider &);

	protected:
		virtual void _task(size_t) = 0; // this function must be overrided in child class and should contain code specific to task ID
										// parameter in function is task ID. This function is called by thread pool
		void _wait();

		bool _ready() const;

	private:
		std::atomic < size_t > _taskCount;          // number of tasks to do
		std::atomic < size_t > _givenTaskCount;     // number of tasks what were given to thread pool
		std::atomic < size_t > _completedTaskCount; // number of completed tasks

		bool _running;                    // boolean variable specifies the state of tasks processing
		std::mutex _completion;           // mutex for synchronization reporting about completion of all tasks
										  // this mutex is waited in _wait() function
		std::condition_variable _waiting; // condition variable for verification that all tasks are really completed

		void _taskRun(bool skip); // function called by thread pool
	};

	// Concrete class of task provider in case when thread pool is not singleton
	class TaskProvider : public AbstractTaskProvider
	{
	public:
		TaskProvider();
		TaskProvider(ThreadPool * pool);
		TaskProvider(const TaskProvider & provider);
		virtual ~TaskProvider();

		TaskProvider & operator=(const TaskProvider & provider);

		void setThreadPool( ThreadPool * pool );

	protected:
		void _run( size_t taskCount );

		bool _ready() const;
	private:
		ThreadPool * _threadPool; // a pointer to thread pool
	};

	class ThreadPool
	{
	public:
		ThreadPool() { };
		ThreadPool(size_t threads);
		ThreadPool & operator=(const ThreadPool &) = delete;
		ThreadPool(const ThreadPool &) = delete;
		~ThreadPool();

		void resize( size_t threads );
		size_t threadCount() const;

		void add( AbstractTaskProvider * provider, size_t taskCount );
		void remove( AbstractTaskProvider * provider );
		bool empty();
		void clear();

		void stop();
	private:
		std::vector < std::thread > _worker; // an array of worker threads
		std::mutex _runTask;                 // mutex to synchronize threads
		std::vector < uint8_t > _run;        // indicator for threads to run tasks
		std::vector < uint8_t > _exit;       // indicator for threads to close themselfs
		std::condition_variable _waiting;    // condition variable for synchronization of threads

		std::mutex _creation;                       // mutex for thread creation verification
		std::atomic < size_t > _runningThreadCount; // variable used to calculate a number of running threads
		std::condition_variable _completeCreation;  // condition variable for verification that all threads are created
		std::size_t _threadCount;                   // current number of threads in pool
		bool _threadsCreated;                       // indicator for pool that all threads are created

		std::list < AbstractTaskProvider * > _task; // a list of tasks to perform
		std::mutex _taskInfo;                       // mutex for synchronization between threads and pool to manage tasks

		static void _workerThread( ThreadPool * pool, size_t threadId );
	};

	// Thread pool singleton (or monoid class) for whole application
	// In most situation thread pool must be one
	class ThreadPoolMonoid
	{
	public:
		static ThreadPool & instance()
		{
			static ThreadPoolMonoid provider; // one and only monoid object

			return provider._pool;
		};

		ThreadPoolMonoid & operator=(const ThreadPoolMonoid &) = delete;
		ThreadPoolMonoid(const ThreadPoolMonoid &) = delete;
		~ThreadPoolMonoid() { };
	private:
		ThreadPoolMonoid() { };

		ThreadPool _pool; // one and only thread pool
	};

	// Concrete class of task provider with thread pool singleton
	class TaskProviderSingleton : public AbstractTaskProvider
	{
	public:
		TaskProviderSingleton();
		TaskProviderSingleton(const TaskProviderSingleton &);
		virtual ~TaskProviderSingleton();

		TaskProviderSingleton & operator=(const TaskProviderSingleton &);

	protected:
		void _run( size_t taskCount );
	};
};