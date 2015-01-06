#include<iostream>
#include<WinSock2.h>
#include <ws2tcpip.h>
#include<string>
#include<cstdio>
#include <winsock2.h>
#include <iphlpapi.h>
#pragma comment(lib,"iphlpapi.lib")
#pragma comment(lib,"wsock32.lib")
using namespace std;
#pragma comment(lib, "ws2_32.lib")
const BYTE ICMP_ECHO_REQUEST = 8;
//const BYTE ICMP_ECHO_REPLY = 0;
const BYTE ICMP_TIMEOUT = 11;
const int DEF_ICMP_DATA_SIZE = 32;			//icmp���ĵ����ݳ���
const int MAX_ICMP_PACKET_SIZE = 1024;
string shangxian;
string xiaxian;
typedef struct
{
	unsigned char hdr_len:4;        //4λͷ������
	unsigned char version:4;        //4λ�汾��
	unsigned char tos;            //8λ��������
	unsigned short total_len;        //16λ�ܳ���
	unsigned short identifier;        //16λ��ʶ��
	unsigned short frag_and_flags;    //3λ��־��13λƬƫ��
	unsigned char ttl;            //8λ����ʱ��
	unsigned char protocol;        //8λ�ϲ�Э���
	unsigned short checksum;        //16λУ���
	unsigned long sourceIP;        //32λԴIP��ַ
	unsigned long destIP;        //32λĿ��IP��ַ
} IP_HEADER;

typedef struct{

	BYTE type;  //���������ֶ�
	BYTE code;	//���Ĵ����ֶ�
	USHORT cksum; //У���
	USHORT id;	//�ͱ����йص��߳�ID
	USHORT seq; //16λ�������к�

}ICMP_HEADER;

unsigned short checkSum (char *pBuffer, int nLen)
{
	unsigned short nWord;
	unsigned int nSum = 0;
	int i;

	//Make 16 bit words out of every two adjacent 8 bit words in the packet
	//and add them up
	for (i = 0; i < nLen; i = i + 2)
	{
		nWord =((pBuffer [i] << 8)& 0xFF00) + (pBuffer [i + 1] & 0xFF);
		nSum = nSum + (unsigned int)nWord;    
	}

	//Take only 16 bits out of the 32 bit sum and add up the carries
	while (nSum >> 16)
	{
		nSum = (nSum & 0xFFFF) + (nSum >> 16);
	}

	//One's complement the result
	nSum = ~nSum;

	return ((unsigned short) nSum);
}


int main()
{
	DWORD flag_mac = 0;
	IPAddr DEST_IP = 0;
	IPAddr SRC_IP = INADDR_ANY;
	ULONG Len_Mac = 0;
	int final_ip;
	string DestIp;
	string haha;
	char temp[100] = {0};
	//�������ͺͽ��ջ�����
	char SendBuff[1024] = {0};
	char RecvBuff[1024] = {0};
	ULONG MacAddress[2] = {0};
	unsigned long totaltime = 0;
	unsigned long minroundtime = -1;
	unsigned long maxroundtime = 0;
	unsigned long pingjuntime = 0;
	unsigned long recvtime = 0;
	unsigned long roundtime = 0;
	int SendCount = 0;
	int RecvCount = 0;
	int losscount = 0;
	int sendicmp_seq = 0;
	//��ʼ���׽���
	WSADATA wsadata;
	int initFlag = WSAStartup(MAKEWORD(2,2),&wsadata);

	if(initFlag != 0)
	{
		cout<<"�׽��ֳ�ʼ��ʧ��"<<endl;
		WSACleanup();
		return 0;
	}
	int  m_TimeOut = 1000;
	//�����׽���
	SOCKET sock_raw = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if(GetLastError() == INVALID_SOCKET)
	{
		cout<<"�׽��ִ���ʧ��"<<endl;
		WSACleanup();
		return 0;
	}
	//�趨�׽��ֵĳ�ʱʱ�䣬��ֹ���պͷ�����Ϣ��ʱ��������ʹ�ó����ܽ���

	setsockopt(sock_raw,SOL_SOCKET,SO_SNDTIMEO,(char*)&m_TimeOut,sizeof(int));
	setsockopt(sock_raw,SOL_SOCKET,SO_RCVTIMEO,(char*)&m_TimeOut,sizeof(int));
	string start_ip = "";
	cout <<"��������Ҫ̽�������"<<endl;
	cin >> DestIp;
	cout <<"������ʼ������:";
	cin>>start_ip;
	string temp_str = DestIp;
	cout << "�����յ�������:";
	//DestIp = "10.1.11.";
	cin>>haha;
	for(int each = atoi(start_ip.c_str()); each <= atoi(haha.c_str()); each++)
	{
		BYTE* ptr_mac;
		flag_mac = 0;
		DEST_IP = 0;
		SRC_IP = INADDR_ANY;
		Len_Mac = 0;
		memset(MacAddress,0,sizeof(MacAddress));
		DestIp = temp_str;
		memset(temp,0,sizeof(temp));
		SendCount = 0;
		DestIp+=itoa(each,temp,10);
		sockaddr_in destsockinfo;
		sockaddr_in fromsockinfo;
		destsockinfo.sin_family = AF_INET;
		memset(&destsockinfo.sin_zero, 0, 8);
		//ȷ��Ŀ��IP��ַ
		u_long long_ip = inet_addr(DestIp.c_str());
		DEST_IP = inet_addr(DestIp.c_str());
		printf("Sending ARP request for IP address: %s\n", DestIp.c_str());
		Len_Mac = sizeof(MacAddress);
		memset(&MacAddress, 0xff, sizeof (MacAddress));
		flag_mac = SendARP(DEST_IP,SRC_IP,&MacAddress,&Len_Mac);

		if(flag_mac == NO_ERROR)
		{
			ptr_mac = (BYTE*)&MacAddress;
			cout << "MAC��ַ�����ݳ���"<<Len_Mac<<endl;
			if(Len_Mac){

				for(int i = 0; i < Len_Mac; i++)
				{
					if(i == Len_Mac-1)
						printf("%.2x\n",ptr_mac[i]);
					else
						printf("%.2x-",ptr_mac[i]);
				}

			}
			else
				printf("Warning: SendArp completed successfully, but returned length=0\n");

		}
		else 
		{
			printf("Error: SendArp failed with error: %d", flag_mac);
			switch (flag_mac)
			{
			case ERROR_GEN_FAILURE:
				printf(" (ERROR_GEN_FAILURE)\n");
				break;
			case ERROR_INVALID_PARAMETER:
				printf(" (ERROR_INVALID_PARAMETER)\n");
				break;
			case ERROR_INVALID_USER_BUFFER:
				printf(" (ERROR_INVALID_USER_BUFFER)\n");
				break;
			case ERROR_BAD_NET_NAME:
				printf(" (ERROR_GEN_FAILURE)\n");
				break;
			case ERROR_BUFFER_OVERFLOW:
				printf(" (ERROR_BUFFER_OVERFLOW)\n");
				break;
			case ERROR_NOT_FOUND:
				printf(" (ERROR_NOT_FOUND)\n");
				break;
			default:
				printf("\n");
				break;
			}
		}

		char zhujiming[NI_MAXHOST] = {0};
		char sendinfo[NI_MAXSERV] = {0};
		u_short port = 27015;
		sockaddr_in hostname;
		hostname.sin_family = AF_INET;
		hostname.sin_addr.S_un.S_addr = long_ip;
		hostname.sin_port = htons(port);

		DWORD hwd_name = getnameinfo((struct sockaddr *) &hostname,
                           sizeof (struct sockaddr),
						   zhujiming,
                           NI_MAXHOST, sendinfo, NI_MAXSERV, NI_NUMERICSERV);

		if (hwd_name != 0) {
			printf("getnameinfo failed with error # %ld\n", WSAGetLastError());
		} else {
			printf("getnameinfo returned hostname = %s\n", zhujiming);
		}



		//if(long_ip == INADDR_NONE)			//�������IP��ַ����ʧ��
		//{
		//	//��������������
		//	hostent *ptrhostname = gethostbyname(DestIp.c_str()); 
		//	if(ptrhostname == NULL)
		//	{
		//		cout << "����������ʧ��,����ĵ�ַ����Ч" <<endl;
		//		WSACleanup();
		//		return 0;
		//	}
		//	else{
		//		long_ip  = (*((in_addr*)ptrhostname->h_addr_list[0])).S_un.S_addr;
		//		//Ŀ��������IP��ַ��
		//		cout << "Ŀ��������IP��ַ�� ��";
		//		cout <<inet_ntoa(*(in_addr*)ptrhostname->h_addr_list) <<endl;

		//	}
		//}
		destsockinfo.sin_addr.S_un.S_addr = long_ip;
		//ѭ�������ĸ�ICMP����������ݱ�

		while(SendCount<4)
		{
			SendCount++;
			//�����ݱ���дҪ���͵���Ϣ
			ICMP_HEADER * ptr_send_icmp = (ICMP_HEADER*)SendBuff;
			ptr_send_icmp->cksum = 0;
			ptr_send_icmp->type = 8;
			ptr_send_icmp->code = 0;
			ptr_send_icmp->id = (USHORT)GetCurrentProcessId();
			ptr_send_icmp->seq = (USHORT)(sendicmp_seq++);

			memset(SendBuff+sizeof(ICMP_HEADER),'*',DEF_ICMP_DATA_SIZE);
			ptr_send_icmp->cksum = htons(checkSum(SendBuff,sizeof(ICMP_HEADER)+DEF_ICMP_DATA_SIZE));


			int sendflag = sendto(sock_raw,SendBuff,sizeof(ICMP_HEADER)+32,0,(sockaddr*)&destsockinfo,sizeof(destsockinfo));

			if(sendflag == SOCKET_ERROR)
			{
				cout<<"Send error" << WSAGetLastError()<<endl;
				break;
			}
			unsigned long sendtime = GetTickCount();
			int fromlen = sizeof(fromsockinfo);
			int recvflag = recvfrom(sock_raw,RecvBuff,MAX_ICMP_PACKET_SIZE,0,(sockaddr*)&fromsockinfo,&fromlen);

			if(recvflag == SOCKET_ERROR)
			{
				if(WSAGetLastError() == WSAETIMEDOUT)
				{
					cout<<"����ʱ"<<endl;
					losscount++;
					continue;
				}
				break;
			}
			else
			{
				IP_HEADER *ptr_ip = (IP_HEADER*)RecvBuff;
				ICMP_HEADER *ptr_icmp_recv = (ICMP_HEADER*)(RecvBuff+sizeof(IP_HEADER));
				//����յ����Ƿ���������Ҫ�����ݱ�
				if(ptr_icmp_recv->code == 0 && ptr_icmp_recv->id == ptr_send_icmp->id && ptr_icmp_recv->seq == ptr_send_icmp->seq)
				{
					RecvCount++;
					recvtime = GetTickCount();
					roundtime = recvtime-sendtime;
					totaltime+=roundtime;
					if(minroundtime == -1)
					{
						minroundtime = roundtime;
						maxroundtime = roundtime;
					}
					if(roundtime < minroundtime)
					{
						minroundtime = roundtime;
					}
					if(roundtime > maxroundtime)
					{
						maxroundtime = roundtime;
					}

					cout <<"Reply from " << inet_ntoa(fromsockinfo.sin_addr) << ": bytes = " << recvflag-(sizeof(IP_HEADER)+sizeof(ICMP_HEADER)) << ", time = " \
						<< roundtime << "ms, TTL = "<<(int)ptr_ip->ttl<<endl;
				}
				else
				{
					cout << "The echo reply is not correct!" << endl;
				}
				Sleep(1000);
			}
		}
		cout<<endl;
		printf("%s �� Ping ͳ����Ϣ:\n",DestIp.c_str());
		printf("\t���ݰ�: �ѷ��� = %d���ѽ��� = %d����ʧ = %d (%d% ��ʧ)��\n",SendCount,RecvCount,losscount,losscount/SendCount);
		cout<<"�����г̵Ĺ���ʱ��(�Ժ���Ϊ��λ):"<<endl;
		printf("\t��� = %dms��� = %dms��ƽ�� = %dms\n",minroundtime,maxroundtime,totaltime/4);
		cout<<endl;
		cout<<endl;
	}

	return 0;
}

