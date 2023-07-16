#include <iostream>
#include "HTTPClient.h"

int main()
{
    std::string serverURL;
    std::cout << "Enter the URL you want to retrieve: ";
    std::getline(std::cin, serverURL);

    try
    {
        HTTPClient httpClient;
        httpClient.getResponse(serverURL);
    }
    catch (const std::runtime_error& error)
    {
        std::cerr << error.what() << std::endl;
        return EXIT_FAILURE;
    }

	return EXIT_SUCCESS;
}