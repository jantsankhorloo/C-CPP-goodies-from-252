#include <vector>

class RWLock {
	pthread_mutex_t read;
	pthread_mutex_t write;
	pthread_mutex_t mutex3;
	pthread_cond_t cond;
	pthread_mutexattr_t attr;
	int _nReader;
	int _nWriter;
	int _nWriterWaiting;

	public:
		static const int ERROR_ALREADY_HOLDING_READ_LOCK = 1;
		static const int ERROR_ALREADY_HOLDING_WRITE_LOCK = 2;
		static const int ERROR_NOT_HOLDING_READ_LOCK = 3;
		static const int ERROR_NOT_HOLDING_WRITE_LOCK = 4;
		static const int LOCK_BUSY = 5;
	
		RWLock();

		int read_lock();
		int try_read_lock();
		int read_unlock();

		int write_lock();
		int try_write_lock();
		int write_unlock();

		int write_to_read();
};

