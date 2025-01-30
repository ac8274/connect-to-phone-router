#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment (lib, "ws2_32.lib")
#include <WinSock2.h>
#include <Windows.h>
#include <iostream>
#include <string>
#include <iphlpapi.h>

#pragma comment(lib, "iphlpapi.lib")  // Link with the IP Helper API library

#define SERVER_PORT 4454

void pushDouble(double src, char* dest, int offset);
double extractDouble(char* buffer, int offset);
std::string getRouterIp();
int main()
{

    std::string routerIp = getRouterIp();
    std::string input = "";
    std::cout << "Type 'START' when you have started the hotspot on the phone" << std::endl;
    while (input != "START")
    {
        std::cout << "Enter Command: ";
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
    std::cout << "If wish to stop enter 'STOP' else Enter a letter (any letter)" << std::endl;
    double currentLatatiude = 20.4;
    double currentLongtatiude = 21.6;
    double currentElevation = 2.9;
    while(hello != "STOP")
    {
        char buffer[4096];

        pushDouble(currentLatatiude, buffer, 0);
        pushDouble(currentLongtatiude, buffer, sizeof(double));
        pushDouble(currentElevation, buffer, sizeof(double) * 2);

        send(clientSocket, buffer, 4096, 0);

        recv(clientSocket, buffer, 4096, 0);

        currentLatatiude += extractDouble(buffer, 0);
        currentLongtatiude += extractDouble(buffer, sizeof(double));
        currentElevation += extractDouble(buffer, sizeof(double)*2);
        
        std::cin >> hello;
    }

    // Close socket
    closesocket(clientSocket);

    // Clean up Winsock
    WSACleanup();

    return 0;
}

void pushDouble(double src,char* dest, int offset)
{
    memcpy(dest+ offset, &src, sizeof(double));
}

double extractDouble(char* buffer,int offset)
{
    double data = 0;
    memcpy(&data, buffer + offset, sizeof(double));
    return data;
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