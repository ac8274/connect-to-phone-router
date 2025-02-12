#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment (lib, "ws2_32.lib")
#include <WinSock2.h>
#include <Windows.h>
#include <iostream>
#include <cstdint>      // For `uint64_t`
#include <cstring>      // For `memcpy`
#include <string>
#include <iphlpapi.h>

#pragma comment(lib, "iphlpapi.lib")  // Link with the IP Helper API library

#define SERVER_PORT 4454

void pushDouble(double src, char* dest, int offset);
double extractDouble(char* buffer, int offset);
std::string getRouterIp();
uint64_t swapEndianness(uint64_t value);
double receiveDouble(int clientSocket);
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
    int connection_status = 0;
    do
    {
        char buffer[4096] = { 0 };

        pushDouble(currentLatatiude, buffer, 0);
        pushDouble(currentLongtatiude, buffer, sizeof(uint64_t));
        pushDouble(currentElevation, buffer, sizeof(uint64_t) * 2);

        send(clientSocket, buffer, 4096, 0);

        std::cout << "------------------------------------------" << std::endl;

        currentLatatiude += receiveDouble(clientSocket);
        currentLongtatiude += receiveDouble(clientSocket);
        currentElevation += receiveDouble(clientSocket);

        std::cout << "Recived:\nLatatiude: " << currentLatatiude << "\nLongtatiude: " << currentLongtatiude << "\nElevation: " << currentElevation << "\n------------------------------------------" << std::endl;

        //Sleep(2000); // wait 5 seconds before sending back the data.

        char temp_buffer[1];
        result = recv(clientSocket, buffer, sizeof(buffer), MSG_PEEK);
    } while (result != 0 && result!= SOCKET_ERROR);

    // Close socket
    closesocket(clientSocket);

    // Clean up Winsock
    WSACleanup();

    return 0;
}

void pushDouble(double src,char* dest, int offset)
{
    uint64_t temp;
    memcpy(&temp, &src, sizeof(temp));
    temp = swapEndianness(temp); // swap location back to big endianss.
    memcpy(dest+ offset, &temp, sizeof(temp));
}

double extractDouble(char* buffer,int offset)
{
    double data = 0;
    memcpy(&data, buffer + offset, sizeof(double));
    return data;
}

uint64_t swapEndianness(uint64_t value) {
    return _byteswap_uint64(value);  // Swap to match Java's Big-Endian order
}

double receiveDouble(int clientSocket) 
{
    char buffer[8];  // Buffer to receive raw bytes
    recv(clientSocket, buffer, sizeof(buffer), 0);

    uint64_t temp;
    memcpy(&temp, buffer, sizeof(temp));  // Copy bytes into uint64_t

    temp = swapEndianness(temp);  // Convert from Java's Big-Endian to Windows' Little-Endian

    double receivedValue;
    memcpy(&receivedValue, &temp, sizeof(receivedValue));  // Copy into double

    std::cout << "recived: " << receivedValue << std::endl;
    
    return receivedValue;
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