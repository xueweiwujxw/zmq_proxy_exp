/**
 * @file zmq_send.cc
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
#include <unistd.h>
#include <iostream>
#include <vector>
#include <condition_variable>
#include <mutex>
#include <atomic>

#include <zmq.hpp>

#include <log.h>
#include <defines.hpp>

std::atomic<bool> running(true);
std::atomic<int> send_num(0);
std::condition_variable sig_cv;

int main(int argc, char const *argv[])
{
	if (argc < 2)
		return 1;

	int threads = atoi(argv[1]);
	if (threads < 1 || threads > 16) {
		logf_err("invalid threads num: %d\n", threads);
		return 1;
	}

	zmq::context_t ctx;

	std::vector<std::shared_ptr<zmq::socket_t>> pub_sockets;
	std::vector<std::thread> workers;

	for (auto i = 0; i < threads; ++i) {
		zmq::socket_t pub_socket;
		try {
			auto pub_socket = std::make_shared<zmq::socket_t>(
			    zmq::socket_t(ctx, ZMQ_PUB));
			std::string end_point =
			    FIXED_ZMQ_PUB_PREFIX + std::to_string(i);
			pub_socket->bind(end_point);
			pub_socket->connect(FIXED_ZMQ_XSUB_PATH);

			logf_info("socket %d bind to %s and connect to %s\n", i,
			          end_point.c_str(), FIXED_ZMQ_XSUB_PATH);
			pub_sockets.push_back(pub_socket);
		} catch (const std::exception &e) {
			logf_err("%s\n", e.what());
			return 1;
		}
	}

	logf_info("finish create sockets.\n");

	for (auto i = 0; i < threads; ++i) {
		auto pub_socket = pub_sockets[i];
		workers.emplace_back([pub_socket, i]() {
			pthread_setname_np(
			    pthread_self(),
			    (std::string("zmq_send_test") + std::to_string(i))
			        .c_str());
			logf_info("Thread %d started.\n", i);
			std::string topic_str = "data/" + std::to_string(i);
			zmq::message_t topic(topic_str);
			zmq::message_t msg(topic_str);
			std::vector<char> msg_buffer(1024);
			std::mutex sig_mutex;
			while (running) {
				std::unique_lock<std::mutex> lock(sig_mutex);
				sig_cv.wait(lock);
				lock.unlock();

				auto size = send_num.load();
				logf_info("Thread %d start sending %d msgs.\n",
				          i, size);
				while (size > 0) {
					int len = snprintf(
					    msg_buffer.data(),
					    msg_buffer.size(),
					    "Thread %d Message %d", i, size);
					topic.rebuild(topic_str.data(),
					              topic_str.size());
					msg.rebuild(msg_buffer.data(), len);
					try {
						pub_socket->send(
						    topic,
						    zmq::send_flags::sndmore);
						pub_socket->send(
						    msg, zmq::send_flags::none);
					} catch (const zmq::error_t &e) {
						logf_err("Thread %d Send "
						         "error: %s\n",
						         i, e.what());
					}

					size--;

					usleep(40 * 1000);
				}
				logf_info("Thread %d finished sending msg.\n",
				          i);
			}
		});
	}

	while (true) {
		int cnt = 0;
		std::cin >> cnt;
		if (cnt <= 0) {
			running = false;
			send_num = 0;
			sig_cv.notify_all();
			break;
		} else {
			send_num = cnt;
			sig_cv.notify_all();
		}
	}

	for (auto &socket : pub_sockets)
		socket->close();

	ctx.close();

	for (auto &worker : workers)
		worker.join();

	return 0;
}
