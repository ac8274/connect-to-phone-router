#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment (lib, "ws2_32.lib")
#include <WinSock2.h>
#include <Windows.h>
#include <iostream>
#include <string>
#include <iphlpapi.h>

#pragma comment(lib, "iphlpapi.lib")  // Link with the IP Helper API library

#define SERVER_IP "192.168.50.186"
#define SERVER_PORT 4454


std::string getRouterIp();
int main()
{

    std::string routerIp = getRouterIp();
    std::string input = "";
    while (input != "START")
    {
        std::cin >> input;
    }


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
    serverAddress.sin_addr.s_addr = inet_addr(routerIp.c_str());

    // Send connection request
    result = connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
    if (result == SOCKET_ERROR) {
        std::cerr << "Connection failed: " << WSAGetLastError() << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }
    
    std::string hello;
    while(hello != "STOP")
    {
        char buffer[4096];
        std::string str = "The Number is:  1234.2222341234";
        send(clientSocket, str.c_str(), str.length(), 0);
        recv(clientSocket, buffer, 4096, 0);
        for (int i = 0; i < 4096; i++)
        {
            if (buffer[i] <= 31 || buffer[i] >= 127)
            {
                buffer[i] = '\0';
            }
        }
        std::string message(buffer);
        std::cout << "message sent: " << message << std::endl;
        std::cin >> hello;
    }

    // Close socket
    closesocket(clientSocket);

    // Clean up Winsock
    WSACleanup();

    return 0;
}


std::string getRouterIp()
{
    std::string ans = "";
    DWORD dwSize = 0;
    PIP_ADAPTER_INFO pAdapterInfo = NULL, pAdapter = NULL;
    DWORD dwRetVal;

    if (GetAdaptersInfo(NULL, &dwSize) == ERROR_BUFFER_OVERFLOW)
    {
        pAdapterInfo = (IP_ADAPTER_INFO*)malloc(dwSize);
        if (NULL == pAdapterInfo)
        {
            printf("Memory allocation failed.\n");
            return "";
        }
    }
    else
    {
        printf("Failed to retrieve buffer size for adapter info.\n");
        return "";
    }

    dwRetVal = GetAdaptersInfo(pAdapterInfo, &dwSize);
    if (NO_ERROR != dwRetVal)
    {
        printf("GetAdaptersInfo failed with error: %d\n", dwRetVal);
        free(pAdapterInfo);
        return "";
    }

    pAdapter = pAdapterInfo;
    while (ans == "" && pAdapter)
    {
        printf("Adapter Name: %s\n", pAdapter->AdapterName);

        if (strlen(pAdapter->GatewayList.IpAddress.String) > 0 && std::string(pAdapter->GatewayList.IpAddress.String) != "0.0.0.0") {
            ans = std::string(pAdapter->GatewayList.IpAddress.String);
        }
        else {
            printf("No Default Gateway for this adapter.\n");
        }

        printf("\n");
        pAdapter = pAdapter->Next;
    }
    free(pAdapterInfo);
    return ans;
}