#include <unity.h>
#include <threads.h>
#include <string.h>
#include <stdlib.h>
#include "queue.h"

void setUp(void) {}
void tearDown(void) {}

void test_queue_init_destroy(void) {
    mr_queue_t q;
    TEST_ASSERT_EQUAL(0, mr_queue_init(&q, 10, sizeof(int)));
    mr_queue_destroy(&q);
}

void test_queue_push_pop(void) {
    mr_queue_t q;
    TEST_ASSERT_EQUAL(0, mr_queue_init(&q, 2, sizeof(int)));

    int val1 = 42;
    int val2 = 24;
    int out;

    TEST_ASSERT_EQUAL(0, mr_queue_push(&q, &val1));
    TEST_ASSERT_EQUAL(0, mr_queue_push(&q, &val2));

    // LIFO behavior
    TEST_ASSERT_EQUAL(0, mr_queue_pop(&q, &out));
    TEST_ASSERT_EQUAL(24, out);

    TEST_ASSERT_EQUAL(0, mr_queue_pop(&q, &out));
    TEST_ASSERT_EQUAL(42, out);

    mr_queue_destroy(&q);
}

void test_queue_close(void) {
    mr_queue_t q;
    TEST_ASSERT_EQUAL(0, mr_queue_init(&q, 10, sizeof(int)));

    int val = 1;
    TEST_ASSERT_EQUAL(0, mr_queue_push(&q, &val));
    TEST_ASSERT_EQUAL(0, mr_queue_close(&q));

    int out;
    TEST_ASSERT_EQUAL(0, mr_queue_pop(&q, &out));
    TEST_ASSERT_EQUAL(1, out);

    // After close and empty, pop should return 1
    TEST_ASSERT_EQUAL(1, mr_queue_pop(&q, &out));

    mr_queue_destroy(&q);
}

typedef struct {
    mr_queue_t* q;
    int count;
} thread_arg_t;

int producer_func(void* arg) {
    thread_arg_t* t_arg = (thread_arg_t*)arg;
    for (int i = 0; i < t_arg->count; i++) {
        mr_queue_push(t_arg->q, &i);
    }
    return 0;
}

int consumer_func(void* arg) {
    thread_arg_t* t_arg = (thread_arg_t*)arg;
    int sum = 0;
    int val;
    while (mr_queue_pop(t_arg->q, &val) == 0) {
        sum += val;
    }
    return sum;
}

void test_queue_concurrency(void) {
    mr_queue_t q;
    TEST_ASSERT_EQUAL(0, mr_queue_init(&q, 5, sizeof(int)));

    thread_arg_t arg = { &q, 100 };
    thrd_t prod, cons;

    thrd_create(&prod, producer_func, &arg);
    thrd_create(&cons, consumer_func, &arg);

    thrd_join(prod, NULL);
    mr_queue_close(&q);

    int sum;
    thrd_join(cons, &sum);

    // Sum of 0..99 = (99 * 100) / 2 = 4950
    TEST_ASSERT_EQUAL(4950, sum);

    mr_queue_destroy(&q);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_queue_init_destroy);
    RUN_TEST(test_queue_push_pop);
    RUN_TEST(test_queue_close);
    RUN_TEST(test_queue_concurrency);
    return UNITY_END();
}
