#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h> 
#include <sys/types.h> 
#include <sys/syscall.h> 
#include <assert.h>
#include <pthread.h>
#include <vector>
#include "RWLock.h"
#include <algorithm>

std::vector<unsigned long> vec;
std::vector<unsigned long> wri;
bool busy = false;//writer blocked?
RWLock::RWLock ()
{
	pthread_mutex_init(&read, &attr);
	pthread_mutex_init(&write, &attr);
	pthread_mutex_init(&mutex3, &attr);
	pthread_cond_init(&cond, NULL);
	_nReader = 0;
	_nWriter = 0;
	_nWriterWaiting = 0;
}

int RWLock::read_lock() 
{	
	unsigned long temp = pthread_self();
	if (std::find(vec.begin(), vec.end(), temp) != vec.end()) {
		                //found it
		return ERROR_ALREADY_HOLDING_READ_LOCK;
	}
	else if (std::find(wri.begin(), wri.end(), temp) != wri.end()) {
		return ERROR_ALREADY_HOLDING_WRITE_LOCK;
	}
	
	
	pthread_mutex_lock(&write);
	//pthread_mutex_lock(&read); 

	 	
	while (_nWriter > 0) {
		pthread_cond_wait(&cond, &read);
	}
	_nReader++;
	vec.push_back(temp);
	pthread_mutex_unlock(&read);
	pthread_mutex_unlock(&write);
	return 0;
}

int RWLock::try_read_lock() 
{
	unsigned long temp = pthread_self();
	if (std::find(vec.begin(), vec.end(), temp) != vec.end()) {
		                //found it
		return ERROR_ALREADY_HOLDING_READ_LOCK;
	}
	else if (std::find(wri.begin(), wri.end(), temp) != wri.end()) {
		return ERROR_ALREADY_HOLDING_WRITE_LOCK;
	}

	pthread_mutex_unlock(&write);	
	pthread_mutex_lock(&write);

	if (_nWriter > 0 || _nWriterWaiting > 0) {
		pthread_mutex_unlock(&write);
		return LOCK_BUSY;
	}
	vec.push_back(temp);
	_nReader++; 	

	pthread_mutex_unlock(&write);
	return 0;
}

int RWLock::read_unlock () 
{
	unsigned long temp = pthread_self();
	if (std::find(vec.begin(), vec.end(), temp) == vec.end()) {//not found
		return ERROR_NOT_HOLDING_READ_LOCK;
	}
	else if (std::find(wri.begin(), wri.end(), temp) != wri.end()) {//found
		return ERROR_NOT_HOLDING_READ_LOCK;
	}
	//pthread_mutex_unlock(&write);
	pthread_mutex_lock(&read);
	if (_nReader == 0) {
		pthread_cond_signal(&cond);
	}
	else {	
		_nReader--;	
		pthread_cond_signal(&cond);
	}
	vec.erase(std::remove(vec.begin(), vec.end(), temp), vec.end());
	pthread_mutex_unlock(&read);
	return 0;
}

int RWLock::write_lock () 
{
	unsigned long temp = pthread_self();
	if (std::find(vec.begin(), vec.end(), temp) != vec.end()) {
		return ERROR_ALREADY_HOLDING_READ_LOCK;
	}	
	else if (std::find(wri.begin(), wri.end(), temp) != wri.end()) {
		                //found it
		return ERROR_ALREADY_HOLDING_WRITE_LOCK;
	}
	
	pthread_mutex_lock(&write);
	while (_nReader > 0 || _nWriter > 0) {
		_nWriterWaiting++;
		pthread_cond_wait(&cond, &read);
		_nWriterWaiting--;
	}	
	wri.push_back(temp);
	_nWriter++;
	pthread_mutex_unlock(&read);

	return 0;
	
}

int RWLock::try_write_lock () 
{
	unsigned long temp = pthread_self();
	if (std::find(vec.begin(), vec.end(), temp) != vec.end()) {
		return ERROR_ALREADY_HOLDING_READ_LOCK;
	}	
	else if (std::find(wri.begin(), wri.end(), temp) != wri.end()) {
		                //found it
		return ERROR_ALREADY_HOLDING_WRITE_LOCK;
	}
	//pthread_mutex_unlock(&write);
	pthread_mutex_lock(&write);
	//pthread_mutex_lock(&read);
	if (_nReader > 0 || _nWriter > 0) {

		pthread_mutex_unlock(&write);
		return LOCK_BUSY;
		
	}
	wri.push_back(temp);
	pthread_mutex_unlock(&write);	
	pthread_mutex_unlock(&read);
	_nWriter++; 

	return 0;
}

int RWLock::write_unlock () 
{
	unsigned long temp = pthread_self();
	if (std::find(vec.begin(), vec.end(), temp) != vec.end()) {
		return ERROR_NOT_HOLDING_WRITE_LOCK;
	}
	else if (std::find(wri.begin(), wri.end(), temp) == wri.end()) {//not found
		return ERROR_NOT_HOLDING_WRITE_LOCK;
	}
	wri.erase(std::remove(wri.begin(), wri.end(), temp), wri.end());
	_nWriter--;
	pthread_cond_signal(&cond);
	pthread_mutex_unlock(&write);
	return 0;
}

int RWLock::write_to_read () 
{
	if (_nWriter == 0) {
		return ERROR_NOT_HOLDING_WRITE_LOCK;
	}
	pthread_mutex_lock(&read);
	_nWriter--; _nReader++;
	pthread_cond_signal(&cond);
	wri.erase(std::remove(wri.begin(), wri.end(), pthread_self()), wri.end());
	vec.push_back(pthread_self());
	pthread_mutex_unlock(&read);
	pthread_mutex_unlock(&write);
	return 0;
}

