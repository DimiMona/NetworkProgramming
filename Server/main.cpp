//Server
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <iostream>
#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iphlpapi.h> 
using namespace std;

//эта директива компоновщику (линковщику) автоматически подключить библиотеку WS2_32.lib к проекту.
//эта библиотека для работы с Winsock2(Windows Sockets 2).
// Она содержит все функции для сетевого программирования.
#pragma comment(lib, "WS2_32.lib")
#define MTU	1500
void main()
{
	setlocale(LC_ALL, "");
	cout << "SERVER" << endl;
	//1) Инициализация WinSOCK:
	WSADATA wsaData;
	INT iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		cout << "WSAStartup failed with error: " << iResult << endl;
		return;
	}

	//2) Параметры подключения:
	addrinfo hints;
	addrinfo* target;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;//AF_INET	IPv4 (старый стандарт)	192.168.1.1
	hints.ai_socktype = SOCK_STREAM;//SOCK_STREAM - tcp или SOCK_DGRAM - udp
	hints.ai_protocol = IPPROTO_TCP;// (уточняем протокол)в данном случае можем написать 0, но чтоб не было ошибок указвает конкретный протокол
	hints.ai_flags = AI_PASSIVE;	//Соединение будет работать в режиме 'LISTENING';

	iResult = getaddrinfo(NULL, "27015", &hints, &target);//Эта функция превращает название в настоящий сетевой адрес, который понимает компьютер
	//(IP-адрес сервера(для всех),Номер порта,параметры, сохраняем ip aдрес )
	
	//проверка которая проверяет получилось ли создать ip адрес
	if (iResult != 0)
	{
		cout << "getaddrinfo() failed with error: " << iResult << endl;
		freeaddrinfo(target);
		WSACleanup();
		return;
	}

	//3) Создание серверного сокета, который он будет постоянно прослушивать:
	//почему нельзя взять параметры которые в Hints, потому что идет проверка системы, которая уже определяет
	// что заявленные параметры в Hints подходят, если нет, то система втоматически их меняет. 
	// Пример: если вдруг мы захотели использовать IPv6 но система не поддерживает, тогда она заменит на IPv4
	//вот почему мы используем target.
	SOCKET listen_socket =
		socket(target->ai_family, target->ai_socktype, target->ai_protocol);//берем данные из Hints
	//Эта проверка критически важна, 
	// потому что socket() может НЕ создаться 
	// даже при правильных параметрах!
	//проверяем, создали ли мы сокет по этому адресу
	if (listen_socket == INVALID_SOCKET)
	{
		cout << "SOCKET creation failed with error: " << WSAGetLastError() << endl;
		freeaddrinfo(target);
		WSACleanup();
		return;
	}

	//4) Привязываем сокет к интерфейсу и порту:
	iResult = bind(listen_socket, target->ai_addr, target->ai_addrlen);//(параметры (пример: ipv4, tcp, подтвержденеие tcp), порт и ip)
	//проверка привязался ли сокет 
	if (iResult != 0)
	{
		cout << "bind failed with error: " << WSAGetLastError() << endl;
		freeaddrinfo(target);// очищаем память 
		closesocket(listen_socket);//закрываем сокет
		WSACleanup();//закрываем
		return;
	}

	//5) Запускаем прослушивание порта:
	if (listen(listen_socket, 1) == SOCKET_ERROR)	//1 - Максимальное количество одновременно подключенных клиентов
	{
		cout << "Listen failed with error: " << WSAGetLastError() << endl;
		closesocket(listen_socket);
		freeaddrinfo(target);
		WSACleanup();
		return;
	}

	//6) Принимаем подключение от клиента
	//создаем на нашем сервере новый сокет "client_socket" (у клиента внешний сокет свой как и ip)
	//и подключаем его на нашем сервере с "listen_socket"
	SOCKADDR_IN client_address;
	INT client_address_len = sizeof(client_address);
	SOCKET client_socket = accept(listen_socket, (SOCKADDR*)&client_address, &client_address_len);
	if (client_socket == INVALID_SOCKET)
	{
		cout << "Accept failed with error: " << WSAGetLastError() << endl;
		closesocket(listen_socket);
		freeaddrinfo(target);
		WSACleanup();
		return;
	}
	cout << inet_ntoa(client_address.sin_addr) << ";" << ntohs(client_address.sin_port) << endl;
	//7) Получаем данные от клиента:
	CHAR recv_buffer[MTU] = {};// Буфер для приёма данны
	CHAR send_buffer[MTU] = "Hello client";// Буфер для отправки 
	INT iReceivedBytes = 0;// Сколько байт получили
	INT iSentBytes = 0;// Сколько байт отправили
	do
	{
		iReceivedBytes = recv(client_socket, recv_buffer, MTU, 0);//(данные клиента,
		//переменная для информации которую передает клиент, размер,спец флаг )
		//Функция recv() - Receive ожидает получение данных по указанному сокету, и возвращает количество полученных Байт.
		if (iReceivedBytes > 0)
		{
			//читаем что отправил клиент 
			cout << "Received " << iReceivedBytes << " " << recv_buffer << endl;
			//отправляем ответ 
			iSentBytes = send(client_socket, send_buffer, strlen(send_buffer), 0);
			//проверка отправки 
			if (iSentBytes == SOCKET_ERROR)	cout << "Send failed with error:\t" << WSAGetLastError() << endl;
			else cout << iSentBytes << " Bytes sent" << endl;
		}
		//если было отправленно пустое письмо
		else if (iReceivedBytes == 0) cout << "Connection closing..." << endl;
		//в других случиях ошибка из списка
		else cout << "Receive failed with error: " << WSAGetLastError() << endl;
	} while (iReceivedBytes > 0);

	//8) Разрываем TCP-соединение:
	iResult = shutdown(client_socket, SD_BOTH);
	if (iResult != SOCKET_ERROR)cout << "shutdown failed with error:\t" << WSAGetLastError() << endl;

	//9) Освобождаем ресурсы, занятиые WinSOCK:
	closesocket(listen_socket);
	freeaddrinfo(target);
	WSACleanup();
}