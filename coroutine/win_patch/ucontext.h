#pragma once

namespace co {

typedef struct sigaltstack {
    void *ss_sp;
    int ss_flags;
    size_t ss_size;
} stack_t;

struct ucontext_t
{
    ucontext_t* uc_link;
    stack_t uc_stack;
    void* arg;
	void (*fn)(void*);
	void* native = nullptr;

	~ucontext_t();
};

extern "C" {

void makecontext(ucontext_t *ucp, void (*func)(), int argc, void* argv);

int swapcontext(ucontext_t *oucp, ucontext_t *ucp);

int getcontext(ucontext_t *ucp);

} //extern "C"

} //namespace co
