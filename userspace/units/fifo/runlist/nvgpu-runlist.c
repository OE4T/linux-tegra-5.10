/*
 * Copyright (c) 2018, NVIDIA CORPORATION.  All rights reserved.
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
#include <nvgpu/gk20a.h>

static void setup_fifo(struct gk20a *g, unsigned long *tsg_map,
		unsigned long *ch_map, struct tsg_gk20a *tsgs,
		struct channel_gk20a *chs, unsigned int num_tsgs,
		unsigned int num_channels,
		struct fifo_runlist_info_gk20a *runlist, u32 *rl_data,
		bool interleave)
{
	struct fifo_gk20a *f = &g->fifo;

	/* we only use the runlist 0 here */
	runlist->mem[0].cpu_va = rl_data;

	runlist->active_tsgs = tsg_map;
	runlist->active_channels = ch_map;

	g->fifo.g = g;
	/* to debug, change this to (u64)-1 */
	g->log_mask = 0;

	f->tsg = tsgs;
	f->channel = chs;
	f->num_channels = num_channels;
	f->runlist_info = runlist;

	/*
	 * For testing the runlist entry order format, these simpler dual-u32
	 * entries are enough. The logic is same across chips.
	 */
	f->runlist_entry_size = 2 * sizeof(u32);
	g->ops.fifo.get_tsg_runlist_entry = gk20a_get_tsg_runlist_entry;
	g->ops.fifo.get_ch_runlist_entry = gk20a_get_ch_runlist_entry;

	g->runlist_interleave = interleave;

	/* set bits in active_tsgs correspond to indices in f->tsg[...] */
	bitmap_set(runlist->active_tsgs, 0, num_tsgs);
	/* same; these are only used if a high enough tsg appears */
	bitmap_set(runlist->active_channels, 0, num_channels);

}

static void setup_tsg(struct tsg_gk20a *tsgs, struct channel_gk20a *chs,
		u32 i, u32 level)
{
	struct tsg_gk20a *tsg = &tsgs[i];
	struct channel_gk20a *ch = &chs[i];

	tsg->tsgid = i;
	nvgpu_rwsem_init(&tsg->ch_list_lock);
	nvgpu_init_list_node(&tsg->ch_list);
	tsg->num_active_channels = 1;
	tsg->interleave_level = level;

	/* 1:1 mapping for simplicity */
	ch->chid = i;
	nvgpu_list_add_tail(&ch->ch_entry, &tsg->ch_list);
}

static void setup_tsg_multich(struct tsg_gk20a *tsgs, struct channel_gk20a *chs,
		u32 i, u32 level, u32 ch_n)
{
	struct tsg_gk20a *tsg = &tsgs[i];
	struct channel_gk20a *ch = &chs[i + 1];
	u32 c;

	setup_tsg(tsgs, chs, i, level);
	tsg->num_active_channels = ch_n;

	/* bind the rest of the channels, onwards from the same id */
	for (c = 1; c < ch_n; c++) {
		ch->chid = i + c;
		nvgpu_list_add_tail(&ch->ch_entry, &tsg->ch_list);
		ch++;
	}
}

static int run_format_test(struct unit_module *m, struct fifo_gk20a *f,
		struct tsg_gk20a *tsg, struct channel_gk20a *chs,
		u32 prio, u32 n_ch, u32 *rl_data,
		u32 *expect_header, u32 *expect_channel)
{
	u32 n;

	setup_tsg_multich(tsg, chs, 0, prio, n_ch);

	/* entry capacity: tsg header and some channels */
	n = nvgpu_runlist_construct_locked(f, f->runlist_info, 0, 1 + n_ch);
	if (n != 1 + n_ch) {
		unit_return_fail(m, "number of entries mismatch %d\n", n);
	}
	if (memcmp(rl_data, expect_header, 2 * sizeof(u32)) != 0) {
		unit_return_fail(m, "tsg header mismatch\n");
	}
	if (memcmp(rl_data + 2, expect_channel, 2 * n_ch * sizeof(u32)) != 0) {
		unit_return_fail(m, "channel data mismatch\n");
	}

	return UNIT_SUCCESS;
}

/*
 * Check that inserting a single tsg of any level with a number of channels
 * works as expected.
 */
static int test_tsg_format(struct unit_module *m, struct gk20a *g, void *args)
{
	struct fifo_gk20a *f = &g->fifo;
	struct fifo_runlist_info_gk20a runlist;
	unsigned long active_tsgs_map = 0;
	unsigned long active_chs_map = 0;
	struct tsg_gk20a tsg = {0};
	struct channel_gk20a chs[5] = {{0}};
	/* header + at most five channels */
	const u32 entries_in_list_max = 1 + 5;
	u32 rl_data[2 * entries_in_list_max];
	u32 ret;
	/* top bits: number of active channels. two u32s per entry */
	u32 expect_header_prio[3][2] = {
		{ 0x0600e000, 0 },
		{ 0x0a00e000, 0 },
		{ 0x1600e000, 0 }
	};
	u32 expect_channel[] = {
		0, 0,
		1, 0,
		2, 0,
		3, 0,
		4, 0
	};

	setup_fifo(g, &active_tsgs_map, &active_chs_map, &tsg, chs, 1, 6,
			&runlist, rl_data, false);

	/* priority 0, one channel */
	ret = run_format_test(m, f, &tsg, chs, 0, 1, rl_data,
			expect_header_prio[0], expect_channel);
	if (ret != 0) {
		unit_return_fail(m, "bad format, channels: 1\n");
	}

	/* priority 1, two channels */
	ret = run_format_test(m, f, &tsg, chs, 1, 2, rl_data,
			expect_header_prio[1], expect_channel);
	if (ret != 0) {
		unit_return_fail(m, "bad format, channels: 2\n");
	}

	/* priority 2, five channels */
	ret = run_format_test(m, f, &tsg, chs, 2, 5, rl_data,
			expect_header_prio[2], expect_channel);
	if (ret != 0) {
		unit_return_fail(m, "bad format, channels: 5\n");
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
	struct fifo_gk20a *f = &g->fifo;
	struct fifo_runlist_info_gk20a runlist;
	unsigned long active_tsgs_map = 0;
	unsigned long active_chs_map = 0;
	struct tsg_gk20a tsgs[6] = {{0}};
	struct channel_gk20a chs[6] = {{0}};
	u32 tsgs_in_list = expect_count;
	/* a tsg header and a channel entry for each */
	const u32 entries_in_list = 2 * tsgs_in_list;
	/* one entry is two u32s in these tests */
	u32 rl_data[2 * entries_in_list];
	u32 n;
	u32 i = 0;

	setup_fifo(g, &active_tsgs_map, &active_chs_map, tsgs, chs,
			levels_count, 6, &runlist, rl_data, interleave);

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
	/* Named channel ids for us humans to parse */
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

/*
 * Test construction of all priority items.
 */
static int test_interleaving(struct unit_module *m, struct gk20a *g, void *args)
{
	return test_interleaving_gen_all(m, g, 0);
}

/*
 * Fail at level 0 immediately: space for just a tsg header.
 */
static int test_interleaving_oversize_tiny(struct unit_module *m,
		struct gk20a *g, void *args)
{
	return test_interleaving_gen_all(m, g, 1);
}

/*
 * Insert a single l0 entry, then descend to l2 and fail there after one l2
 * entry.
 */
static int test_interleaving_oversize_l0_l2(struct unit_module *m,
		struct gk20a *g, void *args)
{
	return test_interleaving_gen_all(m, g, (1 + 1) * 2);
}

/*
 * Insert a single l0 entry, both l2 entries, one l1, then next l2 won't fit.
 */
static int test_interleaving_oversize_l0_l2_l1(struct unit_module *m,
		struct gk20a *g, void *args)
{
	return test_interleaving_gen_all(m, g, (1 + 2 + 1) * 2);
}

/*
 * Stop at the second l0 entry that doesn't fit.
 */
static int test_interleaving_oversize_l0_l2_l1_l2_l1_l2(struct unit_module *m,
		struct gk20a *g, void *args)
{
	return test_interleaving_gen_all(m, g, (1 + 2 + 1 + 2 + 1 + 2) * 2);
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
	UNIT_TEST(tsg_format, test_tsg_format, NULL),

	UNIT_TEST(flat, test_flat, NULL),

	UNIT_TEST(flat_oversize_tiny, test_flat_oversize_tiny, NULL),
	UNIT_TEST(flat_oversize_single, test_flat_oversize_single, NULL),
	UNIT_TEST(flat_oversize_onehalf, test_flat_oversize_onehalf, NULL),
	UNIT_TEST(flat_oversize_two, test_flat_oversize_two, NULL),
	UNIT_TEST(flat_oversize_end, test_flat_oversize_end, NULL),

	UNIT_TEST(interleaving, test_interleaving, NULL),

	UNIT_TEST(interleaving_oversize_tiny,
			test_interleaving_oversize_tiny, NULL),
	UNIT_TEST(interleaving_oversize_l0_l2,
		  test_interleaving_oversize_l0_l2, NULL),
	UNIT_TEST(interleaving_oversize_l0_l2_l1,
		  test_interleaving_oversize_l0_l2_l1, NULL),
	UNIT_TEST(interleaving_oversize_l0_l2_l1_l2_l1_l2,
		  test_interleaving_oversize_l0_l2_l1_l2_l1_l2, NULL),

	UNIT_TEST(interleaving_l0, test_interleaving_l0, NULL),
	UNIT_TEST(interleaving_l1, test_interleaving_l1, NULL),
	UNIT_TEST(interleaving_l2, test_interleaving_l2, NULL),
	UNIT_TEST(interleaving_l0_l1, test_interleaving_l0_l1, NULL),
	UNIT_TEST(interleaving_l1_l2, test_interleaving_l1_l2, NULL),
	UNIT_TEST(interleaving_l0_l2, test_interleaving_l0_l2, NULL),
};

UNIT_MODULE(nvgpu_runlist, nvgpu_runlist_tests, UNIT_PRIO_NVGPU_TEST);
