#include <iostream>
#include <ctime>
#include <string>
#include <sstream>
#include <algorithm>
#include <cstring>
#include <vector>
#include <thread>
#ifdef _WIN32
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#elif __linux__
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#endif

#ifdef __linux
#define SOCKET int
#endif

const int first_avalible_port(std::vector<int>& vec)
{
	sort(vec.begin(), vec.end());
	for (size_t i = 0; i < vec.size() - 1; i++)
	{
		if (vec[i] + 1 != vec[i + 1])
		{
			vec.push_back(vec[i] + 1);
			return vec[i] + 1;
		}
	}
	// no gaps
	vec.push_back(vec[vec.size() - 1] + 1);
	return vec[vec.size() - 1];
}

SOCKET setup_listening(const std::string& ipAddress, const int& listening_port)
{
	WSAData data;
	WORD ver = MAKEWORD(2, 2);
	int wsResult = WSAStartup(ver, &data);
	if (wsResult != 0)
	{
		std::cerr << "Can't start Winsock, Err #" << wsResult << std::endl;
		exit(1);
	}

	SOCKET listening = socket(AF_INET, SOCK_STREAM, 0);
	if (listening == INVALID_SOCKET)
	{
		std::cerr << "Can't create a socket! Quitting" << std::endl;
		exit(0);
	}


	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(listening_port);
	hint.sin_addr.S_un.S_addr = INADDR_ANY;

	bind(listening, (sockaddr*)&hint, sizeof(hint));

	int lsResult = listen(listening, SOMAXCONN);
	if (lsResult != 0)
	{
		std::cerr << "Can't establish a listening, Err #" << lsResult << std::endl;
		exit(1);
	}

	return listening;
}

void sending(const SOCKET& s)
{
	char sendbuf[] = "Hello World!";
	while (true)
	{
		Sleep(1000);
		send(s, sendbuf, sizeof(sendbuf), 0);
	}
}
void receiving(const SOCKET& s)
{
	char recvbuf[1024];
	for (char& c : recvbuf)
	{
		c = '\0';
	}
	while (true)
	{
		recv(s, recvbuf, sizeof(recvbuf), 0);
		// handle the received data here
		printf("%s\r\n", recvbuf);
		/* code */
	}

// closes the socket after receiving a afk ping
	closesocket(s);
	WSACleanup();
}



int main()
{
	std::string ipAddress = "127.0.0.1";
	std::vector<int> starting_port;
	starting_port.push_back(54000); // linux_port
	// the client leaving should send which port they used so it could be removed from the vector.
	// count ports until one that isn't in use appears.

	SOCKET listening = setup_listening(ipAddress, starting_port[0]);

	/*SOCKET client = accept(listening, nullptr, nullptr);
	printf("first connection established!\r\n");
	SOCKET client2 = accept(listening, nullptr, nullptr);
	printf("second connection established!\r\n");
	. . .
	*/

	std::vector<std::thread> threads;

	while (true)
	{
		SOCKET client = accept(listening, nullptr, nullptr);

		threads.push_back(std::thread(sending, client));
		threads.push_back(std::thread(receiving, client));
	}

	// when all threads have joined
	std::cin.get();
}
