#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment (lib, "ws2_32.lib")
#include <WinSock2.h>
#include <Windows.h>
#include <iostream>
#include <string>

#define SERVER_IP "192.168.115.145"
#define SERVER_PORT 4454

int main()
{
    // Initialize Winsock
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed: " << result << std::endl;
        return 1;
    }

    // Create socket
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    // Specifying address
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(SERVER_PORT);
    serverAddress.sin_addr.s_addr = inet_addr(SERVER_IP);

    // Send connection request
    result = connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
    if (result == SOCKET_ERROR) {
        std::cerr << "Connection failed: " << WSAGetLastError() << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }
    char buffer[1024];
    std::string str = "bye\n";
    send(clientSocket, str.c_str(), str.length(), 0);
    recv(clientSocket, buffer, 1024, 0);


    // Close socket
    closesocket(clientSocket);

    for (int i = 0; i < 1024; i++)
    {
        if (buffer[i] <=31 or buffer[i] >=127)
        {
            buffer[i] = '\0';
        }
    }
    std::string message(buffer);
    printf("message sendt:%s", message.c_str());

    // Clean up Winsock
    WSACleanup();

    return 0;
}