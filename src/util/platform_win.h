#pragma once

#include <WinSock2.h>

int gettimeofday(struct timeval* tv, void* tzv = nullptr);

int netError2crtError(int ec);

int notifySock_create(int* fds);
int notifySock_write(int f, const void* data, size_t len);
int notifySock_read(int f, void* data, size_t len);
int notifySock_close(int f);
