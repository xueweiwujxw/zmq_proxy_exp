/**
 * @file zmq_proxy.cc
 * @author wlanxww (xueweiwujxw@outlook.com)
 * @brief
 * @date 2025-05-30
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <atomic>
#include <signal.h>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <sys/stat.h>

#include <zmq.hpp>

#include <log.h>
#include <defines.hpp>

std::mutex sig_mutex;
std::condition_variable sig_cv;

void sigint_cb_handler(int signum) { sig_cv.notify_all(); }

void sigterm_cb_handler(int signum) { sig_cv.notify_all(); }

bool set_ipc_permissions(const char *endpoint)
{
	const char *prefix = "ipc://";
	size_t prefix_len = strlen(prefix);

	if (strncmp(endpoint, prefix, prefix_len) != 0) {
		logf_err("Invalid IPC endpoint: %s\n", endpoint);
		return false;
	}

	const char *filepath = endpoint + prefix_len;

#ifndef _WIN32
	struct stat st;
	if (stat(filepath, &st) != 0) {
		logf_err("IPC file not found: %s (%s)\n", filepath,
		         strerror(errno));
		return false;
	}

	if (chmod(filepath, S_IRWXU | S_IRWXG | S_IRWXO) != 0) {
		logf_err("Failed to set permissions for %s: %s\n", filepath,
		         strerror(errno));
		return false;
	}

	logf_info("Set global permissions for %s\n", filepath);
#endif

	return true;
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

	// xsub
	zmq::socket_t xsub_socket(ctx, ZMQ_XSUB);

	try {
		xsub_socket.bind(FIXED_ZMQ_XSUB_PATH);
		logf_info("XSUB bound to %s\n", FIXED_ZMQ_XSUB_PATH);
	} catch (const std::exception &e) {
		logf_err("%s\n", e.what());
		return 1;
	}

	// xpub
	zmq::socket_t xpub_socket(ctx, ZMQ_XPUB);

	try {
		xpub_socket.bind(FIXED_ZMQ_XPUB_PATH);
		xpub_socket.setsockopt(ZMQ_XPUB_VERBOSER, 1);
		logf_info("XPUB bound to %s\n", FIXED_ZMQ_XPUB_PATH);
	} catch (const std::exception &e) {
		logf_err("%s\n", e.what());
		return 1;
	}

	// Optional
	if (!set_ipc_permissions(FIXED_ZMQ_XSUB_PATH))
		return 1;
	if (!set_ipc_permissions(FIXED_ZMQ_XPUB_PATH))
		return 1;

	std::thread proxy_thread([&]() {
		try {
			logf_info("Starting proxy...\n");
			zmq::proxy(xsub_socket, xpub_socket);
		} catch (const zmq::error_t &e) {
			if (e.num() != ETERM && e.num() != ENOTSOCK)
				logf_err("Proxy error: %s %d\n", e.what(),
				         e.num());
			else
				logf_info("Proxy terminated normally\n");
		}
	});

	logf_info("Proxy running. Press Ctrl+C to exit...\n");

	std::unique_lock<std::mutex> lock(sig_mutex);
	sig_cv.wait(lock);

	xsub_socket.close();
	xpub_socket.close();

	ctx.close();

	logf_info("Shutting down...\n");

	proxy_thread.join();

	logf_info("Clean exit\n");
	lock.unlock();

	return 0;
}
