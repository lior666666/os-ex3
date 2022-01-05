#ifndef __REQUEST_H__

void requestHandle(int fd, time_t arrival_time_sec, suseconds_t arrival_time_usec, time_t dispatch_time_sec, 
	suseconds_t dispatch_time_usec, int thread_id, int* thread_counter, int* thread_static_counter, int* thread_dynamic_counter);

#endif
