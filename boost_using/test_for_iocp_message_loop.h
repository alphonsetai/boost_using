#include <boost/asio.hpp>
#include <boost/function.hpp>
#include <iostream>

extern LONGLONG test_start_time;

static LONGLONG test_start_count = 0;

extern const int loop_count;

class test_for_iocp_message_loop
{
public:
	void add(int priority, boost::function<void()> function)
	{
		/*handlers_.push(queued_handler(priority, function));*/

		function();
	}

	void execute_all()
	{
		/*while (!handlers_.empty())
		{
			queued_handler handler = handlers_.top();
			handler.execute();
			handlers_.pop();
		}*/
	}

	// A generic wrapper class for handlers to allow the invocation to be hooked.
	class wrapped_handler
	{
	public:
		wrapped_handler(test_for_iocp_message_loop& q, LONGLONG TimeTick)
			: queue_(q), TimeTick_(TimeTick)
		{
		}

		void operator()()
		{
			LARGE_INTEGER qpfreq;
			LARGE_INTEGER qpcnt;
			QueryPerformanceFrequency(&qpfreq);
			QueryPerformanceCounter(&qpcnt);


			LONGLONG SpeedTime = qpcnt.QuadPart - TimeTick_;
			SpeedTime *= 1000000;
			SpeedTime /= qpfreq.QuadPart;

			test_start_count++;

			//std::cout << "post -> run, speed time: " << SpeedTime << std::endl;

			if (loop_count == test_start_count)
			{
				LONGLONG time_tmp = (qpcnt.QuadPart - test_start_time) * 1000000 / qpfreq.QuadPart;
				std::cout << "test total speed time: " << time_tmp << "us, " << time_tmp/1000 << "ms, "<< time_tmp/(1000*1000) << "s" << std::endl;
			}

			//handler_();
		}

		//template <typename Arg1>
		//void operator()(Arg1 arg1)
		//{
		//	//handler_(arg1);
		//}

		//template <typename Arg1, typename Arg2>
		//void operator()(Arg1 arg1, Arg2 arg2)
		//{
		//	//handler_(arg1, arg2);
		//}

		//private:
		test_for_iocp_message_loop& queue_;
		LONGLONG TimeTick_;
	};

	//template <typename Handler>
	wrapped_handler wrap(LONGLONG TimeTick)
	{
		return wrapped_handler(*this, TimeTick);
	}

private:
	class queued_handler
	{
	public:
		queued_handler(int p, boost::function<void()> f)
			: priority_(p), function_(f)
		{
		}

		void execute()
		{
			function_();
		}

		friend bool operator<(const queued_handler& a,
			const queued_handler& b)
		{
			return a.priority_ < b.priority_;
		}

	private:
		int priority_;
		boost::function<void()> function_;
	};

	//std::priority_queue<queued_handler> handlers_;
};