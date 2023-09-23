#include "HTTPClient.h"
#include <future>

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

    int result = getaddrinfo(url.c_str(), nullptr, &hints, &serverAddressInfo);
    if (result != CODE_SUCCESS)
    {
        std::cerr << "Failed to get IP address: " << gai_strerrorA(result) << std::endl;
        throw std::runtime_error("HTTPClient: Failed to get IP address");
    }

    char ipAddress[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &((struct sockaddr_in*)(serverAddressInfo->ai_addr))->sin_addr, ipAddress, INET_ADDRSTRLEN);
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

void HTTPClient::sendHttpRequest(const std::string& url)
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
    WSAPOLLFD wsaDescriptor{};
    wsaDescriptor.fd = sock;
    wsaDescriptor.events = POLLIN;
    wsaDescriptor.revents = 0;

    //std::future<void> receiveJob = std::async(&HTTPClient::receiveMessage, this, std::launch::deferred);

    bool nextMessageAvailable = false;
    
    while (true) 
    {
        int result = WSAPoll(&wsaDescriptor, 1, 1);
        if (result > 0 && wsaDescriptor.revents & POLLIN)
        {
            if (!nextMessageAvailable)
            {
                std::cout << "Press space to display next message...\n";
            }
            std::cout << "New package received.\n";
            nextMessageAvailable = true;
        }

        if (isSpaceButtonPressed() && nextMessageAvailable)
        {
            system("CLS");
            std::cout << messageQueue.front() << std::endl;
            messageQueue.pop();
            if (messageQueue.empty())
            {
                nextMessageAvailable = false;
            }
        }
    }
}

bool HTTPClient::isSpaceButtonPressed() const
{
    return GetKeyState(VK_SPACE) & 0x8000;
}

void HTTPClient::receiveMessage()
{
    const int bufferSize = 128;
    char buffer[bufferSize];

    while (true)
    {
        memset(buffer, 0, bufferSize);
        int bytesRead = recv(sock, buffer, bufferSize - 1, 0);
        if (bytesRead <= 0)
        {
            break;
        }

        messageQueue.push(buffer);
    }
}
