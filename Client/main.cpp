
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
//Если с библиотекой <WinSOCK2.h> подключается файл <Windows.h> или <IPhlpAPI>,
//то они тоже подключают файл <WinSOCK2.h>, что приводит к конфликтам.
//Для того чтобы <Windows.h> и <IPhlpAPI.h> не подтягивали WinSOCK, создается макроопределение.
#endif // !WIN32_LEAN_AND_MEAN


#include<iostream>
#include<Windows.h>
#include<WinSock2.h>
#include<WS2tcpip.h>
#include<iphlpapi.h>
using namespace  std;

#pragma comment(lib, "WS2_32.lib")	//Вставляем статическую библиотеку, для заголовка <WS2TCPIP.h>

#define MTU	1500	//Maximum Transfer Unit - Максимально-возможный размер Ethernet-кадра

void main()
{
	setlocale(LC_ALL, "");

	WSAData wsaData;
	int iResult = 0;
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		cout << "WinSOCK init failed with code: " << iResult;
		return;
	}
	//2) Определяем параметры подключения:
	addrinfo  hints;
	addrinfo* target;
	ZeroMemory(&hints, sizeof(hints));	//Обнуляем экземпляр стуктуры
	hints.ai_family = AF_INET;			//Стек протоколов TCP/IPv4
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;	//Определяем протокол транспортного уровня
	iResult = getaddrinfo("127.0.0.1", "27015", &hints, &target);
	if (iResult != 0)
	{
		cout << "getaddressinfo() failed with code " << iResult << endl;
		freeaddrinfo(target);
		WSACleanup();
		return;
	}

	//3) Создаем сокет:
	//SOCKET - тип данных;
	//socket() - это функция;
	SOCKET connect_socket =
		socket(target->ai_family, target->ai_socktype, target->ai_protocol);
	if (connect_socket == INVALID_SOCKET)
	{
		cout << "SOCKET creation failed with error:\t" << WSAGetLastError() << endl;
		freeaddrinfo(target);
		WSACleanup();
		return;
	}

	//4) Подключаемся к узлу:
	iResult = connect(connect_socket, target->ai_addr, target->ai_addrlen);
	DWORD dwError = WSAGetLastError();
	freeaddrinfo(target);
	if (iResult == SOCKET_ERROR)
	{
		cout << "Error " << dwError << ":\t";
		//	WSAGetLastError() в обязатенльном порядке должна быть вызвана непосредственно 
		//	после вывоза функции, которая потенциально может выполниться с ошибкой.
		cout << "Unable to connect to server" << endl;
		closesocket(connect_socket);
		//freeaddrinfo(target);
		WSACleanup();
		return;
	}
	//freeaddrinfo(target);

	//5) Отправка:
	CHAR send_buffer[MTU] = "Hello Server";
	iResult = send(connect_socket, send_buffer, strlen(send_buffer), 0);
	if (iResult == SOCKET_ERROR)
	{
		cout << "Send failed with error: " << WSAGetLastError() << endl;
		closesocket(connect_socket);
		WSACleanup();
		return;
	}

	//6) Получение данных:
	CHAR recv_buffer[MTU] = {};
	do
	{
		iResult = recv(connect_socket, recv_buffer, MTU, 0);
		if (iResult > 0)
			cout << "Bytes received: " << iResult << "Message: " << recv_buffer << endl;
		else if (iResult == 0)cout << "Connection closed" << endl;
		else cout << "Receive failed with error " << WSAGetLastError() << endl;

	} while (iResult > 0);

	iResult = shutdown(connect_socket, SD_BOTH);//Закрываем сокет на получение и отправку данных (разрываем TCP-соединение):
	if (iResult == SOCKET_ERROR)
		cout << "Shutdown failed with error " << WSAGetLastError() << endl;
	//7) Освобождаем ресурсы WinSOCK:
	closesocket(connect_socket);
	WSACleanup();
}

