/* -*- C -*- */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#include <dlfcn.h>

#ifdef MODULE
#include <pthread.h>
#endif

static void vsay(const char *str, va_list va)
{
	char buf[2048];
	int n = vsnprintf(buf, sizeof(buf), str, va);
	if (n >= sizeof(buf))
		n = sizeof(buf)-1;
	buf[n] = '\n';
	write(2, buf, n+1);
}

static void say(const char *str, ...)
{
	va_list va;
	va_start(va, str);
	vsay(str, va);
	va_end(va);
}

static void die(const char *str, ...)
{
	va_list va;
	va_start(va, str);
	vsay(str, va);
	va_end(va);
	_exit(1);
}

static void thrash_memory(int id)
{
	char *ptrs[20];
	for (int n = 0; n < 5; ++n)
	{
		for (int i = 0; i < 20; ++i)
			ptrs[i] = malloc((random() & 0x7ff) << ((random() & 7) + 3));
		for (int i = 0; i < 20; ++i)
			free(ptrs[i]);
		say("%d: iteration %d", id, n);
	}
}

typedef void (modfunc_t)(int op);
	
#ifdef MODULE

static void *mod_thread(void *ptr)
{
	char *volatile dummy;
	dummy = malloc(500);
	say("thread running");
	thrash_memory(1);
	return NULL;
}

modfunc_t mod_main;

static pthread_t thr;

void mod_main(int op)
{
	char *volatile dummy;
	say("module invoked: op=%d", op);
	switch (op)
	{
		case 1:
		{
			int rc = pthread_create(&thr, NULL, mod_thread, NULL);
			if (rc)
				die("thread creation failed: %s", strerror(rc));
			dummy = malloc(500);
			say("thread started");
			return;
		}
			
		case 0:
		{
			say("waiting for thread");
			pthread_join(thr, NULL);
			say("thread stopped");
			return;
		}
	}
}

#else

int main()
{
	alarm(3);
	say("starting");
	void *mod_handle = dlopen("./dynthr_mod.so", RTLD_LOCAL);
	if (!mod_handle)
		die("failed to open dynthr_mod.so: %s", dlerror());
	say("module loaded");
	dlfunc_t rawfunc = dlfunc(mod_handle, "mod_main");
	if (!rawfunc)
		die("failed to resolve function mod_main");
	say("invoking module");
	modfunc_t *func = (modfunc_t *) rawfunc;
	func(1);
	thrash_memory(0);
	say("done; invoking module stop");
	func(0);
	say("stopped successfully");
	return 0;
}

#endif
