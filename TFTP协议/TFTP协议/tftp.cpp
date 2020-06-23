//#include "stdafx.h"
#include <WinSock2.h>
#include <iostream>
#include <WS2tcpip.h>
#include <cstdlib>
#pragma comment(lib,"ws2_32.lib")	
using namespace std;

//��ʼ��winsock����ȡudp sokcet
int getUdpSocket()
{
	WORD ver = MAKEWORD(2, 2);
	WSADATA lpData;
	int err = WSAStartup(ver, &lpData);
	if (err != 0)
		return -1;
	int udpsocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (udpsocket == INVALID_SOCKET)
		return -2;
	return udpsocket;
}

//��ȡָ����ַ��Ϣ���ݽṹ
sockaddr_in getAddr(char *ip, int port)
{
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.S_un.S_addr = inet_addr(ip);
	return addr;
}

//��װ�������ر���
char *RequestDownloadPack(const char *content, int &datalen)
{
	int len = strlen(content);
	char *buf = new char[len + 2 + 2 + 5];
	buf[0] = 0x00;
	buf[1] = 0x01;
	memcpy(buf + 2, content, len);
	memcpy(buf + 2 + len, "\0", 1);
	memcpy(buf + 2 + len + 1, "octet", 5);
	memcpy(buf + 2 + len + 1 + 5, "\0", 1);
	datalen = len + 2 + 2 + 5;
	return buf;
}


int main()
{
	SOCKET sock = getUdpSocket();
	sockaddr_in addr = getAddr("192.168.23.1", 69);//TFTP��������ַ
	int datalen;
	char *sendData = RequestDownloadPack("123.flac", datalen);//��������123.flac�ļ�

	int addrlen;
	int res = sendto(sock, sendData, datalen, 0, (sockaddr*)&addr, sizeof(addr));
	if (res != datalen)
	{
		std::cout << "sendto failed" << std::endl;
		goto END;
	}
	delete[]sendData;

	FILE *f = fopen("123.flac", "wb");//���ر���Ϊ123.flac
	if (f == NULL)
	{
		std::cout << "File open failed!" << std::endl;
		goto END;
	}

	while (true)
	{
		char buf[1024];
		sockaddr_in server;
		int len = sizeof(server);
		res = recvfrom(sock, buf, 1024, 0, (sockaddr*)&server, &len);
		//�����������ص�������69�˿ں󣬷���˻������һ���˿����������ݸ��ͻ��ˣ��ͻ��˷���ȷ�ϱ���ʱ����������69�˿ڷ���ACK��������ö˿ڷ���ACK��������Ҫ�����µĵ�ַ��Ϣserver���øõ�ַ��Ϣ���������ݴ���
		if (res > 0)
		{
			short flag;
			memcpy(&flag, buf, 2);
			flag = ntohs(flag);//�����ֽ���ת��Ϊ�����ֽ��򣬺���Ҫ
			if (flag == 3)
			{
				short no;
				memcpy(&no, buf + 2, 2);
				fwrite(buf + 4, res - 4, 1, f);
				if (res > 0 && res < 512)
				{
					std::cout << "download finished!" << std::endl;
					break;
				}
				std::cout << "Pack No=" << ntohs(no) << std::endl;
				//����ȷ�ϱ���
				char ack[4];
				ack[0] = 0x00;
				ack[1] = 0x04;
				memcpy(ack + 2, &no, 2);
				int sendlen = sendto(sock, ack, 4, 0, (sockaddr*)&server, sizeof(server));
				if (sendlen != 4)
				{
					std::cout << "ack error" << WSAGetLastError() << std::endl;
					break;
				}
			}
			if (flag == 5)
			{
				//����˷����˴�����Ϣ
				short errorcode;
				memcpy(&errorcode, buf + 2, 2);
				errorcode = ntohs(errorcode);
				char strError[1024];
				int iter = 0;
				while ((*buf + iter + 4) != 0)
				{
					memcpy(strError + iter, buf + iter + 4, 1);
					++iter;
				}
				std::cout << "Error " << errorcode << "  " << strError << std::endl;
				break;
			}
		}
	}
	fclose(f);
END:
	system("pause");
	return 0;
}
