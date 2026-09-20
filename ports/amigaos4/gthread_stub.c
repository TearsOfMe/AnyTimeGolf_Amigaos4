#include <stddef.h>

int pthread_mutex_lock(void *mutex) { (void) mutex; return 0; }
int pthread_mutex_unlock(void *mutex) { (void) mutex; return 0; }
int pthread_mutex_init(void *mutex, const void *attr) { (void) mutex; (void) attr; return 0; }
int pthread_mutex_destroy(void *mutex) { (void) mutex; return 0; }
int pthread_cond_signal(void *cond) { (void) cond; return 0; }
int pthread_cond_init(void *cond, const void *attr) { (void) cond; (void) attr; return 0; }
int pthread_cond_destroy(void *cond) { (void) cond; return 0; }
int pthread_cond_wait(void *cond, void *mutex) { (void) cond; (void) mutex; return 0; }
int pthread_create(void *thread, const void *attr, void *(*func)(void *), void *arg)
{
	(void) thread; (void) attr; (void) func; (void) arg; return -1;
}
int pthread_join(void *thread, void **value) { (void) thread; (void) value; return 0; }

typedef struct { int data1, data2; } gthread_once_t;
typedef struct { unsigned long id; } gthread_key_t;
typedef int gthread_t;

int __gthread_once(gthread_once_t *once, void (*func)(void))
{
	(void) once;
	if (func != NULL)
		func();
	return 0;
}

void __gthread_once_unlock(gthread_once_t *once) { (void) once; }
int __gthread_active_p(void) { return 0; }

int __gthread_mutex_init(void *mutex) { (void) mutex; return 0; }
int __gthread_mutex_destroy(void *mutex) { (void) mutex; return 0; }
int __gthread_mutex_lock(void *mutex) { (void) mutex; return 0; }
int __gthread_mutex_trylock(void *mutex) { (void) mutex; return 0; }
int __gthread_mutex_unlock(void *mutex) { (void) mutex; return 0; }

int __gthread_recursive_mutex_init(void *mutex) { (void) mutex; return 0; }
int __gthread_recursive_mutex_lock(void *mutex) { (void) mutex; return 0; }
int __gthread_recursive_mutex_trylock(void *mutex) { (void) mutex; return 0; }
int __gthread_recursive_mutex_unlock(void *mutex) { (void) mutex; return 0; }
int __gthread_recursive_mutex_destroy(void *mutex) { (void) mutex; return 0; }

int __gthread_cond_init(void *cond) { (void) cond; return 0; }
int __gthread_cond_signal(void *cond) { (void) cond; return 0; }
int __gthread_cond_broadcast(void *cond) { (void) cond; return 0; }
int __gthread_cond_wait(void *cond, void *mutex)
{
	(void) cond;
	(void) mutex;
	return 0;
}
int __gthread_cond_wait_recursive(void *cond, void *mutex)
{
	(void) cond;
	(void) mutex;
	return 0;
}
int __gthread_cond_timedwait(void *cond, void *mutex, const void *timeout)
{
	(void) cond;
	(void) mutex;
	(void) timeout;
	return 0;
}
int __gthread_cond_destroy(void *cond) { (void) cond; return 0; }

int __gthread_create(gthread_t *thread, void *(*func)(void *), void *arg)
{
	(void) thread;
	(void) func;
	(void) arg;
	return -1;
}
int __gthread_join(gthread_t thread, void **value) { (void) thread; (void) value; return 0; }
int __gthread_detach(gthread_t thread) { (void) thread; return 0; }
int __gthread_equal(gthread_t a, gthread_t b) { return a == b; }

int __gthread_key_create(gthread_key_t *key, void (*dtor)(void *))
{
	(void) key;
	(void) dtor;
	return 0;
}
int __gthread_key_delete(gthread_key_t key) { (void) key; return 0; }
void *__gthread_getspecific(gthread_key_t key) { (void) key; return NULL; }
int __gthread_setspecific(gthread_key_t key, const void *value)
{
	(void) key;
	(void) value;
	return 0;
}
