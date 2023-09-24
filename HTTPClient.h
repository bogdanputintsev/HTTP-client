#pragma once

#include <mutex>
#include <string>
#include <winsock2.h>
#include <queue>

#pragma comment(lib, "ws2_32.lib")

class HTTPClient final
{
public:
	HTTPClient();
	~HTTPClient();
	HTTPClient(HTTPClient&) = delete;
	HTTPClient(HTTPClient&&) = delete;
	HTTPClient& operator=(const HTTPClient&) = delete;
	HTTPClient& operator=(HTTPClient&&) = delete;

	void getResponse(const std::string& url);
private:
	static constexpr int PORT = 80;
	static constexpr int CODE_SUCCESS = 0;
	static constexpr int BUFFER_SIZE = 256;
	static constexpr long long LAG_DELAY_SECONDS = 5;

	void initWsa();
	void initSocket();
	std::string getIpAddressFromUrl(const std::string& url);
	void setupServerDetails(const std::string& ipAddress);
	void connectToServer();
	void sendHttpRequest(const std::string& url) const;
	void receiveAndPrintResponse();

	static bool isSpaceButtonDown();
	static bool isSpaceButtonUp();
	void receiveMessage();

	WSADATA wsaData{};
	SOCKET sock{};
	addrinfo hints {};
	addrinfo* serverAddressInfo{};
	sockaddr_in serverAddress {};

	bool wsaInitialized = false;
	bool socketInitialized = false;

	std::mutex messageQueueMutex;
	std::queue<std::string> messageQueue;

};

