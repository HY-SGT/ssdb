#if _WIN32 || _WIN64
#include "../src/util/platform_win.h"
#include <stdio.h>
#include <io.h>
#include <stdint.h>
#include <windows.h>

int pipe_create(int* f)
{
	SOCKET server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (server <= 0)
	{
		return -1;
	}
	sockaddr_in addr = {0};
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	int ret = bind(server, (sockaddr*)&addr, sizeof(addr));
	ret = listen(server, 2);
	int addrlen = sizeof(addr);
	getsockname(server, (sockaddr*)&addr, &addrlen);
	if (addr.sin_port)
	{
#if _DEBUG || DEBUG
		int port = ntohs(addr.sin_port);
#endif
		SOCKET client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (client > 0)
		{
			ULONG async = 1;
			ioctlsocket(client, FIONBIO, &async);
			ret = connect(client, (sockaddr*)&addr, sizeof(addr));
			if (ret ==0 || GetLastError() == WSAEWOULDBLOCK)
			{
				sockaddr tmp;
				int tmplen = sizeof(tmp);
				SOCKET peer = accept(server, &tmp, &tmplen);
				if (peer > 0)
				{
					async = 0;
					ioctlsocket(client, FIONBIO, &async);
					f[0] = peer;
					f[1] = client;
					return 0;
				}
			}
			closesocket(client);
		}
	}
	closesocket(server);
	return -1;

}

int pipe_write(int f, const void* data, size_t len)
{
	int ret = send(f, (const char*)data, len, 0);
	errno = netError2crtError(GetLastError());
	return ret;
}
int pipe_read(int f, void* data, size_t len)
{
	int ret = recv(f, (char*)data, len, 0);
	errno = netError2crtError(GetLastError());
	return ret;

}
int pipe_close(int f)
{
	if (f<=0)
	{
		return 0;
	}
	return closesocket(f);
}
//////////////////////////////////////////////////////////////////////////
#define EPOCH_BIAS 116444736000000000LL
int gettimeofday(struct timeval* tv, void* tzv )
{
	union
	{
		long long ns100;
		FILETIME ft;
	} now;
	GetSystemTimeAsFileTime (&now.ft);
	tv->tv_usec = (long) ((now.ns100 / 10LL) % 1000000LL);
	tv->tv_sec = (long) ((now.ns100 - EPOCH_BIAS) / 10000000LL);

	return (0);
}
double millitime(){
	struct timeval now;
	gettimeofday(&now, NULL);
	double ret = now.tv_sec + now.tv_usec/1000.0/1000.0;
	return ret;
}

int64_t time_ms(){
	struct timeval now;
	gettimeofday(&now, NULL);
	return now.tv_sec * 1000 + now.tv_usec/1000;
}


#endif