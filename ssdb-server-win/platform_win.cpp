#if SSDB_PLATFORM_WINDOWS
#include "../src/util/platform_win.h"
#include <stdio.h>
#include <io.h>
#include <stdint.h>
#include <windows.h>

int notifySock_create(int* f)
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
			if (ret == 0 || GetLastError() == WSAEWOULDBLOCK)
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

int notifySock_write(int f, const void* data, size_t len)
{
	int ret = send(f, (const char*)data, len, 0);
	errno = netError2crtError(GetLastError());
	return ret;
}
int notifySock_read(int f, void* data, size_t len)
{
	int ret = recv(f, (char*)data, len, 0);
	errno = netError2crtError(GetLastError());
	return ret;

}
int notifySock_close(int f)
{
	if (f <= 0)
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
//////////////////////////////////////////////////////////////////////////
#include <signal.h>
#include <errno.h>

void my_signal_run_one()
{
	char pipename[64];
	sprintf(pipename, "\\\\.\\pipe\\_signal_pipe:%d", GetCurrentProcessId());
	HANDLE hPipe = CreateNamedPipeA(pipename,
		PIPE_ACCESS_DUPLEX | FILE_FLAG_WRITE_THROUGH,
		PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
		PIPE_UNLIMITED_INSTANCES,
		0, 0, 0, NULL);
	int ec = GetLastError();
	if (hPipe == INVALID_HANDLE_VALUE)
	{
		int ec = GetLastError();
		return;
	}
	if( !ConnectNamedPipe(hPipe, NULL) )
	{
		int ec = GetLastError();
		return;
	}
	while (true)
	{
		char line[128];
		DWORD readBytes = 0;
		int ret = ReadFile(hPipe, line, sizeof(line) - 1, &readBytes, NULL);
		int ec = GetLastError();
		if (readBytes <= 0)
		{
			//assert(ec == ERROR_BROKEN_PIPE);
			break;
		}
		if(readBytes > 0)
		{
			line[readBytes] = '\0';
			line[sizeof(line) - 1] = '\0';
			int arg1 = 0;
			DWORD writeBytes = 0;
			if( sscanf_s(line, "kill %d\n", &arg1) == 1 )
			{
				if (arg1 != 0)
				{
					raise(arg1);
					Sleep(1);
				}
				const char* resp = "ok\n";
				WriteFile(hPipe, resp, strlen(resp), &writeBytes, NULL);
				continue;
			}

			char* resp = "error\n";
			WriteFile(hPipe, resp, strlen(resp), &writeBytes, NULL);

		}
	}
	FlushFileBuffers(hPipe);
	DisconnectNamedPipe(hPipe);
	CloseHandle(hPipe);
}
void my_signal_thread()
{
	for (;;)
	{
		my_signal_run_one();
	}

};
bool signal_init()
{
	for (size_t i = 0; i < 2; ++i) // 一个name pipe只能同时支持一个connection
	{
		CloseHandle(CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)my_signal_thread, 0, 0, 0));
	}
	return true;
}
bool g_signal_init = signal_init();

int kill(int pid, int code)
{
	char pipename[64];
	sprintf(pipename, "\\\\.\\pipe\\_signal_pipe:%d", pid);
	HANDLE hPipe = CreateFileA(pipename, GENERIC_ALL, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
	int ec = GetLastError();
	if (hPipe != INVALID_HANDLE_VALUE)
	{
		char sendmsg[64];
		sprintf(sendmsg, "kill %d\n", code);
		DWORD bytes = 0;
		WriteFile(hPipe, sendmsg, strlen(sendmsg), &bytes, NULL);
		ec = GetLastError();

		char outbuf[32] = {0};
		bytes = 0;
		ReadFile(hPipe, outbuf, sizeof(outbuf), &bytes, NULL);
		ec = GetLastError();

		CloseHandle(hPipe);

		outbuf[sizeof(outbuf) - 1] = '\0';
		if (strcmp(outbuf, "ok\n") == 0)
		{
			errno = 0;
			return 0;
		}
	}

	errno = ESRCH;
	return -1;
}
#endif