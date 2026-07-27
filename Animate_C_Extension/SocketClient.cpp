#include "SocketClient.h"
#include "PacketProcessor.h"

#include <iostream>
#include <string>

SocketClient* SocketClient::instance = nullptr;
std::mutex SocketClient::instanceMutex;

SocketClient::SocketClient() : sock(INVALID_SOCKET), connected(false) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed\n";
    }
}

SocketClient::~SocketClient() {
    Disconnect();
    WSACleanup();
}

SocketClient& SocketClient::GetInstance() {
    static SocketClient instance;
    return instance;
}

bool SocketClient::Connect(int port) {
    if (connected) return true;

    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Socket creation failed: " << WSAGetLastError() << "\n";
        return false;
    }

    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    inet_pton(AF_INET, "127.0.0.1", &serverAddress.sin_addr);

    if (connect(sock, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
        std::cerr << "Connection failed: " << WSAGetLastError() << "\n";
        closesocket(sock);
        sock = INVALID_SOCKET;
        return false;
    }

    connected = true;
    receiveThread = std::thread(&SocketClient::receiveLoop, this);
    return true;
}

void SocketClient::Disconnect() {
    if (!connected) return;

    connected = false;
    shutdown(sock, SD_BOTH);
    closesocket(sock);
    sock = INVALID_SOCKET;

    if (receiveThread.joinable()) {
        receiveThread.join();
    }
}

bool SocketClient::SendData(const std::string& data) {
    if (!connected) return false;

    std::string str = data + '\n';
    int sentBytes = send(sock, str.c_str(), static_cast<int>(str.size()), 0);

    return sentBytes == static_cast<int>(str.size());
}

void SocketClient::receiveLoop() {
    char buffer[1024]{};
    while (connected) {
        int received = recv(sock, buffer, sizeof(buffer), 0);

        if (received > 0) {
            std::string data(buffer, received);

            PacketProcessor::GetInstance().AppendData(data);
            SendData("Done");

            for (const auto& val : PacketProcessor::GetInstance().GetParsedDataByName("Test")) {
                std::cout << "받은 데이터 : " << val << '\n';
            }
        }
        else if (received == 0) {
            std::cerr << "Connection closed by the server\n";
            connected = false;
        }
        else {
            std::cerr << "Receive failed: " << WSAGetLastError() << "\n";
            connected = false;
        }
    }
}
