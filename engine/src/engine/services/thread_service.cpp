#include <engine/services/thread_service.hpp>
#include <unordered_map>

namespace kokoro
{
	namespace
	{
		//------------------------------------------------------------------------------------------------------------------------
		class cworker final : public tf::WorkerInterface
		{
		public:
			cworker(unsigned thread_count);
			~cworker() = default;

			void scheduler_prologue(tf::Worker& worker) override final;
			void scheduler_epilogue(tf::Worker& worker, std::exception_ptr ptr) override final;

		private:
			std::unordered_map<uint64_t, std::thread::id> m_threads;
		};

		//------------------------------------------------------------------------------------------------------------------------
		cworker::cworker(unsigned thread_count)
		{
			m_threads.reserve(thread_count);
		}

		//------------------------------------------------------------------------------------------------------------------------
		void cworker::scheduler_prologue(tf::Worker& worker)
		{
			m_threads[worker.id()] = std::this_thread::get_id();
		}

		//------------------------------------------------------------------------------------------------------------------------
		void cworker::scheduler_epilogue(tf::Worker& worker, std::exception_ptr ptr)
		{
			m_threads.erase(worker.id());
		}

	} //- unnamed

	//------------------------------------------------------------------------------------------------------------------------
	bool cthread_service::init()
	{
		const auto available_threads = std::thread::hardware_concurrency();
		const auto thread_count = static_cast<uint64_t>(static_cast<float>(available_threads) * 0.75f);

		//- Allocate executor using 75% of available threads
		m_executor = std::make_unique<tf::Executor>(thread_count, std::make_shared<cworker>(thread_count));
		return true;
	}

	//------------------------------------------------------------------------------------------------------------------------
	void cthread_service::post_init()
	{

	}

	//------------------------------------------------------------------------------------------------------------------------
	void cthread_service::shutdown()
	{

	}

	//------------------------------------------------------------------------------------------------------------------------
	void cthread_service::update(float dt)
	{

	}

} //- kokoro