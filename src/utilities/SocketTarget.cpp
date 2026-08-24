//
// Created by Grzegorz on 8/23/2026.
//

#if defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#include <print>
#include <string>
#include <thread>
#include <iostream>

namespace
{
    void serverThread(SOCKET listenSock)
    {
        sockaddr_in clientAddr{};
        int         clientAddrLen = sizeof( clientAddr );
        SOCKET      client        = accept(listenSock, reinterpret_cast<sockaddr*>(&clientAddr), &clientAddrLen);
        if (client == INVALID_SOCKET)
        {
            return;
        }

        char buf[512];
        while (true)
        {
            int n = recv(client, buf, sizeof( buf ), 0);

            if (n <= 0)
            {
                break;
            }

            send(client, buf, n, 0);
        }

        closesocket(client);
    }
}

int main()
{
    std::print("victim_net.exe PID = {}\n", GetCurrentProcessId());

    WSADATA wsaData{};
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        std::print("WSAStartup failed\n");
        return EXIT_FAILURE;
    }

    SOCKET listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSock == INVALID_SOCKET)
    {
        std::print("socket() failed\n");
        return EXIT_FAILURE;
    }

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port        = 0; // ephemeral -- let the OS pick a free port

    if (bind(listenSock, reinterpret_cast<sockaddr*>(&addr), sizeof( addr )) == SOCKET_ERROR)
    {
        std::print("bind() failed\n");
        return EXIT_FAILURE;
    }

    if (listen(listenSock, 1) == SOCKET_ERROR)
    {
        std::print("listen() failed\n");
        return EXIT_FAILURE;
    }

    int boundAddrLen = sizeof( addr );
    getsockname(listenSock, reinterpret_cast<sockaddr*>(&addr), &boundAddrLen);
    unsigned short port = ntohs(addr.sin_port);

    std::thread server(serverThread, listenSock);
    server.detach();

    SOCKET      clientSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in serverAddr{};
    serverAddr.sin_family      = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddr.sin_port        = htons(port);

    if (connect(clientSock, reinterpret_cast<sockaddr*>(&serverAddr), sizeof( serverAddr )) == SOCKET_ERROR)
    {
        std::print("connect() failed\n");
        return EXIT_FAILURE;
    }

    std::print("Connected loopback socket on 127.0.0.1:{}\n", port);
    std::print("Press Enter to send a message over the socket, or type 'quit' to exit.\n");

    std::string line;
    while (std::getline(std::cin, line))
    {
        if (line == "quit")
        {
            break;
        }

        std::string message = "hello from victim_net";
        send(clientSock, message.c_str(), static_cast<int>(message.size()), 0);

        char buf[512];
        int  n = recv(clientSock, buf, sizeof( buf ), 0);
        if (n > 0)
        {
            std::print("Echoed back {} bytes: {}\n", n, std::string(buf, n));
        }
        else
        {
            std::print("recv() returned {}\n", n);
        }
    }

    closesocket(clientSock);
    closesocket(listenSock);
    WSACleanup();

    return EXIT_SUCCESS;
}
#endif
