#pragma once

#include <WinSock2.h>

int gettimeofday(struct timeval* tv, void* tzv = nullptr);

int netError2crtError(int ec);

int pipe_create(int* fds);
int pipe_write(int f, const void* data, size_t len);
int pipe_read(int f, void* data, size_t len);
int pipe_close(int f);
