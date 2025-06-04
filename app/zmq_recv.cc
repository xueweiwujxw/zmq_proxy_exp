/**
 * @file zmq_recv.cc
 * @author wlanxww (xueweiwujxw@outlook.com)
 * @brief
 * @date 2025-05-30
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <thread>
#include <stdint.h>
#include <stdlib.h>
#include <signal.h>
#include <condition_variable>
#include <mutex>
#include <atomic>

#include <zmq.hpp>

#include <log.h>
#include <defines.hpp>

std::atomic<bool> running(true);
std::mutex sig_mutex;
std::condition_variable sig_cv;

void sigint_cb_handler(int signum)
{
	running = false;
	sig_cv.notify_all();
}

void sigterm_cb_handler(int signum)
{
	running = false;
	sig_cv.notify_all();
}

int main(int argc, char const *argv[])
{
	struct sigaction sigIntHandler;
	sigIntHandler.sa_handler = sigint_cb_handler;
	sigemptyset(&sigIntHandler.sa_mask);
	sigIntHandler.sa_flags = 0;
	sigaction(SIGINT, &sigIntHandler, NULL);

	struct sigaction sigTermHandler;
	sigTermHandler.sa_handler = sigterm_cb_handler;
	sigemptyset(&sigTermHandler.sa_mask);
	sigTermHandler.sa_flags = 0;
	sigaction(SIGTERM, &sigTermHandler, NULL);

	zmq::context_t ctx;

	zmq::socket_t sub_socket(ctx, ZMQ_SUB);
	try {
		sub_socket.setsockopt(ZMQ_SUBSCRIBE, "data", 4);
		sub_socket.setsockopt(ZMQ_RCVTIMEO, 300);
		sub_socket.connect(FIXED_ZMQ_XPUB_PATH);
		logf_info("recv connected to %s\n", FIXED_ZMQ_XPUB_PATH);
	} catch (const std::exception &e) {
		logf_err("%s\n", e.what());
		return 1;
	}

	std::thread sub_thread([&]() {
		zmq::message_t msg;
		size_t cnt = 0;
		while (running) {
			try {
				auto result = sub_socket.recv(msg);
				if (result.has_value())
					printf("%ld - %s\n", ++cnt,
					       msg.to_string().c_str());
			} catch (const zmq::error_t &e) {
				if (e.num() != ETERM)
					logf_err("Receiver error: %s %d\n",
					         e.what(), e.num());
				else
					logf_info(
					    "Receiver terminated normally\n");
			}
		}
	});

	logf_info("Recving running. Press Ctrl+C to exit...\n");

	std::unique_lock<std::mutex> lock(sig_mutex);
	sig_cv.wait(lock);

	logf_info("Shutting down...\n");

	sub_thread.join();

	sub_socket.close();
	ctx.close();

	logf_info("Clean exit\n");
	lock.unlock();

	return 0;
}
