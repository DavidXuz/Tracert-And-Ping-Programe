#include<iostream>
#include<WinSock2.h>
#include<WS2tcpip.h>
const BYTE ICMP_ECHO_REQUEST = 8;
const BYTE ICMP_ECHO_REPLY = 0;
const BYTE ICMP_TIMEOUT = 11;

const int DEF_ICMP_DATA_SIZE = 32;			//icmp���ĵ����ݳ���
const int MAX_ICMP_PACKET_SIZE = 1024;
const DWORD DEF_ICMP_TIMEOUT =3000;
const int DEF_MAX_HOP = 30;					//�������
char HostName[255] = {0};		//���������������������IP��ַ
using namespace std;
//����ICMP����ͷ���ṹ��
//#pragma comment( lib, "ws2_32.lib" )    
typedef struct{

	BYTE type;  //���������ֶ�
	BYTE code;	//���Ĵ����ֶ�
	USHORT cksum; //У���
	USHORT id;	//�ͱ����йص��߳�ID
	USHORT seq; //16λ�������к�

}ICMP_HEADER;

//IP��ͷ
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

	USHORT usSeqNo;
	DWORD dwRoundTripTime;
	in_addr ipadress;
}DECODE_RESULT;


//��������У��ͺ���
USHORT checksum(USHORT *pBuf,int iSize)
{
    unsigned long cksum=0;
    while(iSize>1)
    {
        cksum+=*pBuf++;
        iSize-=sizeof(USHORT);
    }
    if(iSize)
    {
        cksum+=*(UCHAR *)pBuf;
    }
    cksum=(cksum>>16)+(cksum&0xffff);
    cksum+=(cksum>>16);
    return (USHORT)(~cksum);
}

BOOL DecodeIcmpResponse(char * pBuf,int iPacketSize,DECODE_RESULT &DecodeResult,BYTE ICMP_ECHO_REPLY,BYTE  ICMP_TIMEOUT)
{
	IP_HEADER *piphdr = (IP_HEADER*)pBuf;
	int iphdrlen = piphdr->hdr_len*4;		//IP���ݱ����ײ�����
	if(iPacketSize < (int)(iphdrlen+sizeof(ICMP_HEADER)))
		return FALSE;

	ICMP_HEADER *ptr_icmp = (ICMP_HEADER*)(pBuf+iphdrlen);
	USHORT usID, usSquNo;
	if(ptr_icmp->type == ICMP_ECHO_REPLY)
	{
		usID = ptr_icmp->id;
		usSquNo = ptr_icmp->seq;
	}
	else if(ptr_icmp->type == ICMP_TIMEOUT)
	{
		char *innerip = pBuf+iphdrlen+sizeof(ICMP_HEADER);
		int inneriplen = ((IP_HEADER*)innerip)->hdr_len*4;
		ICMP_HEADER *innericmp = (ICMP_HEADER*)(innerip+inneriplen);
		usID = innericmp->id;
		usSquNo = innericmp->seq;
	}
	else
		return false;


	if(usID!=(USHORT)GetCurrentProcessId()||usSquNo!=DecodeResult.usSeqNo)
		return false;

	DecodeResult.ipadress.S_un.S_addr = piphdr->sourceIP;
	DecodeResult.dwRoundTripTime = GetTickCount()-DecodeResult.dwRoundTripTime;//��������ʱ��

	if(ptr_icmp->type == ICMP_ECHO_REPLY||ICMP_TIMEOUT)
	{
		if(DecodeResult.dwRoundTripTime){
			cout <<"	"<<DecodeResult.dwRoundTripTime<<"ms"<<flush;
		}
		else
			cout <<"	"<<"1ms"<<flush;
	}
	return true;
}

int main()
{

	cout << "������Ŀ��Ŀ��������IP��ַ��������>>>" << endl;
	cin >> HostName;


	//��ʼ���׽��ֿ�

	WSADATA m_wsadata;
	int initFlag = WSAStartup(MAKEWORD(2,2),&m_wsadata);

	if(initFlag != 0)																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																						
	{
		cout <<"��ʼ������"<<endl;
		return 0;
	}

	//���ַ�����ʽ��IP��ַת��Ϊ�����з��������u_long����
	u_long long_ip = inet_addr(HostName);

	if(long_ip == INADDR_NONE)			//�������IP��ַ����ʧ��
	{
		//��������������
		hostent *ptrhostname = gethostbyname(HostName); 
		if(ptrhostname == NULL)
		{
			cout << "����������ʧ��,����ĵ�ַ����Ч" <<endl;
			WSACleanup();
			return 0;
		}
		else{
			long_ip  = (*((in_addr*)ptrhostname->h_addr_list[0])).S_un.S_addr;
			//Ŀ��������IP��ַ��
			cout << "Ŀ��������IP��ַ�� ��";
			cout <<inet_ntoa(*(in_addr*)ptrhostname->h_addr_list) <<endl;

		}
	}
	
	//�洢Ŀ�������ĵ�ַ��Ϣ�����н�����Ϣ�ĵ�ַ��Ϣ
	sockaddr_in sendinfo;
	ZeroMemory(&sendinfo,sizeof(sockaddr_in));
	cout << "Tracing roote to    "<<HostName<<"    with a maximun of 30 hops.\n" << endl;
	//���Ŀ��������ַ��Ϣ
	sendinfo.sin_family = AF_INET;
	sendinfo.sin_addr.S_un.S_addr = long_ip;

	SOCKET sockRaw = WSASocket(AF_INET,SOCK_RAW,IPPROTO_ICMP,NULL,0,WSA_FLAG_OVERLAPPED);
	if(INVALID_SOCKET == WSAGetLastError())
	{
		cout <<"�����׽���ʧ��"<<endl;
		WSACleanup();
		return 0;
	}
	//���ó�ʱʱ��
	int TimeOut = 3000;

	//�����������ͺͽ������ݵĳ�ʱʱ��
	setsockopt(sockRaw,SOL_SOCKET,SO_SNDTIMEO,(char*)&TimeOut,sizeof(TimeOut));
	setsockopt(sockRaw,SOL_SOCKET,SO_RCVTIMEO,(char*)&TimeOut,sizeof(TimeOut));
	char IcmpSendBuff[DEF_ICMP_DATA_SIZE+sizeof(ICMP_HEADER)] = {0};		//���巢�ͻ�����
	char IcmpRecvBuff[MAX_ICMP_PACKET_SIZE] = {0};				//������ջ�����

	//��ʼ��ICMP����ͷ��
	//����������һ���ڴ�ռ䣬Ȼ���ٶ��ڴ�ռ���в������ѽṹ���еĳ�Ա��ֵ��ȥ
	ICMP_HEADER* ptr_icmpheader = (ICMP_HEADER*)IcmpSendBuff;

	ptr_icmpheader->type = ICMP_ECHO_REQUEST;		//�趨��������
	ptr_icmpheader->code = 0;
	ptr_icmpheader->id = (USHORT)GetCurrentProcessId();
	memset(IcmpSendBuff+sizeof(ICMP_HEADER),'E',DEF_ICMP_DATA_SIZE);

	USHORT usSeqNo = 0;
	int TTL = 1;
	BOOL QuitFlag = FALSE;
	int maxhop = DEF_MAX_HOP;
	DECODE_RESULT DecodeResult;
	while(!QuitFlag&&maxhop--)				//��ʼ׷��·�ɲ���
	{
		//����ÿ�η��͵�IP���ݱ���TTL
		setsockopt(sockRaw,IPPROTO_IP,IP_TTL,(char*)&TTL,sizeof(TTL));
		cout << TTL << flush;
		ptr_icmpheader->cksum = 0;
		ptr_icmpheader->seq = htons(usSeqNo++);
		ptr_icmpheader->cksum = checksum((USHORT*)IcmpSendBuff,sizeof(ICMP_HEADER)+DEF_ICMP_DATA_SIZE);


		DecodeResult.usSeqNo = ((ICMP_HEADER*)IcmpSendBuff)->seq;
		DecodeResult.dwRoundTripTime = GetTickCount();
		//�������ݱ�
		sendto(sockRaw,IcmpSendBuff,sizeof(IcmpSendBuff),0,(sockaddr*)&sendinfo,sizeof(sendinfo));
		sockaddr_in recvinfo;
		int recvlen = 0;
		int fromlen = sizeof(recvinfo);
		//׼�����ܷ�������ICMP���ݱ�
		while(1)
		{
			//memset(IcmpRecvBuff,0,sizeof(IcmpRecvBuff));
			recvlen = recvfrom(sockRaw,IcmpRecvBuff,MAX_ICMP_PACKET_SIZE,0,(SOCKADDR*)&recvinfo,&fromlen);
			if(recvlen!=SOCKET_ERROR)
			{
				if(DecodeIcmpResponse(IcmpRecvBuff,recvlen,DecodeResult,ICMP_ECHO_REPLY,ICMP_TIMEOUT))
				{
					if(DecodeResult.ipadress.S_un.S_addr == sendinfo.sin_addr.S_un.S_addr)
						QuitFlag = true;
					cout << "\t" << inet_ntoa(DecodeResult.ipadress)<<endl;
					break;
				}
			}
			else if (WSAGetLastError() == WSAETIMEDOUT)
			{
				cout <<"		*"<<"	"<<"Request timed out" <<endl;
				break;
			}
			else
				break;
		}
		//׼��������һ�����ݱ�
		TTL++;
	}
	return 0;
}