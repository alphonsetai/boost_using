// boost_using.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
//#include "boost\asio.hpp"
#include "async_tcp_echo_server.h"
#include "async_udp_echo_server.h"
#include "prioritised_handlers.h"
#include "test_for_iocp_message_loop.h"

#include "my_mutex.h"

#include <iostream>

#include <list>

//----------------------------------------------------------------------
void high_priority_handler(const boost::system::error_code& /*ec*/)
{
	std::cout << "High priority handler\n";
}

void middle_priority_handler(const boost::system::error_code& /*ec*/)
{
	std::cout << "Middle priority handler\n";
}

void low_priority_handler()
{
	std::cout << "Low priority handler\n";
}


DWORD WINAPI TCPServerThread(LPVOID lpParam)
{
	const int iTCPPort = 15666;


	std::stringstream ss;
	ss << "TCPServerThread Start, port:" << iTCPPort << std::endl;
	std::cout <<  ss.str();

	boost::asio::io_service io_service;
	tcp_server s(io_service, iTCPPort);
	io_service.run();


	return 0;
}

DWORD WINAPI UDPServerThread(LPVOID lpParam)
{
	const int iUDPPort = 15666;

	std::stringstream ss;
	ss << "UDPServerThread Start, port:" << iUDPPort << std::endl;
	std::cout <<  ss.str();


	boost::asio::io_service io_service;
	udp_server s(io_service, iUDPPort);
	io_service.run();


	return 0;
}

DWORD WINAPI PrioritisedHandlersThread(LPVOID lpParam)
{
	boost::asio::io_service io_service;
	
	handler_priority_queue pri_queue;
	
	// Post a completion handler to be run immediately.
	io_service.post(pri_queue.wrap(0, low_priority_handler));
	
	// Start an asynchronous accept that will complete immediately.
	/*tcp::endpoint endpoint(boost::asio::ip::address_v4::loopback(), 0);
	tcp::acceptor acceptor(io_service, endpoint);
	tcp::socket server_socket(io_service);
	acceptor.async_accept(server_socket, pri_queue.wrap(100, high_priority_handler));
	tcp::socket client_socket(io_service);
	client_socket.connect(acceptor.local_endpoint());*/
	
	// Set a deadline timer to expire immediately.
	boost::asio::deadline_timer timer(io_service);
	timer.expires_at(boost::posix_time::neg_infin);
	timer.async_wait(pri_queue.wrap(42, middle_priority_handler));
	
	while (io_service.run_one())
	{
		// The custom invocation hook adds the handlers to the priority queue
		// rather than executing them from within the poll_one() call.
		while (io_service.poll_one())
			;
	
		pri_queue.execute_all();
	}


	return 0;
}


const int loop_count = 100000000;
//////////////////////////////////////////////////////////////////////////
my_mutex g_mutex1;
std::list<LONGLONG> g_lMessagePool;

static LONGLONG normal_thread_start_time = 0;


DWORD WINAPI test_normal_thread_loop_thread(LPVOID lpParam)
{
	HANDLE hEvent = (HANDLE)lpParam;
	assert(NULL != hEvent);

	std::list<LONGLONG> lMessagePool;
	LONGLONG StartQuad = 0;
	LONGLONG EndQuad = 0;

	int i = 0;
	
	LARGE_INTEGER qpfreq;
	LARGE_INTEGER qpcnt;
	QueryPerformanceFrequency(&qpfreq);
	
	while(i < loop_count)
	{
		
		{
			my_scoped_lock locker(g_mutex1);
			lMessagePool.swap(g_lMessagePool);
		}

		if (lMessagePool.empty())
		{
			WaitForSingleObject(hEvent, INFINITE);
		}

		QueryPerformanceCounter(&qpcnt);

		//计算event切换过来的耗时
		if (!lMessagePool.empty())
		{
			StartQuad = *(lMessagePool.begin());
		}

		LONGLONG SpeedTime = qpcnt.QuadPart - StartQuad;
		SpeedTime *= 1000000;
		SpeedTime /= qpfreq.QuadPart;

		//std::cout << "-- brenchmark wait for process -- SpeedTime: " << SpeedTime << std::endl;

		if (!lMessagePool.empty() && 0 == i)
		{
			StartQuad = *(lMessagePool.begin());
		}

		while (!lMessagePool.empty())
		{
			if (loop_count == i+1)
			{
				EndQuad = *(lMessagePool.begin());
			}

			lMessagePool.pop_front();
			i++;
		}

		SpeedTime = qpcnt.QuadPart;
		QueryPerformanceCounter(&qpcnt);
		SpeedTime = qpcnt.QuadPart - SpeedTime;
		SpeedTime *= 1000000;
		SpeedTime /= qpfreq.QuadPart;
		//std::cout << "-- brenchmark process end -- SpeedTime: " << SpeedTime << std::endl;
	}
	
	//计算一下
	LONGLONG time_tmp = qpcnt.QuadPart - normal_thread_start_time;
	time_tmp *= 1000000;
	time_tmp /= qpfreq.QuadPart;
	std::cout << "-- total SpeedTime: " << time_tmp << "us, " << time_tmp/1000 << "ms, "<< time_tmp/(1000*1000) << "s" << std::endl;

	return 0;
}

void test_normal_thread_loop()
{
	

	HANDLE hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	assert(NULL != hEvent);

	HANDLE hThread = CreateThread(NULL, NULL, test_normal_thread_loop_thread, hEvent, NULL, NULL);

	int i = 0;
	while(i < loop_count)
	{
		//LARGE_INTEGER qpfreq;
		LARGE_INTEGER qpcnt;
		QueryPerformanceCounter(&qpcnt);
		//QueryPerformanceFrequency(&qpfreq);
		//return (qpcnt.QuadPart * 1000) / qpfreq.QuadPart;

		if (0 == normal_thread_start_time)
		{
			normal_thread_start_time = qpcnt.QuadPart;
		}

		my_scoped_lock locker(g_mutex1);
		g_lMessagePool.push_back(qpcnt.QuadPart);
		SetEvent(hEvent);

		i++;
	}
	
	//CloseHandle(hThread);
	WaitForSingleObject(hThread, INFINITE);
}

//////////////////////////////////////////////////////////////////////////
static LONGLONG test_start_time = 0;


DWORD WINAPI test_iocp_thread(LPVOID lpParam)
{
	boost::asio::io_service* pTmp = (boost::asio::io_service*)lpParam;
	if (NULL ==  pTmp)
	{
		return 0;
	}

	pTmp->run();
	
	/*while ()
	{

	}*/

	return 0;
}

void test_iocp()
{
	boost::asio::io_service io_service;
	test_for_iocp_message_loop test_for_loop;

	HANDLE hTcpThread = CreateThread(NULL, NULL, test_iocp_thread, &io_service, NULL, NULL);

	LARGE_INTEGER qpcnt;
	QueryPerformanceCounter(&qpcnt);

	int i = 0;
	while (i < loop_count)
	{
		if (0 == test_start_time)
		{
			test_start_time = qpcnt.QuadPart;
		}
		io_service.post(test_for_loop.wrap(qpcnt.QuadPart));
		i++;
	}
	

	WaitForSingleObject(hTcpThread, INFINITE);
}


int _tmain(int argc, _TCHAR* argv[])
{
	//brenchmask

	if (2 == argc)
	{
		//测试iocp的性能(消息)
		test_iocp();
		return 0;

	}
	else if (3 == argc)
	{
		//测试线程事件性能（消息）
		test_normal_thread_loop();
		return 0;
	}
	
	
	
	
	
	
	
	HANDLE hTcpThread = CreateThread(NULL, NULL, TCPServerThread, NULL, NULL, NULL);
	HANDLE hUdpThread = CreateThread(NULL, NULL, UDPServerThread, NULL, NULL, NULL);
	HANDLE hPrioritisedHandlersThread = CreateThread(NULL, NULL, PrioritisedHandlersThread, NULL, NULL, NULL);

	do 
	{
		Sleep(10*1000);
	} while (true);

	return 0;
}

