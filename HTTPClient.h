#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <winsock2.h>
#include <Ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

class HTTPClient final
{
public:
	HTTPClient();
	~HTTPClient();

	void getResponse(const std::string& http);
private:
	void initWsa();
	void initSocket();
	std::string getIpAddressFromUrl(const std::string& url);
	void setupServerDetails(const std::string& ipAddress);
	void connectToServer();
	void sendHttpRequest(const std::string& url);
	void receiveAndPrintResponse();
	
	static constexpr int PORT = 80;
	static constexpr int CODE_SUCCESS = 0;

	WSADATA wsaData;
	SOCKET sock;
	struct addrinfo hints {};
	struct addrinfo* serverAddressInfo;
	struct sockaddr_in serverAddress {};

	bool wsaInitialized = false;
	bool socketInitialized = false;
};

