#include "HTTPClient.h"
#include <future>
#include <iostream>
#include <ws2ipdef.h>
#include <Ws2tcpip.h>

HTTPClient::HTTPClient()
{
    initWsa();
    initSocket();
}

HTTPClient::~HTTPClient()
{
    if (socketInitialized)
    {
        closesocket(sock);
    }
    if (wsaInitialized)
    {
        WSACleanup();
    }
}

void HTTPClient::initWsa()
{
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        throw std::runtime_error("HTTPClient: Failed to initialize Winsock");
    }

    wsaInitialized = true;
}

void HTTPClient::initSocket()
{
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET)
    {
        throw std::runtime_error("HTTPClient: Failed to create socket");
    }

    socketInitialized = true;
}

void HTTPClient::getResponse(const std::string& url)
{
    const std::string ipAddress{ getIpAddressFromUrl(url) };
    setupServerDetails(ipAddress);
    connectToServer();
    sendHttpRequest(url);
    receiveAndPrintResponse();
}

std::string HTTPClient::getIpAddressFromUrl(const std::string& url)
{
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;

    const int result = getaddrinfo(url.c_str(), nullptr, &hints, &serverAddressInfo);
    if (result != CODE_SUCCESS)
    {
        std::cerr << "Failed to get IP address: " << gai_strerror(result) << std::endl;
        throw std::runtime_error("HTTPClient: Failed to get IP address");
    }

    char ipAddress[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &reinterpret_cast<sockaddr_in*>(serverAddressInfo->ai_addr)->sin_addr, ipAddress, INET_ADDRSTRLEN);
    freeaddrinfo(serverAddressInfo);

    return std::string{ ipAddress };
}

void HTTPClient::setupServerDetails(const std::string& ipAddress)
{
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(PORT);
    if (inet_pton(AF_INET, ipAddress.c_str(), &(serverAddress.sin_addr)) <= CODE_SUCCESS)
    {
        throw std::runtime_error("HTTPClient: Invalid address/Address not supported");
    }
}

void HTTPClient::connectToServer()
{
    if (connect(sock, reinterpret_cast<struct sockaddr*>(&serverAddress), sizeof(serverAddress)) == SOCKET_ERROR)
    {
        throw std::runtime_error("HTTPClient: Connection failed");
    }
}

void HTTPClient::sendHttpRequest(const std::string& url) const
{
    const std::string request = "GET / HTTP/1.1\r\n"
        "Host: " + url + "\r\n"
        "Connection: close\r\n\r\n";

    if (send(sock, request.c_str(), static_cast<int>(request.length()), 0) == SOCKET_ERROR)
    {
        throw std::runtime_error("HTTPClient: Failed to send request");
    }
}

void HTTPClient::receiveAndPrintResponse()
{
	WSAPOLLFD wsaDescriptor;
	wsaDescriptor.fd = sock;
	wsaDescriptor.events = POLLIN;
    wsaDescriptor.revents = 0;

	const auto receiveJob = std::async(std::launch::async, &HTTPClient::receiveMessage, this);
	bool nextMessageAvailable = false;
    bool spaceButtonWasPressed = false;

	while (true)
	{
		const int pollResult = WSAPoll(&wsaDescriptor, 1, 1);
        if (pollResult == SOCKET_ERROR)
        {
            throw std::runtime_error("Socket error occurred.");
        }

        if(pollResult > 0 && wsaDescriptor.revents & POLLIN)
		{
			std::cout << "\nNew package received.\n";
			if (!nextMessageAvailable)
			{
                printf("Press sp ace to display next message...\n");
			}
			nextMessageAvailable = true;
		}

        if(isSpaceButtonDown())
        {
            spaceButtonWasPressed = true;
        }

        if (spaceButtonWasPressed && isSpaceButtonUp() && nextMessageAvailable)
        {
            spaceButtonWasPressed = false;

            const auto jobStatus = receiveJob.wait_for(std::chrono::milliseconds(0));

            std::unique_lock<std::mutex> lock(messageQueueMutex);
            if (messageQueue.empty())
            {
                lock.unlock();  
                nextMessageAvailable = false;
                if (jobStatus == std::future_status::ready)
                {
                    break;
                }

                printf("\nWaiting for new packages...\n");
            }
            else
            {
                system("CLS");
                printf("%s\n", messageQueue.front().c_str());
                messageQueue.pop();
            }
		}
	}
}

bool HTTPClient::isSpaceButtonDown()
{
    return GetAsyncKeyState(VK_SPACE) & 0x8000;
}

bool HTTPClient::isSpaceButtonUp()
{
    return GetAsyncKeyState(VK_SPACE) == 0;
}

void HTTPClient::receiveMessage()
{
    char buffer[BUFFER_SIZE];

    auto timeOfLastReceivedMessage = std::chrono::system_clock::now();

    while (true)
    {
        const auto now = std::chrono::system_clock::now();
        const auto secondsAfterLastMessage = std::chrono::duration_cast<std::chrono::seconds>(now - timeOfLastReceivedMessage).count();

        if (secondsAfterLastMessage > LAG_DELAY_SECONDS)
        {
            timeOfLastReceivedMessage = std::chrono::system_clock::now();
            memset(buffer, 0, BUFFER_SIZE);
            const int bytesRead = recv(sock, buffer, BUFFER_SIZE - 1, 0);
            if (bytesRead <= 0)
            {
                break;
            }

            std::lock_guard<std::mutex> lock(messageQueueMutex);
            messageQueue.push(buffer);
        }
        
    }
}
