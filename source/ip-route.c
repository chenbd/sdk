#include "ip-route.h"
#if defined(OS_WINDOWS)
#include <winsock2.h>
#include <iphlpapi.h>
#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "iphlpapi.lib")

#else
#include <sys/types.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#endif

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#if defined(OS_WINDOWS)
int ip_route_get(const char* distination, char ip[40])
{
	DWORD index = ~(-1);
	struct sockaddr_in addrin;
	MIB_IPADDRTABLE *table = NULL;
	ULONG dwSize = 0;
	DWORD errcode = 0;
	DWORD i = 0;

	addrin.sin_family = AF_INET;
	addrin.sin_port = htons(0);
	addrin.sin_addr.s_addr = inet_addr(distination);
	if(NO_ERROR != GetBestInterfaceEx((struct sockaddr*)&addrin, &index))
		return -1;

	errcode = GetIpAddrTable( table, &dwSize, 0 );
	assert(ERROR_INSUFFICIENT_BUFFER == errcode);

	table = (MIB_IPADDRTABLE*)malloc(dwSize);
	errcode = GetIpAddrTable( table, &dwSize, 0 );
	if(NO_ERROR != errcode)
		return -1;

	ip[0] = '\0';
	for(i = 0; i < table->dwNumEntries; i++)
	{
		if(table->table[i].dwIndex == index)
		{
			sprintf(ip, "%d.%d.%d.%d", 
				(table->table[i].dwAddr >> 0) & 0xFF,
				(table->table[i].dwAddr >> 8) & 0xFF,
				(table->table[i].dwAddr >> 16) & 0xFF,
				(table->table[i].dwAddr >> 24) & 0xFF);
			break;
		}
	}

	free(table);
	return 0==ip[0] ? -1 : 0;
}
#else
int ip_route_get(const char* distination, char ip[40])
{
	size_t n = 0;
	FILE *fp = NULL;
	char cmd[64] = {0};

	// "ip route get 255.255.255.255 | grep -Po '(?<=src )(\d{1,3}.){4}'"
	snprintf(cmd, sizeof(cmd), "ip route get %s | grep -Po '(?<=src )(\\d{1,3}.){4}'", distination);
	fp = popen(cmd, "r");
	if(!fp)
		return errno;

	fgets(ip, 40, fp);
	pclose(fp);

	n = strlen(ip);
	while(n > 0 && strchr("\r\n \t", ip[n-1]))
		--n;

	ip[n] = '\0';
	return n > 0 ? 0 : -1;
}
#endif

#if defined(OS_WINDOWS)
int ip_local(char ip[40])
{
	MIB_IPADDRTABLE *table = NULL;
	ULONG dwSize = 0;
	DWORD errcode = 0;
	DWORD i = 0;

	errcode = GetIpAddrTable( table, &dwSize, 0 );
	assert(ERROR_INSUFFICIENT_BUFFER == errcode);

	table = (MIB_IPADDRTABLE*)malloc(dwSize);
	errcode = GetIpAddrTable( table, &dwSize, 0 );
	if(NO_ERROR != errcode)
		return -1;

	ip[0] = '\0';
	for(i = 0; i < table->dwNumEntries; i++)
	{
		if(table->table[i].wType & MIB_IPADDR_PRIMARY)
		{
			sprintf(ip, "%d.%d.%d.%d", 
				(table->table[i].dwAddr >> 0) & 0xFF,
				(table->table[i].dwAddr >> 8) & 0xFF,
				(table->table[i].dwAddr >> 16) & 0xFF,
				(table->table[i].dwAddr >> 24) & 0xFF);
			break;
		}
	}

	free(table);
	return 0==ip[0] ? -1 : 0;
}
#else
int ip_local(char ip[40])
{
	struct ifaddrs *ifaddr, *ifa;

	if(0 != getifaddrs(&ifaddr))
		return errno;

	for(ifa = ifaddr; ifa; ifa = ifa->ifa_next)
	{
		if(!ifa->ifa_addr || AF_INET != ifa->ifa_addr->sa_family || 0 == strcmp("lo", ifa->ifa_name))
			continue;

		inet_ntop(AF_INET, &((struct sockaddr_in*)ifa->ifa_addr)->sin_addr, ip, 40);
		break;
	}

	freeifaddrs(ifaddr);
	return 0;
}
#endif
