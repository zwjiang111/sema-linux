// Software License Agreement (BSD License)
//
// Copyright (c) 2022, ADLINK Technology, Inc
// All rights reserved.
//
// Redistribution and use of this software in source and binary forms,
// with or without modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
// * Neither the name of ADLINK Technology nor the names of its contributors may be used
//   to endorse or promote products derived from this software without specific
//   prior written permission of ADLINK Technology, Inc.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>
#include <common.h>
#include <eapi.h>

pthread_mutex_t lib_mutex = PTHREAD_MUTEX_INITIALIZER;

int read_sysfs_file(char *sysfile, char *value, unsigned short size)
{	
	pthread_mutex_lock(&lib_mutex);
	int fd, ret;
	
	fd = open(sysfile, O_RDONLY);
	if(fd < 0){
		pthread_mutex_unlock(&lib_mutex);
		return -1;
	}

	if (flock(fd, LOCK_SH) < 0) {  // Locking the sysfs file
		close(fd);
		pthread_mutex_unlock(&lib_mutex);
		return -1;
	}

	ret = read(fd, value, size);

	// Release lock
	flock(fd, LOCK_UN);
	
	if (ret)
		close(fd);
	else {
		pthread_mutex_unlock(&lib_mutex);
		close(fd);
		return -1;
	}
	pthread_mutex_unlock(&lib_mutex);
	return 0;
}

int write_sysfs_file(char *sysfile, char *value, unsigned short size)
{	
	pthread_mutex_lock(&lib_mutex);
	int fd, ret;
	fd = open(sysfile, O_RDWR);
	if(fd < 0){
		pthread_mutex_unlock(&lib_mutex);
		return -1;
	}

	if (flock(fd, LOCK_SH) < 0) {  // Locking the sysfs file
		close(fd);
		pthread_mutex_unlock(&lib_mutex);
		return -1;
	}
	ret = write(fd, value, size);	

	// Release lock
	flock(fd, LOCK_UN);
	if (ret)
		close(fd);
	else {
		pthread_mutex_unlock(&lib_mutex);
		close(fd);
		return -1;
	}
	pthread_mutex_unlock(&lib_mutex);
	return 0;
}
