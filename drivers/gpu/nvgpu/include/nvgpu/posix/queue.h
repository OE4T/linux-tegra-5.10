/*
 * Copyright (c) 2018-2020, NVIDIA CORPORATION.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef NVGPU_POSIX_QUEUE_H
#define NVGPU_POSIX_QUEUE_H

#include <nvgpu/lock.h>
#ifdef NVGPU_UNITTEST_FAULT_INJECTION_ENABLEMENT
struct nvgpu_posix_fault_inj;
#endif

struct nvgpu_queue {
	/**
	 * Index where messages will be enqueued.
	 */
	unsigned int in;
	/**
	 * Index from where messages will be dequeued.
	 */
	unsigned int out;
	/**
	 * Size of the allocated message queue minus 1.
	 */
	unsigned int mask;
	/**
	 * Points to the allocated memory for the message queue.
	 */
	unsigned char *data;
};

/**
 * @brief Calculate the unused message queue length.
 *
 * @param queue [in]	Queue structure to use.
 *
 * The size of all the messages currently enqueued is subtracted from the total
 * size of the queue to get the unused queue length.
 *
 * @return Return unused queue length.
 */
unsigned int nvgpu_queue_unused(struct nvgpu_queue *queue);

/**
 * @brief Calculate the length of the message queue in use.
 *
 * @param queue [in]	Queue structure to use.
 *
 * Returns the size of all the messages currently enqueued in the queue.
 *
 * @return Return the size of all the messages currently enqueued in the queue.
 */
unsigned int nvgpu_queue_available(struct nvgpu_queue *queue);

/**
 * @brief Allocate memory and initialize the message queue variables.
 *
 * @param queue [in]	Queue structure to use.
 * @param size [in]	Size of the queue.
 *
 * Allocates memory for the message queue. Also initializes the message queue
 * variables "in", "out" and "mask".
 *
 * @return Return 0 on success, otherwise returns error number to indicate the
 * error.
 */
int nvgpu_queue_alloc(struct nvgpu_queue *queue, unsigned int size);

/**
 * @brief Free the allocated memory for the message queue.
 *
 * @param queue [in]	Queue structure to use.
 *
 * Free the allocated memory for the message queue. Also resets the message
 * queue variables "in", "out" and "mask".
 */
void nvgpu_queue_free(struct nvgpu_queue *queue);

/**
 * @brief Enqueue message into message queue.
 *
 * @param queue [in]	Queue structure to use.
 * @param buf [in]	Pointer to source message buffer.
 * @param len [in]	Size of the message to be enqueued.
 *
 * Enqueues the message pointed by \a buf into the message queue and advances
 * the "in" index of the message queue by \a len which is the size of the
 * enqueued message.
 *
 * @return Returns \a len on success, otherwise returns error number to indicate
 * the error.
 */
int nvgpu_queue_in(struct nvgpu_queue *queue, const void *buf,
		unsigned int len);

/**
 * @brief Enqueue message into message queue.
 *
 * @param queue [in]	Queue structure to use.
 * @param buf [in]	Pointer to source message buffer.
 * @param len [in]	Size of the message to be enqueued.
 * @param lock [in]	Mutex lock for concurrency management.
 *
 * Enqueues the message pointed by \a buf into the message queue and advances
 * the "in" index of the message queue by \a len which is the size of the
 * enqueued message.
 *
 * @return Returns \a len on success, otherwise returns error number to indicate
 * the error.
 */
int nvgpu_queue_in_locked(struct nvgpu_queue *queue, const void *buf,
		unsigned int len, struct nvgpu_mutex *lock);

/**
 * @brief Dequeue message from message queue.
 *
 * @param queue [in]	Queue structure to use.
 * @param buf [in]	Pointer to destination message buffer.
 * @param len [in]	Size of the message to be dequeued.
 *
 * Dequeues the message of size \a len from the message queue and copies the
 * same to \a buf. Also advances the "out" index of the queue by \a len.
 *
 * @return Returns \a len on success, otherwise returns error number to indicate
 * the error.
 */
int nvgpu_queue_out(struct nvgpu_queue *queue, void *buf,
		unsigned int len);

/**
 * @brief Dequeue message from message queue.
 *
 * @param queue [in]	Queue structure to use.
 * @param buf [in]	Pointer to destination message buffer.
 * @param len [in]	Size of the message to be dequeued.
 * @param lock [in]	Mutex lock for concurrency management.
 *
 * Dequeues the message of size \a len from the message queue and copies the
 * same to \a buf. Also advances the "out" index of the queue by \a len.
 *
 * @return Returns \a len on success, otherwise returns error number to indicate
 * the error.
 */
int nvgpu_queue_out_locked(struct nvgpu_queue *queue, void *buf,
		unsigned int len, struct nvgpu_mutex *lock);
#ifdef NVGPU_UNITTEST_FAULT_INJECTION_ENABLEMENT
struct nvgpu_posix_fault_inj *nvgpu_queue_out_get_fault_injection(void);
#endif

#endif
