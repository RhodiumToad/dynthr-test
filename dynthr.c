/*
 * Copyright (C) 2019 Andrew Gierth
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Though this file is initially distributed under the 2-clause BSD license,
 * the author grants permission for its redistribution under alternative
 * licenses as set forth at <https://rhodiumtoad.github.io/RELICENSE.txt>.
 * This paragraph and the RELICENSE.txt file are not part of the license and
 * may be omitted in redistributions.
 */

#include <stdio.h>
#include <stdlib.h>
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

typedef void (modfunc_t)(int op);
	
#ifdef MODULE

static void *mod_thread(void *ptr)
{
	char *volatile dummy;
	dummy = malloc(500);
	say("thread running");
	dummy = malloc(500);
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
	char *volatile dummy;
	alarm(3);
	say("starting");
#ifdef PRELOAD_LIBTHR
	void *thr_handle = dlopen("libthr.so", RTLD_GLOBAL|RTLD_NOW);
	if (!thr_handle)
		die("failed to open libthr.so: %s", dlerror());
#endif
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
	dummy = malloc(500);
	say("done; invoking module stop");
	func(0);
	say("stopped successfully");
	return 0;
}

#endif
