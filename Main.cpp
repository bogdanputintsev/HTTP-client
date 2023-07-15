#include <iostream>
#include <string>
#include <sstream>
#include <winsock2.h>
#include <Ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

//188.166.14.102 80

void* getSinAddr(addrinfo* addr)
{
	switch (addr->ai_family)
	{
	case AF_INET:
		return &(reinterpret_cast<sockaddr_in*>(addr->ai_addr)->sin_addr);

	case AF_INET6:
		return &(reinterpret_cast<sockaddr_in6*>(addr->ai_addr)->sin6_addr);
	}

	return NULL;
}

int main()
{
    std::string serverURL = "example.com";
    int serverPort = 80;

    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Failed to initialize Winsock" << std::endl;
        return -1;
    }

    // Get IP address from the provided URL
    struct addrinfo hints {};
    struct addrinfo* serverAddressInfo;

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;

    int result = getaddrinfo(serverURL.c_str(), nullptr, &hints, &serverAddressInfo);
    if (result != 0) {
        std::cerr << "Failed to get IP address: " << gai_strerrorA(result) << std::endl;
        WSACleanup();
        return -1;
    }

    // Retrieve the IP address
    char ipAddress[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &((struct sockaddr_in*)(serverAddressInfo->ai_addr))->sin_addr, ipAddress, INET_ADDRSTRLEN);
    freeaddrinfo(serverAddressInfo);

    // Create a TCP socket
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Failed to create socket" << std::endl;
        WSACleanup();
        return -1;
    }

    // Set up the server details
    struct sockaddr_in serverAddress {};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(serverPort);
    if (inet_pton(AF_INET, ipAddress, &(serverAddress.sin_addr)) <= 0) {
        std::cerr << "Invalid address/Address not supported" << std::endl;
        closesocket(sock);
        WSACleanup();
        return -1;
    }

    // Connect to the server
    if (connect(sock, reinterpret_cast<struct sockaddr*>(&serverAddress), sizeof(serverAddress)) == SOCKET_ERROR) {
        std::cerr << "Connection failed" << std::endl;
        closesocket(sock);
        WSACleanup();
        return -1;
    }

    // Send an HTTP request
    std::string request = "GET / HTTP/1.1\r\n"
        "Host: " + serverURL + "\r\n"
        "Connection: close\r\n\r\n";

    if (send(sock, request.c_str(), static_cast<int>(request.length()), 0) == SOCKET_ERROR) {
        std::cerr << "Failed to send request" << std::endl;
        closesocket(sock);
        WSACleanup();
        return -1;
    }

    // Receive and print the response
    const int bufferSize = 4096;
    char buffer[bufferSize];
    std::stringstream response;

    while (true) {
        memset(buffer, 0, bufferSize);
        int bytesRead = recv(sock, buffer, bufferSize - 1, 0);
        if (bytesRead <= 0) {
            break;
        }
        response << buffer;
    }

    std::cout << response.str() << std::endl;

    // Close the socket
    closesocket(sock);
    WSACleanup();

	return EXIT_SUCCESS;
}