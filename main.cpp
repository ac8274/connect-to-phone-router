#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _USE_MATH_DEFINES
#pragma comment (lib, "ws2_32.lib")
#include <WinSock2.h>
#include <Windows.h>
#include <ws2tcpip.h>
#include <iostream>
#include <cstdint>      // For `uint64_t`
#include <cstring>      // For `memcpy`
#include <string>
#include <iphlpapi.h>
#include <cmath>

#pragma comment(lib, "iphlpapi.lib")  // Link with the IP Helper API library

#define SERVER_PORT 4454
#define EARTH_RADIUS 6371000.0  // Earth's radius in meters
#define DEG_TO_RAD (M_PI / 180.0)
#define RAD_TO_DEG (180.0 / M_PI)
#define SECOND 1000

bool isSocketOpen(int socket);
void pushDouble(double src, char* dest, int offset);
std::string getRouterIp();
uint64_t swapEndianness(uint64_t value);
double receiveDouble(int clientSocket);
void calculateNewPosition(double distance, double bearing, double& newLat, double& newLon);
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
    
    double currentLatatiude = 31.27379972458028;
    double currentLongtatiude = 34.80235696568683;
    double currentElevation = 2.9;
    int connection_status = 0;
    int wait_time = 2 * SECOND;
    try {
        do
        {
            char buffer[4096] = { 0 };

            pushDouble(currentLatatiude, buffer, 0);
            pushDouble(currentLongtatiude, buffer, sizeof(uint64_t));
            pushDouble(currentElevation, buffer, sizeof(uint64_t) * 2);

            send(clientSocket, buffer, 24, 0);

            std::cout << "------------------------------------------" << std::endl;

            double distance = receiveDouble(clientSocket);
            double angle = receiveDouble(clientSocket);
            currentElevation += receiveDouble(clientSocket);

            calculateNewPosition(distance, angle, currentLatatiude, currentLongtatiude);

            std::cout << "Recived:\nLatatiude: " << currentLatatiude << "\nLongtatiude: " << currentLongtatiude << "\nElevation: " << currentElevation << "\n------------------------------------------" << std::endl;

            Sleep(wait_time); // wait 2 seconds before sending back the data.

        } while (isSocketOpen(clientSocket)); //check if server closed connection.
    }
    catch (const std::invalid_argument& e)
    {
        // Close socket
        closesocket(clientSocket);

        // Clean up Winsock
        WSACleanup();
    }
    
    return 0;
}

void pushDouble(double src,char* dest, int offset)
{
    uint64_t temp;
    memcpy(&temp, &src, sizeof(temp));
    temp = swapEndianness(temp); // swap location back to big endianss.
    memcpy(dest+ offset, &temp, sizeof(temp));
}

uint64_t swapEndianness(uint64_t value) {
    return _byteswap_uint64(value);  // Swap to match Java's Big-Endian order
}

double receiveDouble(int clientSocket) 
{

    char buffer[8];  // Buffer to receive raw bytes
    int recived = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (recived > 0)
    {
        uint64_t temp;
        memcpy(&temp, buffer, sizeof(temp));  // Copy bytes into uint64_t

        temp = swapEndianness(temp);  // Convert from Java's Big-Endian to Windows' Little-Endian

        double receivedValue;
        memcpy(&receivedValue, &temp, sizeof(receivedValue));  // Copy into double

        return receivedValue;
    }
    else
    {
        throw std::invalid_argument("Socket Closed");
    }
}

void calculateNewPosition(double distance, double bearing, double& newLat, double& newLon) {
    double lat = newLat * DEG_TO_RAD;  // Convert to radians
    double lon = newLon * DEG_TO_RAD;
    bearing *= DEG_TO_RAD;

    double delta = distance / EARTH_RADIUS;

    // New latitude calculation
    newLat = asin(sin(lat) * cos(delta) + cos(lat) * sin(delta) * cos(bearing));

    // New longitude calculation
    newLon = lon + atan2(sin(bearing) * sin(delta) * cos(lat), cos(delta) - sin(lat) * sin(newLat));

    // Convert back to degrees
    newLat *= RAD_TO_DEG;
    newLon *= RAD_TO_DEG;
}

// Function to check if the socket is still open
bool isSocketOpen(int socket) {
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(socket, &readfds);

    timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    int result = select(0, &readfds, NULL, NULL, &timeout);
    if (result > 0) {
        char buffer[1];
		int recvResult = recv(socket, buffer, sizeof(buffer), MSG_PEEK); // MSG_PEEL: check the data without removing it from the buffer.
        if (recvResult == 0) {
            return false; // Connection closed
        }
    }
    else
    {
        return result == 0;
    }
    return true; // Connection is still open
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