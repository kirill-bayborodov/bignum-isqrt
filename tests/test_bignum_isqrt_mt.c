/**
 * @file test_bignum_isqrt_mt.c
 * @brief Multithreaded independent-call test for bignum_isqrt.
 * @version 1.0.0
 * @date 2026-08-22
 * @details Each thread owns all input/output records; the test verifies the
 * documented no-global-state thread-safety guarantee against an integer oracle.
 */
#include "bignum_isqrt.h"
#include <assert.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

struct worker_arg { unsigned id; int failed; };
static uint64_t root64(uint64_t v) { uint64_t lo=0,hi=UINT64_C(0xffffffff),best=0; while(lo<=hi){uint64_t m=lo+((hi-lo)>>1);if(m==0||(__uint128_t)m*m<=v){best=m;lo=m+1;}else hi=m-1;}return best; }
static uint64_t next_random(uint64_t *s) { *s ^= *s << 7; *s ^= *s >> 9; *s ^= *s << 8; return *s; }
static void *worker(void *opaque)
{
    struct worker_arg *arg = opaque; uint64_t s = UINT64_C(0x123456789abcdef0) + arg->id;
    for (size_t i=0; i<2000; ++i) { bignum_t x,r; uint64_t v=next_random(&s), expected=root64(v); memset(&x,0,sizeof x); x.len=v?1:0; x.words[0]=v; memset(&r,0xa5,sizeof r); if(bignum_isqrt(&r,&x)!=BIGNUM_ISQRT_SUCCESS || r.len!=(expected?1U:0U) || (r.len && r.words[0]!=expected)){arg->failed=1;return NULL;} }
    return NULL;
}
int main(void)
{
    enum { THREADS = 8 }; pthread_t threads[THREADS]; struct worker_arg args[THREADS];
    puts("--- Starting multithreaded bignum_isqrt tests ---");
    for (unsigned i=0;i<THREADS;++i){args[i]=(struct worker_arg){i,0};assert(pthread_create(&threads[i],NULL,worker,&args[i])==0);}
    for (unsigned i=0;i<THREADS;++i){assert(pthread_join(threads[i],NULL)==0);assert(args[i].failed==0);}
    puts("--- Multithreaded bignum_isqrt test passed ---"); return 0;
}
