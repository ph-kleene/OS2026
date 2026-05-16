#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* ── test configuration ───────────────────────────────────────────── */

/* margin added to start/end so the Gantt chart doesn't clip at 0 */
#define CHART_MARGIN_MS  200

/* ── event record ─────────────────────────────────────────────────── */

typedef enum { OP_READ, OP_WRITE } op_t;

typedef struct {
    int      tid;          /* thread id */
    op_t     op;           /* reader or writer */
    /* all times in ms from an arbitrary base */
    long     wait_start;   /* when the thread first tried to lock */
    long     hold_start;   /* when it actually acquired the lock  */
    long     hold_end;     /* when it released the lock           */
} event_t;

#define MAX_EVENTS 32
static event_t  events[MAX_EVENTS];
static int      event_count = 0;
static pthread_mutex_t event_mutex = PTHREAD_MUTEX_INITIALIZER;

static long timebase_ms;   /* set once in main(), before spawning threads */

/* ── helpers ──────────────────────────────────────────────────────── */

static long now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * 1000L + ts.tv_nsec / 1000000L) - timebase_ms;
}

static void record(int tid, op_t op,
                   long ws, long hs, long he)
{
    pthread_mutex_lock(&event_mutex);
    int i = event_count++;
    pthread_mutex_unlock(&event_mutex);
    events[i].tid        = tid;
    events[i].op         = op;
    events[i].wait_start = ws;
    events[i].hold_start = hs;
    events[i].hold_end   = he;
}

/* ── thread argument ──────────────────────────────────────────────── */

typedef struct {
    int         tid;
    op_t        op;
    long        delay_ms;   /* how long after creation to request lock */
    long        hold_ms;    /* how long to hold the lock once acquired  */
    pthread_rwlock_t *rwlock;
    pthread_barrier_t *barrier; /* all threads start together */
} thread_arg_t;

/* ── thread function ──────────────────────────────────────────────── */

static void *rw_thread(void *raw)
{
    thread_arg_t *a = (thread_arg_t *)raw;

    /* wait until all threads are created, then start together */
    pthread_barrier_wait(a->barrier);

    /* delay before requesting the lock (controls ordering) */
    struct timespec ts = { .tv_sec = a->delay_ms / 1000,
                           .tv_nsec = (a->delay_ms % 1000) * 1000000L };
    nanosleep(&ts, NULL);

    long ws = now_ms();

    if (a->op == OP_READ) {
        pthread_rwlock_rdlock(a->rwlock);
    } else {
        pthread_rwlock_wrlock(a->rwlock);
    }

    long hs = now_ms();

    ts.tv_sec  = a->hold_ms / 1000;
    ts.tv_nsec = (a->hold_ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);

    pthread_rwlock_unlock(a->rwlock);
    long he = now_ms();

    record(a->tid, a->op, ws, hs, he);
    return NULL;
}

/* ── Gantt chart ──────────────────────────────────────────────────── */

static void print_gantt(const char *title, int num_threads)
{
    printf("\n━━━ %s ━━━\n\n", title);

    long t_max = 0;
    for (int i = 0; i < event_count; i++) {
        long end = events[i].hold_end;
        if (end > t_max) t_max = end;
    }
    /* add margin */
    t_max += CHART_MARGIN_MS;

    int chars_total = 80;
    double scale = (double)chars_total / t_max;

    printf("time (ms)  ");
    for (int c = 0; c <= chars_total; c += 8)
        printf("%-7ld ", (long)(c / scale));
    printf("\n");

    printf("           ");
    for (int c = 0; c <= chars_total; c++)
        printf("%s", (c % 8 == 0) ? "├" : "─");
    printf("\n");

    for (int t = 1; t <= num_threads; t++) {
        /* find the event for this thread */
        event_t *e = NULL;
        for (int i = 0; i < event_count; i++) {
            if (events[i].tid == t) { e = &events[i]; break; }
        }
        if (!e) { printf("\n"); continue; }

        /* label */
        printf("T%-2d %s  ", t,
               (e->op == OP_READ) ? "[R]" : "[W]");

        int wait_pos  = (int)(e->wait_start * scale);
        int hold_pos  = (int)(e->hold_start * scale);
        int end_pos   = (int)(e->hold_end * scale);
        if (end_pos > chars_total) end_pos = chars_total;

        for (int c = 0; c < chars_total; c++) {
            if (c < wait_pos)
                printf(" ");
            else if (c < hold_pos)
                printf("·");       /* waiting */
            else if (c < end_pos)
                printf("█");       /* holding  */
            else
                printf(" ");
        }
        /* annotation */
        printf("  wait=%ldms hold=%ldms",
               e->hold_start - e->wait_start,
               e->hold_end - e->hold_start);
        printf("\n");
    }

    printf("\nLegend: · = waiting for lock   █ = holding lock\n\n");
}

/* ── run one test scenario ────────────────────────────────────────── */

struct scenario {
    const char *desc;
    int         attr_kind;   /* -1 = default, else PTHREAD_RWLOCK_PREFER_*_NP */
    const char *attr_name;
    int         num_threads;
    int         ops[8];       /* OP_READ / OP_WRITE */
    long        delays[8];    /* ms before requesting lock */
    long        holds[8];     /* ms to hold the lock */
};

static void run_scenario(const struct scenario *s)
{
    /* reset */
    event_count = 0;
    memset(events, 0, sizeof(events));

    pthread_rwlock_t rwlock;
    pthread_rwlockattr_t attr;
    int has_attr = (s->attr_kind >= 0);

    if (has_attr) {
        pthread_rwlockattr_init(&attr);
        pthread_rwlockattr_setkind_np(&attr, s->attr_kind);
        pthread_rwlock_init(&rwlock, &attr);
        pthread_rwlockattr_destroy(&attr);
    } else {
        pthread_rwlock_init(&rwlock, NULL);
    }

    pthread_barrier_t barrier;
    pthread_barrier_init(&barrier, NULL, s->num_threads);

    pthread_t       threads[8];
    thread_arg_t    args[8];

    timebase_ms = 0;
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    timebase_ms = ts.tv_sec * 1000L + ts.tv_nsec / 1000000L;

    for (int i = 0; i < s->num_threads; i++) {
        args[i].tid      = i + 1;
        args[i].op       = (op_t)s->ops[i];
        args[i].delay_ms = s->delays[i];
        args[i].hold_ms  = s->holds[i];
        args[i].rwlock   = &rwlock;
        args[i].barrier  = &barrier;
        pthread_create(&threads[i], NULL, rw_thread, &args[i]);
    }

    for (int i = 0; i < s->num_threads; i++)
        pthread_join(threads[i], NULL);

    pthread_barrier_destroy(&barrier);
    pthread_rwlock_destroy(&rwlock);

    /* title line */
    char title[128];
    snprintf(title, sizeof(title), "Test: %s  [attr = %s]  (%d threads)",
             s->desc, s->attr_name, s->num_threads);
    print_gantt(title, s->num_threads);
}

/* ── main ─────────────────────────────────────────────────────────── */

int main(void)
{
    printf("═══════════════════════════════════════════════════════\n");
    printf("  pthread 读写锁行为测试\n");
    printf("  Linux glibc rwlock attribute modes\n");
    printf("═══════════════════════════════════════════════════════\n");

    /* ────────────────────────────────────────────────────────
     * Scenario A:  Reader arrives first, then Writer, then Reader.
     *   R1 at t=0, hold 3000ms
     *   W2 at t=500ms
     *   R3 at t=1000ms
     *
     * Key question: does R3 share the lock with R1 (read-pref),
     * or does W2 get priority after R1 leaves (write-pref)?
     * ──────────────────────────────────────────────────────── */
    {
        struct scenario s = {
            .desc     = "R1 holds, W2 arrives, then R3",
            .attr_kind = -1,
            .attr_name = "default",
            .num_threads = 3,
            .ops     = { OP_READ, OP_WRITE, OP_READ },
            .delays  = { 0,       500,      1000 },
            .holds   = { 3000,    1000,     1000 },
        };
        run_scenario(&s);
    }
    {
        struct scenario s = {
            .desc     = "R1 holds, W2 arrives, then R3",
            .attr_kind = PTHREAD_RWLOCK_PREFER_READER_NP,
            .attr_name = "PREFER_READER_NP",
            .num_threads = 3,
            .ops     = { OP_READ, OP_WRITE, OP_READ },
            .delays  = { 0,       500,      1000 },
            .holds   = { 3000,    1000,     1000 },
        };
        run_scenario(&s);
    }
    {
        struct scenario s = {
            .desc     = "R1 holds, W2 arrives, then R3",
            .attr_kind = PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP,
            .attr_name = "PREFER_WRITER_NONRECURSIVE_NP",
            .num_threads = 3,
            .ops     = { OP_READ, OP_WRITE, OP_READ },
            .delays  = { 0,       500,      1000 },
            .holds   = { 3000,    1000,     1000 },
        };
        run_scenario(&s);
    }
    {
        struct scenario s = {
            .desc     = "R1 holds, W2 arrives, then R3",
            .attr_kind = PTHREAD_RWLOCK_PREFER_WRITER_NP,
            .attr_name = "PREFER_WRITER_NP",
            .num_threads = 3,
            .ops     = { OP_READ, OP_WRITE, OP_READ },
            .delays  = { 0,       500,      1000 },
            .holds   = { 3000,    1000,     1000 },
        };
        run_scenario(&s);
    }

    /* ────────────────────────────────────────────────────────
     * Scenario B: Writer holds, then Readers arrive (cascaded).
     *   W1 at t=0, hold 3000ms
     *   R2 at t=500ms
     *   W3 at t=1000ms
     *   R4 at t=1500ms
     *
     * Tests whether waiting writers are preferred over waiting readers.
     * ──────────────────────────────────────────────────────── */
    {
        struct scenario s = {
            .desc     = "W1 holds → R2, W3, R4 arrive",
            .attr_kind = PTHREAD_RWLOCK_PREFER_READER_NP,
            .attr_name = "PREFER_READER_NP",
            .num_threads = 4,
            .ops     = { OP_WRITE, OP_READ, OP_WRITE, OP_READ },
            .delays  = { 0,        500,     1000,      1500 },
            .holds   = { 3000,     1000,    1000,      1000 },
        };
        run_scenario(&s);
    }
    {
        struct scenario s = {
            .desc     = "W1 holds → R2, W3, R4 arrive",
            .attr_kind = PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP,
            .attr_name = "PREFER_WRITER_NONRECURSIVE_NP",
            .num_threads = 4,
            .ops     = { OP_WRITE, OP_READ, OP_WRITE, OP_READ },
            .delays  = { 0,        500,     1000,      1500 },
            .holds   = { 3000,     1000,    1000,      1000 },
        };
        run_scenario(&s);
    }

    /* ────────────────────────────────────────────────────────
     * Scenario C: Writer starvation under read preference.
     *   R1 at t=0, hold 2000ms
     *   R2 at t=500ms, hold 2000ms
     *   W3 at t=700ms (arrives while R1,R2 hold → waits)
     *   R4 at t=1200ms (arrives before R1,R2 release → read-pref lets it in)
     *
     * Under read-pref, W3 may wait through multiple reader batches.
     * ──────────────────────────────────────────────────────── */
    {
        struct scenario s = {
            .desc     = "Writer starvation demo (read-pref)",
            .attr_kind = PTHREAD_RWLOCK_PREFER_READER_NP,
            .attr_name = "PREFER_READER_NP",
            .num_threads = 4,
            .ops     = { OP_READ, OP_READ, OP_WRITE, OP_READ },
            .delays  = { 0,       500,     700,      1200 },
            .holds   = { 2000,    2000,    1000,     1000 },
        };
        run_scenario(&s);
    }
    {
        struct scenario s = {
            .desc     = "No writer starvation (write-pref)",
            .attr_kind = PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP,
            .attr_name = "PREFER_WRITER_NONRECURSIVE_NP",
            .num_threads = 4,
            .ops     = { OP_READ, OP_READ, OP_WRITE, OP_READ },
            .delays  = { 0,       500,     700,      1200 },
            .holds   = { 2000,    2000,    1000,     1000 },
        };
        run_scenario(&s);
    }

    /* ────────────────────────────────────────────────────────
     * Scenario D: Multiple writers ordered by arrival (write-pref).
     *   W1 at t=0, hold 2000ms
     *   R2 at t=300ms
     *   W3 at t=600ms
     *   R4 at t=900ms
     *
     * Under write-pref, W3 should get the lock before R2 and R4.
     * ──────────────────────────────────────────────────────── */
    {
        struct scenario s = {
            .desc     = "Multi-writer ordering (read-pref)",
            .attr_kind = PTHREAD_RWLOCK_PREFER_READER_NP,
            .attr_name = "PREFER_READER_NP",
            .num_threads = 4,
            .ops     = { OP_WRITE, OP_READ, OP_WRITE, OP_READ },
            .delays  = { 0,        300,     600,       900 },
            .holds   = { 2000,     1000,    1000,      1000 },
        };
        run_scenario(&s);
    }
    {
        struct scenario s = {
            .desc     = "Multi-writer ordering (write-pref)",
            .attr_kind = PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP,
            .attr_name = "PREFER_WRITER_NONRECURSIVE_NP",
            .num_threads = 4,
            .ops     = { OP_WRITE, OP_READ, OP_WRITE, OP_READ },
            .delays  = { 0,        300,     600,       900 },
            .holds   = { 2000,     1000,    1000,      1000 },
        };
        run_scenario(&s);
    }

    printf("\n═══════════════════════════════════════════════════════\n");
    printf("  测试完成。\n");
    printf("═══════════════════════════════════════════════════════\n");

    return 0;
}
