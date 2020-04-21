#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <winsock2.h>
#include "ndds/ndds_cpp.h"
#include "clock/clock_highResolution.h"

#pragma comment(lib, "ws2_32.lib") 

int main(int argc, char* argv[])
{
	double *_roundtrip_time_sorted;
	double _min_roundtrip_time=100000000.0;
	double _max_roundtrip_time=0.0;
	struct RTINtpTime _start_time  = RTI_NTP_TIME_ZERO;
	struct RTINtpTime _finish_time = RTI_NTP_TIME_ZERO;
	struct RTIClock *_clock;
	int _message_count=0;

	int count = 0;
	int buffer_size = 0;
	if(argc >= 2)
	{
		count = atoi(argv[1]);
	}

	if(argc >= 3)
	{
		buffer_size = atoi(argv[2]);
	}

    WORD socketVersion = MAKEWORD(2,2);
    WSADATA wsaData; 
    if(WSAStartup(socketVersion, &wsaData) != 0)
    {
        return 0;
    }

    SOCKET sclient = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    
    sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_port = htons(8080);
    sin.sin_addr.S_un.S_addr = inet_addr("192.168.1.110");
    int len = sizeof(sin);

	_roundtrip_time_sorted = new double[count];

	char * sendData = (char*)malloc(buffer_size);
	char * recvData = (char*)malloc(buffer_size+100);
	if(!sendData || !recvData)
	{
		printf("malloc error\n");
		return -1;
	}
	_clock = RTIHighResolutionClock_new();
	RTINtpTime roundtrip = {0, 0};
	double roundtrip_in_double = 0.0;
	int recv_size = 0;
	for(int sample_count = 0;(sample_count < count);sample_count++)
	{
		//printf("write %d .\n",sample_count);
		_message_count = sample_count;
		_clock->getTime(_clock, &_start_time);
		sendto(sclient, sendData, buffer_size, 0, (sockaddr *)&sin, len);
		while(buffer_size - recv_size > 0)
		{
			recv_size += recvfrom(sclient, recvData, buffer_size - recv_size, 0, (sockaddr *)&sin, &len);
			//printf("%d \n",recv_size);
		}
		recv_size = 0;
		_clock->getTime(_clock, &_finish_time);

		RTINtpTime_subtract(roundtrip, _finish_time, _start_time);
		roundtrip_in_double =1E6 * RTINtpTime_toDouble(&roundtrip);
		if (roundtrip_in_double <= 0) {
			printf("roundtrip time <= 0\n");
		}

		if (  roundtrip_in_double < _min_roundtrip_time ) 
		{
			_min_roundtrip_time = roundtrip_in_double;
		}
		if (  roundtrip_in_double > _max_roundtrip_time ) {
			_max_roundtrip_time = roundtrip_in_double;
		}
		_roundtrip_time_sorted[sample_count] = roundtrip_in_double;
	}
   
	double sum,env;
	sum=0;
	for(count=0;count<=_message_count;count++)
	{
		sum = sum + _roundtrip_time_sorted[count];
	}
	_message_count++;
	env=sum/_message_count;
	printf("test successful!\n");
	printf("env=%7.1f\n",env);
	printf("max=%7.1f\n",_max_roundtrip_time);
	printf("min=%7.1f\n",_min_roundtrip_time);

    closesocket(sclient);
    WSACleanup();
    return 0;
}