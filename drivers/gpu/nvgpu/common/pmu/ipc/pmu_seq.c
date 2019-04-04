/*
 * Copyright (c) 2017-2019, NVIDIA CORPORATION.  All rights reserved.
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

#include <nvgpu/pmu/seq.h>
#include <nvgpu/bitops.h>
#include <nvgpu/errno.h>
#include <nvgpu/kmem.h>
#include <nvgpu/bug.h>

void *nvgpu_get_pmu_sequence_in_alloc_ptr_v3(struct pmu_sequence *seq)
{
	return (void *)(&seq->in_v3);
}

void *nvgpu_get_pmu_sequence_in_alloc_ptr_v1(struct pmu_sequence *seq)
{
	return (void *)(&seq->in_v1);
}

void *nvgpu_get_pmu_sequence_out_alloc_ptr_v3(struct pmu_sequence *seq)
{
	return (void *)(&seq->out_v3);
}

void *nvgpu_get_pmu_sequence_out_alloc_ptr_v1(struct pmu_sequence *seq)
{
	return (void *)(&seq->out_v1);
}

int nvgpu_pmu_sequences_alloc(struct gk20a *g,
			      struct pmu_sequences *sequences)
{
	int err;

	sequences->seq = nvgpu_kzalloc(g, PMU_MAX_NUM_SEQUENCES *
				       sizeof(struct pmu_sequence));
	if (sequences->seq == NULL) {
		return -ENOMEM;
	}

	err = nvgpu_mutex_init(&sequences->pmu_seq_lock);
	if (err != 0) {
		nvgpu_kfree(g, sequences->seq);
		return err;
	}

	return 0;
}

void nvgpu_pmu_sequences_free(struct gk20a *g,
			      struct pmu_sequences *sequences)
{
	nvgpu_mutex_destroy(&sequences->pmu_seq_lock);
	nvgpu_kfree(g, sequences->seq);
}

void nvgpu_pmu_sequences_init(struct pmu_sequences *sequences)
{
	u32 i;

	(void) memset(sequences->seq, 0,
		sizeof(struct pmu_sequence) * PMU_MAX_NUM_SEQUENCES);
	(void) memset(sequences->pmu_seq_tbl, 0,
		sizeof(sequences->pmu_seq_tbl));

	for (i = 0; i < PMU_MAX_NUM_SEQUENCES; i++) {
		sequences->seq[i].id = (u8)i;
	}
}

void nvgpu_pmu_seq_payload_free(struct gk20a *g, struct pmu_sequence *seq)
{
	nvgpu_log_fn(g, " ");

	seq->out_payload_fb_queue = false;
	seq->in_payload_fb_queue = false;
	seq->fbq_heap_offset = 0;
	seq->in_mem = NULL;
	seq->out_mem = NULL;
}

int nvgpu_pmu_seq_acquire(struct gk20a *g,
			  struct pmu_sequences *sequences,
			  struct pmu_sequence **pseq,
			  pmu_callback callback, void *cb_params)
{
	struct pmu_sequence *seq;
	unsigned long index;

	nvgpu_mutex_acquire(&sequences->pmu_seq_lock);
	index = find_first_zero_bit(sequences->pmu_seq_tbl,
				sizeof(sequences->pmu_seq_tbl));
	if (index >= sizeof(sequences->pmu_seq_tbl)) {
		nvgpu_err(g, "no free sequence available");
		nvgpu_mutex_release(&sequences->pmu_seq_lock);
		return -EAGAIN;
	}
	nvgpu_assert(index <= U32_MAX);
	set_bit((int)index, sequences->pmu_seq_tbl);
	nvgpu_mutex_release(&sequences->pmu_seq_lock);

	seq = &sequences->seq[index];
	seq->state = PMU_SEQ_STATE_PENDING;
	seq->callback = callback;
	seq->cb_params = cb_params;
	seq->out_payload = NULL;
	seq->in_payload_fb_queue = false;
	seq->out_payload_fb_queue = false;

	*pseq = seq;
	return 0;
}

void nvgpu_pmu_seq_release(struct gk20a *g,
			   struct pmu_sequences *sequences,
			   struct pmu_sequence *seq)
{
	seq->state	= PMU_SEQ_STATE_FREE;
	seq->callback	= NULL;
	seq->cb_params	= NULL;
	seq->out_payload = NULL;

	nvgpu_mutex_acquire(&sequences->pmu_seq_lock);
	clear_bit((int)seq->id, sequences->pmu_seq_tbl);
	nvgpu_mutex_release(&sequences->pmu_seq_lock);
}

u16 nvgpu_pmu_seq_get_fbq_out_offset(struct pmu_sequence *seq)
{
	return seq->fbq_out_offset_in_queue_element;
}

void nvgpu_pmu_seq_set_fbq_out_offset(struct pmu_sequence *seq, u16 size)
{
	seq->fbq_out_offset_in_queue_element = size;
}

u16 nvgpu_pmu_seq_get_buffer_size(struct pmu_sequence *seq)
{
	return seq->buffer_size_used;
}

void nvgpu_pmu_seq_set_buffer_size(struct pmu_sequence *seq, u16 size)
{
	seq->buffer_size_used = size;
}

struct nvgpu_engine_fb_queue *nvgpu_pmu_seq_get_cmd_queue(
					struct pmu_sequence *seq)
{
	return seq->cmd_queue;
}

void nvgpu_pmu_seq_set_cmd_queue(struct pmu_sequence *seq,
				 struct nvgpu_engine_fb_queue *fb_queue)
{
	seq->cmd_queue = fb_queue;
}

u16 nvgpu_pmu_seq_get_fbq_heap_offset(struct pmu_sequence *seq)
{
	return seq->fbq_heap_offset;
}

void nvgpu_pmu_seq_set_fbq_heap_offset(struct pmu_sequence *seq, u16 size)
{
	seq->fbq_heap_offset = size;
}

u8 *nvgpu_pmu_seq_get_out_payload(struct pmu_sequence *seq)
{
	return seq->out_payload;
}

void nvgpu_pmu_seq_set_out_payload(struct pmu_sequence *seq, u8 *payload)
{
	seq->out_payload = payload;
}

void nvgpu_pmu_seq_set_in_payload_fb_queue(struct pmu_sequence *seq, bool state)
{
	seq->in_payload_fb_queue = state;
}

bool nvgpu_pmu_seq_get_out_payload_fb_queue(struct pmu_sequence *seq)
{
	return seq->out_payload_fb_queue;
}

void nvgpu_pmu_seq_set_out_payload_fb_queue(struct pmu_sequence *seq,
					    bool state)
{
	seq->out_payload_fb_queue = state;
}

struct nvgpu_mem *nvgpu_pmu_seq_get_in_mem(struct pmu_sequence *seq)
{
	return seq->in_mem;
}

void nvgpu_pmu_seq_set_in_mem(struct pmu_sequence *seq, struct nvgpu_mem *mem)
{
	seq->in_mem = mem;
}

struct nvgpu_mem *nvgpu_pmu_seq_get_out_mem(struct pmu_sequence *seq)
{
	return seq->out_mem;
}

void nvgpu_pmu_seq_set_out_mem(struct pmu_sequence *seq, struct nvgpu_mem *mem)
{
	seq->out_mem = mem;
}

u32 nvgpu_pmu_seq_get_fbq_element_index(struct pmu_sequence *seq)
{
	return seq->fbq_element_index;
}

void nvgpu_pmu_seq_set_fbq_element_index(struct pmu_sequence *seq, u32 index)
{
	seq->fbq_element_index = index;
}

u8 nvgpu_pmu_seq_get_id(struct pmu_sequence *seq)
{
	return seq->id;
}

enum pmu_seq_state nvgpu_pmu_seq_get_state(struct pmu_sequence *seq)
{
	return seq->state;
}

void nvgpu_pmu_seq_set_state(struct pmu_sequence *seq, enum pmu_seq_state state)
{
	seq->state = state;
}

struct pmu_sequence *nvgpu_pmu_sequences_get_seq(struct pmu_sequences *seqs,
						 u8 id)
{
	return &seqs->seq[id];
}

void nvgpu_pmu_seq_callback(struct gk20a *g, struct pmu_sequence *seq,
			    struct pmu_msg *msg, int err)
{
	if (seq->callback != NULL) {
		seq->callback(g, msg, seq->cb_params, err);
	}
}
