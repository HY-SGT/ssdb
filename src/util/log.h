/*
Copyright (c) 2012-2014 The SSDB Authors. All rights reserved.
Use of this source code is governed by a BSD-style license that can be
found in the LICENSE file.
*/
#ifndef UTIL_LOG_H
#define UTIL_LOG_H

#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <assert.h>
#include <signal.h>
#include <time.h>
#if SSDB_PLATFORM_WINDOWS
#include "platform_win.h"
#ifndef PATH_MAX
#define PATH_MAX 260
#endif
#else
#include <inttypes.h>
#include <unistd.h>
#include <sys/time.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>

class Logger{
	public:
		static const int LEVEL_NONE		= (-1);
		static const int LEVEL_MIN		= 0;
		static const int LEVEL_FATAL	= 0;
		static const int LEVEL_ERROR	= 1;
		static const int LEVEL_WARN		= 2;
		static const int LEVEL_INFO		= 3;
		static const int LEVEL_DEBUG	= 4;
		static const int LEVEL_TRACE	= 5;
		static const int LEVEL_MAX		= 5;

		static int get_level(const char *levelname);
	private:
		FILE *fp;
		char filename[PATH_MAX];
		int level_;
		pthread_mutex_t *mutex;

		uint64_t rotate_size;
		struct{
			uint64_t w_curr;
			uint64_t w_total;
		}stats;

		void rotate();
		void threadsafe();
	public:
		Logger();
		~Logger();

		int level(){
			return level_;
		}

		void set_level(int level){
			this->level_ = level;
		}

		int open(FILE *fp, int level=LEVEL_DEBUG, bool is_threadsafe=false);
		int open(const char *filename, int level=LEVEL_DEBUG,
			bool is_threadsafe=false, uint64_t rotate_size=0);
		void close();

		int logv(int level, const char *fmt, va_list ap);

		int trace(const char *fmt, ...);
		int debug(const char *fmt, ...);
		int info(const char *fmt, ...);
		int warn(const char *fmt, ...);
		int error(const char *fmt, ...);
		int fatal(const char *fmt, ...);
};


int log_open(FILE *fp, int level=Logger::LEVEL_DEBUG, bool is_threadsafe=false);
int log_open(const char *filename, int level=Logger::LEVEL_DEBUG,
	bool is_threadsafe=false, uint64_t rotate_size=0);
int log_level();
void set_log_level(int level);
int log_write(int level, const char *fmt, ...);
int log_writeV(int level, const char *fmt, va_list ap);

#ifndef _SUPPORT_MACRO_ARGS
#	if _MSC_VER>0 && _MSC_VER<1700
#		define _SUPPORT_MACRO_ARGS 0
#	else
#		define _SUPPORT_MACRO_ARGS	1
#	endif
#endif


#if _SUPPORT_MACRO_ARGS

#ifdef NDEBUG
	#define log_trace(fmt, args...) do{}while(0)
#else
	#define log_trace(fmt, args...)	\
		log_write(Logger::LEVEL_TRACE, "%s(%d): " fmt, __FILE__, __LINE__, ##args)
#endif

#define log_debug(fmt, args...)	\
	log_write(Logger::LEVEL_DEBUG, "%s(%d): " fmt, __FILE__, __LINE__, ##args)
#define log_info(fmt, args...)	\
	log_write(Logger::LEVEL_INFO,  "%s(%d): " fmt, __FILE__, __LINE__, ##args)
#define log_warn(fmt, args...)	\
	log_write(Logger::LEVEL_WARN,  "%s(%d): " fmt, __FILE__, __LINE__, ##args)
#define log_error(fmt, args...)	\
	log_write(Logger::LEVEL_ERROR, "%s(%d): " fmt, __FILE__, __LINE__, ##args)
#define log_fatal(fmt, args...)	\
	log_write(Logger::LEVEL_FATAL, "%s(%d): " fmt, __FILE__, __LINE__, ##args)

#else
	template<int Level>
	static int log_writeT(const char* fmt, ...){
		va_list ap;
		va_start(ap, fmt);
		int ret = log_writeV(Level, fmt, ap);
		va_end(ap);
		return 0;
	}

#ifdef NDEBUG
#define log_trace	1?0:
#else
#define log_trace log_writeT<Logger::LEVEL_TRACE>
#endif

#define log_debug log_writeT<Logger::LEVEL_DEBUG>
#define log_info log_writeT<Logger::LEVEL_INFO>
#define log_warn log_writeT<Logger::LEVEL_WARN>
#define log_error log_writeT<Logger::LEVEL_ERROR>
#define log_fatal log_writeT<Logger::LEVEL_FATAL>

#endif

#endif
