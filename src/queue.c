#include "queue.h"

#include <threads.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

int mr_queue_init(mr_queue_t* q, size_t capacity, size_t element_size) {
    assert(q != NULL);
    assert(capacity > 0);
    assert(element_size > 0);

    q->buffer = malloc(capacity * element_size);
    if (q->buffer == NULL) {
        return -1;
    }

    q->capacity = capacity;
    q->element_size = element_size;
    q->count = 0;
    q->closed = false;

    if (mtx_init(&q->mutex, mtx_plain) != thrd_success) {
        free(q->buffer);
        return -1;
    }

    if (cnd_init(&q->full) != thrd_success) {
        mtx_destroy(&q->mutex);
        free(q->buffer);
        return -1;
    }

    if (cnd_init(&q->empty) != thrd_success) {
        cnd_destroy(&q->full);
        mtx_destroy(&q->mutex);
        free(q->buffer);
        return -1;
    }

    return 0;
}

void mr_queue_destroy(mr_queue_t* q) {
    assert(q != NULL);

    free(q->buffer);
    mtx_destroy(&q->mutex);
    cnd_destroy(&q->full);
    cnd_destroy(&q->empty);
}

int mr_queue_push(mr_queue_t* q, const void* data) {
    assert(q != NULL);
    assert(data != NULL);
    assert(!q->closed);

    if (mtx_lock(&q->mutex) != thrd_success) {
        return -1;
    }

    while (q->count == q->capacity) {
        if (cnd_wait(&q->full, &q->mutex) != thrd_success) {
            mtx_unlock(&q->mutex);
            return -1;
        }
    }

    memcpy(q->buffer + (q->count * q->element_size), data, q->element_size);
    q->count++;

    if (cnd_signal(&q->empty) != thrd_success) {
        mtx_unlock(&q->mutex);
        return -1;
    }
    if (mtx_unlock(&q->mutex) != thrd_success) {
        return -1;
    }

    return 0;
}

int mr_queue_pop(mr_queue_t* q, void* out_data) {
    assert(q != NULL);
    assert(out_data != NULL);

    if (mtx_lock(&q->mutex) != thrd_success) {
        return -1;
    }

    while (q->count == 0 && !q->closed) {
        if (cnd_wait(&q->empty, &q->mutex) != thrd_success) {
            mtx_unlock(&q->mutex);
            return -1;
        }
    }

    if (q->count == 0 && q->closed) {
        mtx_unlock(&q->mutex);
        return 1;
    }

    q->count--;
    memcpy(out_data, q->buffer + (q->count * q->element_size), q->element_size);

    if (cnd_signal(&q->full) != thrd_success) {
        mtx_unlock(&q->mutex);
        return -1;
    }
    if (mtx_unlock(&q->mutex) != thrd_success) {
        return -1;
    }

    return 0;
}

int mr_queue_close(mr_queue_t* q) {
    assert(q != NULL);

    if (mtx_lock(&q->mutex) != thrd_success) {
        return -1;
    }

    q->closed = true;

    if (cnd_broadcast(&q->empty) != thrd_success) {
        mtx_unlock(&q->mutex);
        return -1;
    }
    if (mtx_unlock(&q->mutex) != thrd_success) {
        return -1;
    }

    return 0;
}

int mr_queue_is_closed(mr_queue_t* q, bool* closed) {
    assert(q != NULL);
    assert(closed != NULL);

    if (mtx_lock(&q->mutex) != thrd_success) {
        return -1;
    }

    *closed = q->closed;

    if (mtx_unlock(&q->mutex) != thrd_success) {
        return -1;
    }

    return 0;
}

int mr_queue_is_empty(mr_queue_t* q, bool* empty) {
    assert(q != NULL);
    assert(empty != NULL);

    if (mtx_lock(&q->mutex) != thrd_success) {
        return -1;
    }

    *empty = (q->count == 0);

    if (mtx_unlock(&q->mutex) != thrd_success) {
        return -1;
    }

    return 0;
}
