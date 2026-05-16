#include "ringBuffer.h"

#include <cstdio>
#include <cstdlib>
#include <sys/time.h>

/* ── test parameters ──────────────────────────────────────────────── */

static constexpr int  TOTAL_ITEMS   = 200000;
static constexpr int  NUM_PRODUCERS = 4;
static constexpr int  NUM_CONSUMERS = 4;
static constexpr int  BUF_CAPACITY  = 16;

static constexpr int  SENTINEL      = -1;

/* ── simple LCG ───────────────────────────────────────────────────── */

static inline int fast_rand(unsigned &seed)
{
    seed = seed * 1103515245U + 12345U;
    return (int)((seed >> 16) & 0x7FFF);
}

/* ── shared buffer ────────────────────────────────────────────────── */

/* Each thread accumulates its own sum to avoid mutex contention.
   After joins, the main thread aggregates. */

struct thread_stats {
    long sum;
    int  count;
};

/* ── producer ─────────────────────────────────────────────────────── */

struct producer_arg {
    ringBuffer  *buf;
    unsigned     seed;
    thread_stats stats;
};

static void *producer(void *raw)
{
    auto    *a   = static_cast<producer_arg *>(raw);
    unsigned seed = a->seed;
    a->stats.sum   = 0;
    a->stats.count = 0;

    for (int i = 0; i < TOTAL_ITEMS; i++) {
        int val = fast_rand(seed) % 1000;
        a->buf->put(val);
        a->stats.sum   += val;
        a->stats.count++;
    }
    return nullptr;
}

/* ── consumer ─────────────────────────────────────────────────────── */

struct consumer_arg {
    ringBuffer  *buf;
    thread_stats stats;
};

static void *consumer(void *raw)
{
    auto *a = static_cast<consumer_arg *>(raw);
    a->stats.sum   = 0;
    a->stats.count = 0;

    for (;;) {
        int val;
        a->buf->get(val);

        if (val == SENTINEL)
            break;

        a->stats.sum   += val;
        a->stats.count++;
    }
    return nullptr;
}

/* ── unit tests ───────────────────────────────────────────────────── */

static bool test_fifo_ordering()
{
    ringBuffer buf(4);
    /* put 2, get 1, put 2, get 3 — verifies wraparound ordering */
    buf.put(10);
    buf.put(20);
    int out;
    buf.get(out);
    if (out != 10) return false;
    buf.put(30);
    buf.put(40);   /* buffer now full: [20,30,40] at tail=1,head=0 */
    buf.get(out); if (out != 20) return false;
    buf.get(out); if (out != 30) return false;
    buf.get(out); if (out != 40) return false;
    if (!buf.empty()) return false;
    return true;
}

static bool test_empty_full()
{
    ringBuffer buf(3);
    if (!buf.empty() || buf.full())   return false;
    buf.put(1);
    if (buf.empty() || buf.full())    return false;
    buf.put(2); buf.put(3);
    if (buf.empty() || !buf.full())   return false;
    int x;
    buf.get(x);
    if (buf.full())                   return false;
    buf.get(x); buf.get(x);
    if (!buf.empty())                 return false;
    return true;
}

/* ── main ─────────────────────────────────────────────────────────── */

int main()
{
    printf("═══════════════════════════════════════════════════════\n");
    printf("  ringBuffer 多线程同步测试\n");
    printf("═══════════════════════════════════════════════════════\n\n");

    printf("─── 基本正确性测试 ───\n");
    printf("FIFO ordering:  %s\n", test_fifo_ordering() ? "PASS" : "FAIL");
    printf("empty/full:     %s\n", test_empty_full()    ? "PASS" : "FAIL");
    printf("\n");

    /* ── multi-threaded test ─────────────────────────────────────── */
    printf("─── 多线程压力测试 ───\n");
    printf("Producers: %d   Consumers: %d   Items/producer: %d   Capacity: %d\n",
           NUM_PRODUCERS, NUM_CONSUMERS, TOTAL_ITEMS, BUF_CAPACITY);

    struct timeval tv_start, tv_end;
    gettimeofday(&tv_start, nullptr);

    ringBuffer buf(BUF_CAPACITY);

    pthread_t     p_threads[NUM_PRODUCERS];
    pthread_t     c_threads[NUM_CONSUMERS];
    producer_arg  p_args[NUM_PRODUCERS];
    consumer_arg  c_args[NUM_CONSUMERS];

    for (int i = 0; i < NUM_CONSUMERS; i++) {
        c_args[i].buf = &buf;
        pthread_create(&c_threads[i], nullptr, consumer, &c_args[i]);
    }

    for (int i = 0; i < NUM_PRODUCERS; i++) {
        p_args[i].buf  = &buf;
        p_args[i].seed = (unsigned)(42 + i * 9973);
        pthread_create(&p_threads[i], nullptr, producer, &p_args[i]);
    }

    /* wait for producers */
    for (int i = 0; i < NUM_PRODUCERS; i++)
        pthread_join(p_threads[i], nullptr);

    /* inject sentinels */
    for (int i = 0; i < NUM_CONSUMERS; i++)
        buf.put(SENTINEL);

    /* wait for consumers */
    for (int i = 0; i < NUM_CONSUMERS; i++)
        pthread_join(c_threads[i], nullptr);

    gettimeofday(&tv_end, nullptr);
    long elapsed_ms = (tv_end.tv_sec  - tv_start.tv_sec)  * 1000L
                    + (tv_end.tv_usec - tv_start.tv_usec) / 1000L;

    /* aggregate stats */
    long prod_sum = 0, cons_sum = 0;
    int  prod_cnt = 0, cons_cnt = 0;
    for (int i = 0; i < NUM_PRODUCERS; i++) {
        prod_sum += p_args[i].stats.sum;
        prod_cnt += p_args[i].stats.count;
    }
    for (int i = 0; i < NUM_CONSUMERS; i++) {
        cons_sum += c_args[i].stats.sum;
        cons_cnt += c_args[i].stats.count;
    }

    int  expected_items = NUM_PRODUCERS * TOTAL_ITEMS;
    long sum_diff       = prod_sum - cons_sum;

    printf("\n─── 验证结果 ───\n");
    printf("Expected items:       %d\n",  expected_items);
    printf("Items produced:       %d\n",  prod_cnt);
    printf("Items consumed:       %d\n",  cons_cnt);
    printf("Produced sum:         %ld\n", prod_sum);
    printf("Consumed sum:         %ld\n", cons_sum);
    printf("Sum difference:       %ld\n", sum_diff);
    printf("Elapsed:              %ld ms\n", elapsed_ms);

    bool count_ok = (prod_cnt == expected_items && cons_cnt == expected_items);
    bool sum_ok   = (sum_diff == 0);

    printf("\nItem count integrity: %s\n", count_ok ? "PASS" : "FAIL");
    printf("Value sum integrity:  %s\n", sum_ok   ? "PASS" : "FAIL");
    printf("Overall:              %s\n",
           (count_ok && sum_ok) ? "PASS" : "FAIL");

    printf("\n═══════════════════════════════════════════════════════\n");
    return (count_ok && sum_ok) ? 0 : 1;
}
