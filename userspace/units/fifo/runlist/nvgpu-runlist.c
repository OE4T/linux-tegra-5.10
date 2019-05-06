/*
 * Copyright (c) 2018-2019, NVIDIA CORPORATION.  All rights reserved.
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

#include <stdlib.h>

#include <unit/io.h>
#include <unit/unit.h>

#include <nvgpu/channel.h>
#include <nvgpu/runlist.h>
#include <nvgpu/gk20a.h>

#include "hal/fifo/runlist_ram_gk20a.h"
#include "hal/fifo/tsg_gk20a.h"

static void setup_fifo(struct gk20a *g, unsigned long *tsg_map,
		unsigned long *ch_map, struct nvgpu_tsg *tsgs,
		struct nvgpu_channel *chs, unsigned int num_tsgs,
		unsigned int num_channels,
		struct nvgpu_runlist_info **runlists, u32 *rl_data,
		bool interleave)
{
	struct nvgpu_fifo *f = &g->fifo;
	struct nvgpu_runlist_info *runlist = runlists[0];

	/* we only use the runlist 0 here */
	runlist->mem[0].cpu_va = rl_data;

	runlist->active_tsgs = tsg_map;
	runlist->active_channels = ch_map;

	g->fifo.g = g;
	/* to debug, change this to (u64)-1 */
	g->log_mask = 0;

	/*
	 * set PTIMER src freq to its nominal frequency to avoid rounding
	 * errors when scaling timeslice.
	 */
	g->ptimer_src_freq = 31250000;

	f->tsg = tsgs;
	f->channel = chs;
	f->num_channels = num_channels;
	f->runlist_info = runlists;

	/*
	 * For testing the runlist entry order format, these simpler dual-u32
	 * entries are enough. The logic is same across chips.
	 */
	f->runlist_entry_size = 2 * sizeof(u32);
	g->ops.runlist.get_tsg_entry = gk20a_runlist_get_tsg_entry;
	g->ops.runlist.get_ch_entry = gk20a_runlist_get_ch_entry;
	g->ops.tsg.default_timeslice_us = nvgpu_tsg_default_timeslice_us;

	g->runlist_interleave = interleave;

	/* set bits in active_tsgs correspond to indices in f->tsg[...] */
	nvgpu_bitmap_set(runlist->active_tsgs, 0, num_tsgs);
	/* same; these are only used if a high enough tsg appears */
	nvgpu_bitmap_set(runlist->active_channels, 0, num_channels);

}

static void setup_tsg(struct nvgpu_tsg *tsgs, struct nvgpu_channel *chs,
		u32 i, u32 level)
{
	struct nvgpu_tsg *tsg = &tsgs[i];
	struct nvgpu_channel *ch = &chs[i];

	tsg->tsgid = i;
	nvgpu_rwsem_init(&tsg->ch_list_lock);
	nvgpu_init_list_node(&tsg->ch_list);
	tsg->num_active_channels = 1;
	tsg->interleave_level = level;

	/* 1:1 mapping for simplicity */
	ch->chid = i;
	nvgpu_list_add_tail(&ch->ch_entry, &tsg->ch_list);
}

static void setup_tsg_multich(struct nvgpu_tsg *tsgs, struct nvgpu_channel *chs,
		u32 i, u32 level, u32 ch_capacity, u32 ch_active)
{
	struct nvgpu_tsg *tsg = &tsgs[i];
	struct nvgpu_channel *ch = &chs[i + 1];
	u32 c;

	setup_tsg(tsgs, chs, i, level);
	tsg->num_active_channels = ch_active;

	/* bind the rest of the channels, onwards from the same id */
	for (c = 1; c < ch_capacity; c++) {
		ch->chid = i + c;
		nvgpu_list_add_tail(&ch->ch_entry, &tsg->ch_list);
		ch++;
	}
}

static int run_format_test(struct unit_module *m, struct nvgpu_fifo *f,
		struct nvgpu_tsg *tsg, struct nvgpu_channel *chs,
		u32 prio, u32 n_ch, u32 *rl_data,
		u32 *expect_header, u32 *expect_channel)
{
	u32 n;

	setup_tsg_multich(tsg, chs, 0, prio, 5, n_ch);

	/* entry capacity: tsg header and some channels */
	n = nvgpu_runlist_construct_locked(f, f->runlist_info[0], 0, 1 + n_ch);
	if (n != 1 + n_ch) {
		unit_return_fail(m, "number of entries mismatch %d\n", n);
	}
	if (memcmp(rl_data, expect_header, 2 * sizeof(u32)) != 0) {
		unit_err(m, "rl_data[0]=%08x", rl_data[0]);
		unit_err(m, "rl_data[1]=%08x", rl_data[1]);
		unit_err(m, "expect_header[0]=%08x", expect_header[0]);
		unit_err(m, "expect_header[1]=%08x", expect_header[1]);

		unit_return_fail(m, "tsg header mismatch\n");
	}
	if (memcmp(rl_data + 2, expect_channel, 2 * n_ch * sizeof(u32)) != 0) {
		unit_return_fail(m, "channel data mismatch\n");
	}

	return UNIT_SUCCESS;
}

static struct tsg_fmt_test_args {
	u32 channels;
	u32 chs_bitmap;
	u32 level;
	u32 timeslice;
	u32 expect_header[2];
	u32 expect_channel[10];
} tsg_fmt_tests[] = {
	/* priority 0, one channel */
	{ 1, 0x01, 0, 0, { 0x0600e000, 0 }, { 0, 0 } },
	/* priority 1, two channels */
	{ 2, 0x03, 1, 0, { 0x0a00e000, 0 }, { 0, 0, 1, 0 } },
	/* priority 2, five channels */
	{ 5, 0x1f, 2, 0, { 0x1600e000, 0 }, { 0, 0, 1, 0, 2, 0, 3, 0, 4, 0 } },
	/* priority 0, one channel, nondefault timeslice timeout */
	{ 1, 0x01, 0, 0xaa<<3, { 0x06a8e000, 0 }, { 0, 0 } },
	/* priority 0, three channels with two inactives in the middle */
	{ 3, 0x01 | 0x04 | 0x10, 0, 0, { 0x0e00e000, 0 }, { 0, 0, 2, 0, 4, 0 } },
};

/*
 * Check that inserting a single tsg of any level with a number of channels
 * works as expected.
 */
static int test_tsg_format_gen(struct unit_module *m, struct gk20a *g,
		void *args)
{
	struct nvgpu_fifo *f = &g->fifo;
	struct nvgpu_runlist_info runlist;
	struct nvgpu_runlist_info *runlists = &runlist;
	unsigned long active_tsgs_map = 0;
	unsigned long active_chs_map = 0;
	struct nvgpu_tsg tsgs[1] = {{0}};
	struct nvgpu_channel chs[5] = {{0}};
	/* header + at most five channels */
	const u32 entries_in_list_max = 1 + 5;
	u32 rl_data[2 * entries_in_list_max];
	u32 ret;
	struct tsg_fmt_test_args *test_args = args;
	(void)test_args->timeslice;

	setup_fifo(g, &active_tsgs_map, &active_chs_map, tsgs, chs, 1, 5,
			&runlists, rl_data, false);

	active_chs_map = test_args->chs_bitmap;

	if (test_args->timeslice == 0U) {
		tsgs[0].timeslice_us = g->ops.tsg.default_timeslice_us(g);
	} else {
		tsgs[0].timeslice_us = test_args->timeslice;
	}

	ret = run_format_test(m, f, &tsgs[0], chs, test_args->level,
			test_args->channels, rl_data,
			test_args->expect_header, test_args->expect_channel);
	if (ret != 0) {
		unit_return_fail(m, "bad format\n");
	}

	return UNIT_SUCCESS;
}

/* compare 1:1 tsg-channel entries against expectations */
static int check_same_simple_tsgs(struct unit_module *m, u32 *expected,
		u32 *actual, u32 n_entries)
{
	u32 i;

	for (i = 0; i < n_entries; i++) {
		u32 want = expected[i];
		/*
		 * 2 u32s per each entry, 2 entries per tsg, and the second
		 * entry of each tsg has the channel id at the first u32.
		 */
		u32 entry_off = 2 * i + 1;
		u32 got = actual[2 * entry_off];
		if (want != got) {
			unit_return_fail(m,
				"wrong entry at %u: expected %u, got %u\n",
				i, want, got);
		}
	}
	return UNIT_SUCCESS;
}

/* Common stuff for all tests below to reduce boilerplate */
static int test_common_gen(struct unit_module *m, struct gk20a *g,
		bool interleave, u32 sizelimit,
		u32 *levels, u32 levels_count,
		u32 *expected, u32 expect_count)
{
	struct nvgpu_fifo *f = &g->fifo;
	struct nvgpu_runlist_info runlist;
	struct nvgpu_runlist_info *runlists = &runlist;
	unsigned long active_tsgs_map = 0;
	unsigned long active_chs_map = 0;
	struct nvgpu_tsg tsgs[6] = {{0}};
	struct nvgpu_channel chs[6] = {{0}};
	u32 tsgs_in_list = expect_count;
	/* a tsg header and a channel entry for each */
	const u32 entries_in_list = 2 * tsgs_in_list;
	/* one entry is two u32s in these tests */
	u32 rl_data[2 * entries_in_list];
	u32 n;
	u32 i = 0;

	setup_fifo(g, &active_tsgs_map, &active_chs_map, tsgs, chs,
			levels_count, 6, &runlists, rl_data, interleave);

	for (i = 0; i < levels_count; i++) {
		setup_tsg(tsgs, chs, i, levels[i]);
	}

	n = nvgpu_runlist_construct_locked(f, &runlist, 0,
			sizelimit != 0U ? sizelimit : entries_in_list);

	if (sizelimit != 0 && sizelimit != entries_in_list) {
		/* Less than enough size is always a negative test here */
		if (n != 0xffffffffU) {
			unit_return_fail(m,
				"limit %d, expected failure, got %u\n",
				sizelimit, n);
		}
		/*
		 * Compare what we got; should be good up until the limit. For
		 * odd limit we miss the last u32 but it's better than nothing.
		 */
		return check_same_simple_tsgs(m, expected, rl_data,
				sizelimit / 2);
	}

	if (n != entries_in_list) {
		unit_return_fail(m, "expected %u entries, got %u\n",
				entries_in_list, n);
	}

	return check_same_simple_tsgs(m, expected, rl_data, tsgs_in_list);
}

static int test_flat_gen(struct unit_module *m, struct gk20a *g, u32 sizelimit)
{
	u32 levels[] = {
		/* Some random-ish order of priority levels */
		0, 1, 2, 1, 0, 2,
	};
	u32 expected[] = {
		/* High (2) indices first, then medium (1), then low (0). */
		2, 5, 1, 3, 0, 4,
	};

	return test_common_gen(m, g, false, sizelimit,
			levels, ARRAY_SIZE(levels),
			expected, ARRAY_SIZE(expected));
}

/*
 * Test the normal case that a successful construct is correct.
 */
static int test_flat(struct unit_module *m, struct gk20a *g, void *args)
{
	return test_flat_gen(m, g, 0);
}

/*
 * Just a corner case, space for just one tsg header; even the first channel
 * entry doesn't fit.
 */
static int test_flat_oversize_tiny(struct unit_module *m, struct gk20a *g,
		void *args)
{
	return test_flat_gen(m, g, 1);
}

/*
 * One tsg header with its channel fit.
 */
static int test_flat_oversize_single(struct unit_module *m, struct gk20a *g,
		void *args)
{
	return test_flat_gen(m, g, 2);
}

/*
 * The second channel would get chopped off.
 */
static int test_flat_oversize_onehalf(struct unit_module *m, struct gk20a *g,
		void *args)
{
	return test_flat_gen(m, g, 3);
}

/*
 * Two full entries fit exactly.
 */
static int test_flat_oversize_two(struct unit_module *m, struct gk20a *g,
		void *args)
{
	return test_flat_gen(m, g, 4);
}

/*
 * All but the last channel entry fit.
 */
static int test_flat_oversize_end(struct unit_module *m, struct gk20a *g,
		void *args)
{
	return test_flat_gen(m, g, 11);
}

/* Common stuff for all tests below */
static int test_interleaving_gen(struct unit_module *m, struct gk20a *g,
		u32 sizelimit,
		u32 *levels, u32 levels_count,
		u32 *expected, u32 expect_count)
{
	return test_common_gen(m, g, true, sizelimit, levels, levels_count,
			expected, expect_count);
}

/* Items in all levels, interleaved */
static int test_interleaving_gen_all(struct unit_module *m, struct gk20a *g,
		u32 sizelimit)
{
	/*
	 * Named channel ids for us humans to parse.
	 *
	 * This works such that the first two TSGs, IDs 0 and 1 (with just one
	 * channel each) are at interleave level "low" ("l0"), the next IDs 2
	 * and 3 are at level "med" ("l2"), and the last IDs 4 and 5 are at
	 * level "hi" ("l2"). Runlist construction doesn't care, so we use an
	 * easy to understand order.
	 *
	 * When debugging this test and/or the runlist code, the logs of any
	 * interleave test should follow the order in the "expected" array. We
	 * start at the highest level, so the first IDs added should be h1 and
	 * h2, i.e., 4 and 5, etc.
	 */
	u32 l1 = 0, l2 = 1;
	u32 m1 = 2, m2 = 3;
	u32 h1 = 4, h2 = 5;
	u32 levels[] = { 0, 0, 1, 1, 2, 2 };
	u32 expected[] = {
		/* Order of channel ids; partly used also for oversize tests */
		h1, h2, m1, h1, h2, m2, h1, h2, l1,
		h1, h2, m1, h1, h2, m2, h1, h2, l2,
	};

	return test_interleaving_gen(m, g, sizelimit,
			levels, ARRAY_SIZE(levels),
			expected, ARRAY_SIZE(expected));
}

static struct interleave_test_args {
	u32 sizelimit;
} interleave_tests[] = {
/* All priority items. */
	{ 0 },
/* Fail at level 2 immediately: space for just a tsg header, no ch entries. */
	{ 1 },
/* Insert both l2 entries, then fail at l1 level. */
	{ 2 * 2 },
/* Insert both l2 entries, one l1, and just one l2: fail at last l2. */
	{ (2 + 1 + 1) * 2 },
/* Stop at exactly the first l2 entry in the first l1-l0 transition. */
	{ (2 + 1 + 2 + 1) * 2 },
/* Stop at exactly the first l0 entry that doesn't fit. */
	{ (2 + 1 + 2 + 1 + 2) * 2 },
};

static int test_interleaving_gen_all_run(struct unit_module *m,
		struct gk20a *g, void *args)
{
	struct interleave_test_args *test_args = args;

	return test_interleaving_gen_all(m, g, test_args->sizelimit);
}

/*
 * Only l0 items.
 */
static int test_interleaving_l0(struct unit_module *m,
		struct gk20a *g, void *args)
{
	u32 levels[] = { 0, 0 };
	/* The channel id sequence is trivial here and in most of the below */
	u32 expected[] = { 0, 1 };

	return test_interleaving_gen(m, g, 2 * ARRAY_SIZE(expected),
			levels, ARRAY_SIZE(levels),
			expected, ARRAY_SIZE(expected));
}

/*
 * Only l1 items.
 */
static int test_interleaving_l1(struct unit_module *m,
		struct gk20a *g, void *args)
{
	u32 levels[] = { 1, 1 };
	u32 expected[] = { 0, 1 };

	return test_interleaving_gen(m, g, 2 * ARRAY_SIZE(expected),
			levels, ARRAY_SIZE(levels),
			expected, ARRAY_SIZE(expected));
}

/*
 * Only l2 items.
 */
static int test_interleaving_l2(struct unit_module *m,
		struct gk20a *g, void *args)
{
	u32 levels[] = { 2, 2 };
	u32 expected[] = { 0, 1 };

	return test_interleaving_gen(m, g, 2 * ARRAY_SIZE(expected),
			levels, ARRAY_SIZE(levels),
			expected, ARRAY_SIZE(expected));
}

/*
 * Only low and medium priority items.
 */
static int test_interleaving_l0_l1(struct unit_module *m,
		struct gk20a *g, void *args)
{
	u32 l1 = 0, l2 = 1, m1 = 2, m2 = 3;
	u32 levels[] = { 0, 0, 1, 1 };
	u32 expected[] = { m1, m2, l1, m1, m2, l2 };

	return test_interleaving_gen(m, g, 2 * ARRAY_SIZE(expected),
			levels, ARRAY_SIZE(levels),
			expected, ARRAY_SIZE(expected));
}

/*
 * Only medium and high priority items.
 */
static int test_interleaving_l1_l2(struct unit_module *m,
		struct gk20a *g, void *args)
{
	u32 m1 = 0, m2 = 1, h1 = 2, h2 = 3;
	u32 levels[] = { 1, 1, 2, 2 };
	u32 expected[] = { h1, h2, m1, h1, h2, m2 };

	return test_interleaving_gen(m, g, 2 * ARRAY_SIZE(expected),
			levels, ARRAY_SIZE(levels),
			expected, ARRAY_SIZE(expected));
}

/*
 * Only low and high priority items.
 */
static int test_interleaving_l0_l2(struct unit_module *m,
		struct gk20a *g, void *args)
{
	u32 l1 = 0, l2 = 1, h1 = 2, h2 = 3;
	u32 levels[] = { 0, 0, 2, 2 };
	u32 expected[] = { h1, h2, l1, h1, h2, l2 };

	return test_interleaving_gen(m, g, 2 * ARRAY_SIZE(expected),
			levels, ARRAY_SIZE(levels),
			expected, ARRAY_SIZE(expected));
}

struct unit_module_test nvgpu_runlist_tests[] = {
	UNIT_TEST(tsg_format_ch1, test_tsg_format_gen, &tsg_fmt_tests[0], 0),
	UNIT_TEST(tsg_format_ch2, test_tsg_format_gen, &tsg_fmt_tests[1], 0),
	UNIT_TEST(tsg_format_ch5, test_tsg_format_gen, &tsg_fmt_tests[2], 0),
	UNIT_TEST(tsg_format_ch1_timeslice, test_tsg_format_gen,
			&tsg_fmt_tests[3], 0),
	UNIT_TEST(tsg_format_ch3_inactive2, test_tsg_format_gen,
			&tsg_fmt_tests[4], 0),

	UNIT_TEST(flat, test_flat, NULL, 0),

	UNIT_TEST(flat_oversize_tiny, test_flat_oversize_tiny, NULL, 0),
	UNIT_TEST(flat_oversize_single, test_flat_oversize_single, NULL, 0),
	UNIT_TEST(flat_oversize_onehalf, test_flat_oversize_onehalf, NULL, 0),
	UNIT_TEST(flat_oversize_two, test_flat_oversize_two, NULL, 0),
	UNIT_TEST(flat_oversize_end, test_flat_oversize_end, NULL, 0),

	UNIT_TEST(interleaving,
		  test_interleaving_gen_all_run, &interleave_tests[0], 0),

	UNIT_TEST(interleaving_oversize_tiny,
		  test_interleaving_gen_all_run, &interleave_tests[1], 0),
	UNIT_TEST(interleaving_oversize_l2,
		  test_interleaving_gen_all_run, &interleave_tests[2], 0),
	UNIT_TEST(interleaving_oversize_l2_l1_l2,
		  test_interleaving_gen_all_run, &interleave_tests[3], 0),
	UNIT_TEST(interleaving_oversize_l2_l1_l2_l1,
		  test_interleaving_gen_all_run, &interleave_tests[4], 0),
	UNIT_TEST(interleaving_oversize_l2_l1_l2_l1_l2,
		  test_interleaving_gen_all_run, &interleave_tests[5], 0),

	UNIT_TEST(interleaving_l0, test_interleaving_l0, NULL, 0),
	UNIT_TEST(interleaving_l1, test_interleaving_l1, NULL, 0),
	UNIT_TEST(interleaving_l2, test_interleaving_l2, NULL, 0),
	UNIT_TEST(interleaving_l0_l1, test_interleaving_l0_l1, NULL, 0),
	UNIT_TEST(interleaving_l1_l2, test_interleaving_l1_l2, NULL, 0),
	UNIT_TEST(interleaving_l0_l2, test_interleaving_l0_l2, NULL, 0),
};

UNIT_MODULE(nvgpu_runlist, nvgpu_runlist_tests, UNIT_PRIO_NVGPU_TEST);
