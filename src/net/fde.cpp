/*
Copyright (c) 2012-2014 The SSDB Authors. All rights reserved.
Use of this source code is governed by a BSD-style license that can be
found in the LICENSE file.
*/
#include "fde.h"

struct Fdevent* Fdevents::get_fde(int fd){
	auto it = events_fd.find(fd);
	if (it != events_fd.end())
	{
		return it->second;
	}
	struct Fdevent *fde = new Fdevent();
	//fde->fd = events.size();
	fde->s_flags = FDEVENT_NONE;
	fde->data.num = 0;
	fde->data.ptr = NULL;
	events_fd[fd] = fde;
	return fde;
}


#ifdef HAVE_EPOLL
#include "fde_epoll.cpp"
#else
#include "fde_select.cpp"
#endif
