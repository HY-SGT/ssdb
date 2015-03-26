/*
Copyright (c) 2012-2014 The SSDB Authors. All rights reserved.
Use of this source code is governed by a BSD-style license that can be
found in the LICENSE file.
*/
#ifndef UTIL_FDE_SELECT_H
#define UTIL_FDE_SELECT_H

#if SSDB_PLATFORM_WINDOWS
int netError2crtError(int ec);
#endif

Fdevents::Fdevents(){
	FD_ZERO(&readset);
	FD_ZERO(&writeset);
}

Fdevents::~Fdevents(){
	for(auto it = events_fd.begin(); it != events_fd.end(); ++it){
		delete it->second;
	}
	events_fd.clear();
	ready_events.clear();
}

bool Fdevents::isset(int fd, int flag){
	struct Fdevent *fde = get_fde(fd);
	return (bool)(fde->s_flags & flag);
}

int Fdevents::set(int fd, int flags, int data_num, void *data_ptr){
#if SSDB_PLATFORM_WINDOWS
	if(fd <= 0 || events_fd.size() >= FD_SETSIZE){
#else
	if(fd > FD_SETSIZE - 1){
#endif
		return -1;
	}

	struct Fdevent *fde = get_fde(fd);
	if(fde->s_flags & flags){
		return 0;
	}

	if(flags & FDEVENT_IN)  FD_SET(fd, &readset);
	if(flags & FDEVENT_OUT) FD_SET(fd, &writeset);

	fde->data.num = data_num;
	fde->data.ptr = data_ptr;
	fde->s_flags |= flags;

	return 0;
}

int Fdevents::del(int fd){
	FD_CLR(fd, &readset);
	FD_CLR(fd, &writeset);

	auto it = events_fd.find(fd);
	if (it != events_fd.end())
	{
		events_fd.erase(it);
	}
	return 0;
}

int Fdevents::clr(int fd, int flags){
	if(flags & FDEVENT_IN)  FD_CLR(fd, &readset);
	if(flags & FDEVENT_OUT) FD_CLR(fd, &writeset);

	auto it = events_fd.find(fd);
	if (it != events_fd.end())
	{
		it->second->s_flags &= ~flags;
		if (it->second->s_flags == 0)
		{
			events_fd.erase(it);
		}
	}

	return 0;
}

const Fdevents::events_t* Fdevents::wait(int timeout_ms){
	struct timeval tv;
	struct Fdevent *fde;
	int i, ret;

	ready_events.clear();
	
	fd_set t_readset = readset;
	fd_set t_writeset = writeset;

	if(timeout_ms >= 0){
		tv.tv_sec =  timeout_ms / 1000;
		tv.tv_usec = (timeout_ms % 1000) * 1000;
		ret = ::select(events_fd.size(), &t_readset, &t_writeset, NULL, &tv);
	}else{
		ret = ::select(events_fd.size(), &t_readset, &t_writeset, NULL, NULL);
	}
	if(ret < 0){
#if SSDB_PLATFORM_WINDOWS
		errno = netError2crtError( GetLastError() );
#endif
		if(errno == EINTR){
			return &ready_events;
		}
		return NULL;
	}

	if(ret > 0){
		for (auto it = events_fd.begin(); it != events_fd.end() && ret>0; ++it)
		{
			fde = it->second;
			fde->events = FDEVENT_NONE;
			if(FD_ISSET(it->first, &t_readset))  fde->events |= FDEVENT_IN;
			if(FD_ISSET(it->first, &t_writeset)) fde->events |= FDEVENT_OUT;

			if(fde->events){
				ready_events[it->first] = it->second;
				--ret;
			}
		}
	}

	return &ready_events;
}

#endif
