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

#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include <unit/io.h>
#include <unit/unit.h>

#include <nvgpu/channel.h>
#include <nvgpu/runlist.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/posix/posix-fault-injection.h>
#include <nvgpu/posix/dma.h>
#include <nvgpu/io.h>

#include "hal/fifo/runlist_ram_gk20a.h"
#include "hal/fifo/tsg_gk20a.h"
#include "nvgpu/hw/gk20a/hw_ram_gk20a.h"
#include "nvgpu-runlist.h"
#include "nvgpu/hw/gk20a/hw_fifo_gk20a.h"

#define RL_MAX_TIMESLICE_TIMEOUT ram_rl_entry_timeslice_timeout_v(U32_MAX)
#define RL_MAX_TIMESLICE_SCALE ram_rl_entry_timeslice_scale_v(U32_MAX)

#include "../nvgpu-fifo-common.h"

#ifdef RUNLIST_UNIT_DEBUG
#define unit_verbose	unit_info
#else
#define unit_verbose(unit, msg, ...) \
	do { \
		if (0) { \
			unit_info(unit, msg, ##__VA_ARGS__); \
		} \
	} while (0)
#endif

struct runlist_unit_ctx {
	u32 branches;
};

static struct runlist_unit_ctx unit_ctx;

#define MAX_STUB	4

struct stub_ctx {
	const char *name;
	u32 count;
	u32 chid;
	u32 tsgid;
};

struct stub_ctx stub[MAX_STUB];

static void subtest_setup(u32 branches)
{
	u32 i;

	unit_ctx.branches = branches;

	memset(stub, 0, sizeof(stub));
	for (i = 0; i < MAX_STUB; i++) {
		stub[i].name = "";
		stub[i].count = 0;
		stub[i].chid = NVGPU_INVALID_CHANNEL_ID;
		stub[i].tsgid = NVGPU_INVALID_TSG_ID;
	}
}

#define pruned test_fifo_subtest_pruned
#define branches_str test_fifo_flags_str

static u32 get_log2(u32 num)
{
	u32 res = 0;

	if (num == 0) {
		return 0;
	}
	while (num > 0) {
		res++;
		num >>= 1;
	}
	return res - 1U;
}

#define RL_MAX_TIMESLICE_TIMEOUT ram_rl_entry_timeslice_timeout_v(U32_MAX)
#define RL_MAX_TIMESLICE_SCALE ram_rl_entry_timeslice_scale_v(U32_MAX)

/*
 * This helper function mimics the non-FUSA gk20a_runlist_get_tsg_entry
 * function that has a simpler logic than other chips but is sufficient for
 * runlist test purposes.
 */
static void generic_runlist_get_tsg_entry(struct nvgpu_tsg *tsg,
	u32 *runlist, u32 timeslice)
{
	u32 timeout = timeslice;
	u32 scale = 0U;

	while (timeout > RL_MAX_TIMESLICE_TIMEOUT) {
		timeout >>= 1U;
		scale++;
	}

	if (scale > RL_MAX_TIMESLICE_SCALE) {
		timeout = RL_MAX_TIMESLICE_TIMEOUT;
		scale = RL_MAX_TIMESLICE_SCALE;
	}

	runlist[0] = ram_rl_entry_id_f(tsg->tsgid) |
			ram_rl_entry_type_tsg_f() |
			ram_rl_entry_tsg_length_f(tsg->num_active_channels) |
			ram_rl_entry_timeslice_scale_f(scale) |
			ram_rl_entry_timeslice_timeout_f(timeout);
	runlist[1] = 0;
}

/*
 * This helper function mimics the non-FUSA gk20a_runlist_get_ch_entry
 * function that has a simpler logic than other chips but is sufficient for
 * runlist test purposes.
 */
static void generic_runlist_get_ch_entry(struct nvgpu_channel *ch, u32 *runlist)
{
	runlist[0] = ram_rl_entry_chid_f(ch->chid);
	runlist[1] = 0;
}

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
	runlist->mem[0].aperture = APERTURE_SYSMEM;
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
	g->ops.runlist.get_tsg_entry = generic_runlist_get_tsg_entry;
	g->ops.runlist.get_ch_entry = generic_runlist_get_ch_entry;
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
		return -1;
	}
	if (memcmp(rl_data, expect_header, 2 * sizeof(u32)) != 0) {
		unit_err(m, "rl_data[0]=%08x", rl_data[0]);
		unit_err(m, "rl_data[1]=%08x", rl_data[1]);
		unit_err(m, "expect_header[0]=%08x", expect_header[0]);
		unit_err(m, "expect_header[1]=%08x", expect_header[1]);

		unit_err(m, "tsg header mismatch\n");
		return -1;
	}
	if (memcmp(rl_data + 2, expect_channel, 2 * n_ch * sizeof(u32)) != 0) {
		unit_err(m, "channel data mismatch\n");
		return -1;
	}

	return 0;
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
#define F_RUNLIST_FORMAT_FAIL_ENTRIES0		BIT(0)
#define F_RUNLIST_FORMAT_CH2			BIT(1)
#define F_RUNLIST_FORMAT_CH5			BIT(2)
#define F_RUNLIST_FORMAT_CH1_TIMESLICE		BIT(3)
#define F_RUNLIST_FORMAT_CH3_INACTIVE2		BIT(4)
#define F_RUNLIST_FORMAT_FAIL_ENTRY1		BIT(5)
#define F_RUNLIST_FORMAT_LAST			BIT(6)

static const char *f_runlist_format[] = {
	"priority_0_one_channel",
	"fail_zero_entries",
	"priority_1_two_channels",
	"priority_2_five_channels",
	"one_channel_nondefault_timeslice_timeout",
	"three_channels_with_two_inactives_in_the_middle",
	"fail_one_entry",
};

int test_tsg_format_gen(struct unit_module *m, struct gk20a *g, void *args)
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
	struct tsg_fmt_test_args *test_args;
	u32 branches = 0U;
	int ret = UNIT_FAIL;
	int err = 0;
	u32 fail = F_RUNLIST_FORMAT_FAIL_ENTRIES0 |
			F_RUNLIST_FORMAT_FAIL_ENTRY1;
	u32 prune = F_RUNLIST_FORMAT_CH2 | F_RUNLIST_FORMAT_CH5 |
			F_RUNLIST_FORMAT_CH1_TIMESLICE |
			F_RUNLIST_FORMAT_CH3_INACTIVE2 |
			fail;
	(void)test_args->timeslice;

	setup_fifo(g, &active_tsgs_map, &active_chs_map, tsgs, chs, 1, 5,
			&runlists, rl_data, false);

	for (branches = 0U; branches < F_RUNLIST_FORMAT_LAST;
		branches++) {
		if (pruned(branches, prune)) {
			unit_verbose(m, "%s branches=%u (pruned)\n", __func__,
				branches);
			continue;
		}
		unit_verbose(m, "%s branches=%u\n", __func__, branches);
		subtest_setup(branches);

		if (branches & fail) {
			test_args = &tsg_fmt_tests[0];
		} else {
			test_args = &tsg_fmt_tests[get_log2(branches)];
		}

		active_chs_map = test_args->chs_bitmap;

		if (test_args->timeslice == 0U) {
			tsgs[0].timeslice_us =
					g->ops.tsg.default_timeslice_us(g);
		} else {
			tsgs[0].timeslice_us = test_args->timeslice;
		}

		if (branches & fail) {
			err = run_format_test(m, f, &tsgs[0], chs,
				test_args->level, get_log2(branches)-1, rl_data,
				test_args->expect_header,
				test_args->expect_channel);
			unit_assert(err != 0, goto done);
		} else {
			err = run_format_test(m, f, &tsgs[0], chs,
				test_args->level, test_args->channels, rl_data,
				test_args->expect_header,
				test_args->expect_channel);
			unit_assert(err == 0, goto done);
		}

	}
	ret = UNIT_SUCCESS;

done:
	if (ret != UNIT_SUCCESS) {
		unit_err(m, "%s branches=%s\n", __func__,
		branches_str(branches, f_runlist_format));
	}

	return ret;
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
			unit_info(m,
				"limit %d, expected failure, got %u\n",
				sizelimit, n);
			return UNIT_FAIL;
		}
		/*
		 * Compare what we got; should be good up until the limit. For
		 * odd limit we miss the last u32 but it's better than nothing.
		 */
		return check_same_simple_tsgs(m, expected, rl_data,
				sizelimit / 2);
	}

	if (n != entries_in_list) {
		unit_info(m, "expected %u entries, got %u\n",
				entries_in_list, n);
		return UNIT_FAIL;
	}

	return check_same_simple_tsgs(m, expected, rl_data, tsgs_in_list);
}

#define F_RUNLIST_FLAT_GEN_OVERSIZE_TINY	BIT(0)
#define F_RUNLIST_FLAT_GEN_OVERSIZE_SINGLE	BIT(1)
#define F_RUNLIST_FLAT_GEN_OVERSIZE_ONEHALF	BIT(2)
#define F_RUNLIST_FLAT_GEN_OVERSIZE_TWO		BIT(3)
#define F_RUNLIST_FLAT_GEN_OVERSIZE_END		BIT(4)
#define F_RUNLIST_FLAT_GEN_LAST			BIT(5)

static const char *f_runlist_flat[] = {
	"runlist_flat_oversize_tiny",
	"runlist_flat_oversize_single",
	"runlist_flat_oversize_onehalf",
	"runlist_flat_oversize_two",
	"runlist_flat_oversize_end",
};

int test_flat_gen(struct unit_module *m, struct gk20a *g, void *args)
{
	u32 levels[] = {
		/* Some random-ish order of priority levels */
		0, 1, 2, 1, 0, 2,
	};
	u32 expected[] = {
		/* High (2) indices first, then medium (1), then low (0). */
		2, 5, 1, 3, 0, 4,
	};
	u32 sizelimits[] = {
		0, 1, 2, 3, 4, 11,
	};
	u32 branches = 0U;
	int ret = UNIT_FAIL;
	u32 prune = F_RUNLIST_FLAT_GEN_OVERSIZE_TINY |
			F_RUNLIST_FLAT_GEN_OVERSIZE_SINGLE |
			F_RUNLIST_FLAT_GEN_OVERSIZE_ONEHALF |
			F_RUNLIST_FLAT_GEN_OVERSIZE_TWO |
			F_RUNLIST_FLAT_GEN_OVERSIZE_END;

	for (branches = 0U; branches < F_RUNLIST_FLAT_GEN_LAST; branches++) {

		if (pruned(branches, prune)) {
			unit_verbose(m, "%s branches=%u (pruned)\n", __func__,
				branches);
			continue;
		}
		unit_verbose(m, "%s branches=%u\n", __func__, branches);
		subtest_setup(branches);

		ret = test_common_gen(m, g, false,
			sizelimits[get_log2(branches)],
			levels, ARRAY_SIZE(levels),
			expected, ARRAY_SIZE(expected));
		if (ret != UNIT_SUCCESS) {
			unit_err(m, "%s branches=%s\n", __func__,
			branches_str(branches, f_runlist_flat));
			break;
		}
	}

	return ret;
}

#define F_RUNLIST_INTERLEAVE_SINGLE_L0		BIT(0)
#define F_RUNLIST_INTERLEAVE_SINGLE_L1		BIT(1)
#define F_RUNLIST_INTERLEAVE_SINGLE_L2		BIT(2)
#define F_RUNLIST_INTERLEAVE_SINGLE_LAST	BIT(3)

static const char *f_runlist_interleave_single[] = {
	"only_L0_items",
	"only_L1_items",
	"only_L2_items",
};

static struct interleave_single_args {
	u32 n_levels;
	u32 levels[2];
	u32 n_expected;
	u32 expected[2];
} interleave_single_tests[] = {
	/* Only l0 items */
	{ 2, { 0, 0 }, 2, { 0, 1 } },
	/* Only l1 items */
	{ 2, { 1, 1 }, 2, { 0, 1 } },
	/* Only l2 items */
	{ 2, { 2, 2 }, 2, { 0, 1 } },
};

int test_interleave_single(struct unit_module *m, struct gk20a *g, void *args)
{
	struct interleave_single_args *single_args;
	u32 branches = 0U;
	int ret = UNIT_FAIL;
	u32 prune = F_RUNLIST_INTERLEAVE_SINGLE_L0 |
			F_RUNLIST_INTERLEAVE_SINGLE_L1 |
			F_RUNLIST_INTERLEAVE_SINGLE_L2;

	for (branches = 0U; branches < F_RUNLIST_INTERLEAVE_SINGLE_LAST;
		branches++) {

		if (pruned(branches, prune)) {
			unit_verbose(m, "%s branches=%u (pruned)\n", __func__,
				branches);
			continue;
		}
		unit_verbose(m, "%s branches=%u\n", __func__, branches);
		subtest_setup(branches);

		single_args = &interleave_single_tests[get_log2(branches)];

		ret = test_common_gen(m, g, true, 2 * single_args->n_expected,
			single_args->levels, single_args->n_levels,
			single_args->expected, single_args->n_expected);

		if (ret != UNIT_SUCCESS) {
			unit_err(m, "%s branches=%s\n", __func__,
			branches_str(branches, f_runlist_interleave_single));
			break;
		}
	}

	return ret;
}

#define F_RUNLIST_INTERLEAVE_DUAL_L0_L1		BIT(0)
#define F_RUNLIST_INTERLEAVE_DUAL_L1_L2		BIT(1)
#define F_RUNLIST_INTERLEAVE_DUAL_L0_L2		BIT(2)
#define F_RUNLIST_INTERLEAVE_DUAL_L0_L2_FAIL	BIT(3)
#define F_RUNLIST_INTERLEAVE_DUAL_LAST		BIT(4)

static const char *f_runlist_interleave_dual[] = {
	"only_L0_and_L1_items",
	"only_L1_and_L2_items",
	"only_L0_and_L2_items",
	"L0_and_L2_items_2_entries",
};

static struct interleave_dual_args {
	u32 n_levels;
	u32 levels[4];
	u32 n_expected;
	u32 expected[6];
} interleave_dual_tests[] = {
	/* Only low and medium priority items. */
	{ 4, { 0, 0, 1, 1 }, 6, { 2, 3, 0, 2, 3, 1 } },
	/* Only medium and high priority items. */
	{ 4, { 1, 1, 2, 2 }, 6, { 2, 3, 0, 2, 3, 1 } },
	/* Only low and high priority items. */
	{ 4, { 0, 0, 2, 2 }, 6, { 2, 3, 0, 2, 3, 1 } },
	/* Only low and high priority items. */
	{ 4, { 0, 0, 2, 2 }, 2, { 2, 3, 0, 2, 3, 1 } },
};

int test_interleave_dual(struct unit_module *m, struct gk20a *g, void *args)
{
	struct interleave_dual_args *dual_args;

	u32 branches = 0U;
	int ret = UNIT_FAIL;
	int err;
	u32 fail = F_RUNLIST_INTERLEAVE_DUAL_L0_L2_FAIL;
	u32 prune = F_RUNLIST_INTERLEAVE_DUAL_L0_L1 |
			F_RUNLIST_INTERLEAVE_DUAL_L1_L2 |
			F_RUNLIST_INTERLEAVE_DUAL_L0_L2 |
			fail;

	for (branches = 1U; branches < F_RUNLIST_INTERLEAVE_DUAL_LAST;
		branches++) {

		if (pruned(branches, prune)) {
			unit_verbose(m, "%s branches=%u (pruned)\n", __func__,
				branches);
			continue;
		}
		unit_verbose(m, "%s branches=%u\n", __func__, branches);
		subtest_setup(branches);

		dual_args = &interleave_dual_tests[get_log2(branches)];

		err = test_common_gen(m, g, true, 2 * dual_args->n_expected,
				dual_args->levels, dual_args->n_levels,
				dual_args->expected, dual_args->n_expected);

		if (branches & fail) {
			unit_assert(err != UNIT_SUCCESS, goto done);
		} else {
			unit_assert(err == UNIT_SUCCESS, goto done);
		}
	}

	ret = UNIT_SUCCESS;

done:
	if (ret != UNIT_SUCCESS) {
		unit_err(m, "%s branches=%s\n", __func__,
		branches_str(branches, f_runlist_interleave_dual));
	}

	return ret;
}

static struct interleave_level_test_args {
u32 sizelimit;
} interleave_level_tests[] = {
	/* All priority items. */
	{ 0 },
	/* Fail at level 2 immediately: space for just a tsg header,
	 * no ch entries.
	 */
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

#define F_RUNLIST_INTERLEAVE_LEVELS_ALL_PRIO    BIT(0)
#define F_RUNLIST_INTERLEAVE_LEVELS_FAIL_L2     BIT(1)
#define F_RUNLIST_INTERLEAVE_LEVELS_FAIL_L1     BIT(2)
#define F_RUNLIST_INTERLEAVE_LEVELS_FIT         BIT(3)
#define F_RUNLIST_INTERLEAVE_LEVELS_FAIL_L0     BIT(4)
#define F_RUNLIST_INTERLEAVE_LEVELS_LAST        BIT(5)

static const char *f_runlist_interleave_levels[] = {
	"interleaving",
	"interleaving_oversize_tiny",
	"interleaving_oversize_l2",
	"interleaving_oversize_l2_l1_l2",
	"interleaving_oversize_l2_l1_l2_l1",
	"interleaving_oversize_l2_l1_l2_l1_l2",
};

int test_interleaving_levels(struct unit_module *m, struct gk20a *g, void *args)
{
	u32 sizelimit;
	u32 l1 = 0, l2 = 1;
	u32 m1 = 2, m2 = 3;
	u32 h1 = 4, h2 = 5;
	u32 levels[] = { 0, 0, 1, 1, 2, 2 };
	u32 expected[] = {
		/* Order of channel ids; partly used also for oversize tests */
		h1, h2, m1, h1, h2, m2, h1, h2, l1,
		h1, h2, m1, h1, h2, m2, h1, h2, l2,
	};

	u32 branches = 1U;
	int ret = UNIT_FAIL;
	u32 prune = F_RUNLIST_INTERLEAVE_LEVELS_ALL_PRIO |
			F_RUNLIST_INTERLEAVE_LEVELS_FAIL_L2 |
			F_RUNLIST_INTERLEAVE_LEVELS_FAIL_L1 |
			F_RUNLIST_INTERLEAVE_LEVELS_FIT |
			F_RUNLIST_INTERLEAVE_LEVELS_FAIL_L0;

	for (branches = 0U; branches < F_RUNLIST_INTERLEAVE_LEVELS_LAST;
		branches++) {

		if (pruned(branches, prune)) {
			unit_verbose(m, "%s branches=%u (pruned)\n", __func__,
				branches);
			continue;
		}
		unit_verbose(m, "%s branches=%u\n", __func__, branches);
		subtest_setup(branches);

		sizelimit = interleave_level_tests[
						get_log2(branches)].sizelimit;

		ret = test_common_gen(m, g, true, sizelimit, levels,
			ARRAY_SIZE(levels), expected, ARRAY_SIZE(expected));

		if (ret != UNIT_SUCCESS) {
			unit_err(m, "%s branches=%s\n", __func__,
			branches_str(branches, f_runlist_interleave_levels));
			break;
		}
	}
	return ret;
}

#define F_RUNLIST_INTERLEAVE_LEVEL_LOW			BIT(0)
#define F_RUNLIST_INTERLEAVE_LEVEL_MEDIUM		BIT(1)
#define F_RUNLIST_INTERLEAVE_LEVEL_HIGH			BIT(2)
#define F_RUNLIST_INTERLEAVE_LEVEL_DEFAULT		BIT(3)
#define F_RUNLIST_INTERLEAVE_LEVEL_LAST			BIT(4)

static const char *f_runlist_interleave_level_name[] = {
	"LOW",
	"MEDIUM",
	"HIGH",
	"?"
};

int test_runlist_interleave_level_name(struct unit_module *m,
				struct gk20a *g, void *args)
{
	u32 branches = 0U;
	int ret = UNIT_FAIL;
	u32 fail = F_RUNLIST_INTERLEAVE_LEVEL_MEDIUM |
		F_RUNLIST_INTERLEAVE_LEVEL_HIGH |
		F_RUNLIST_INTERLEAVE_LEVEL_DEFAULT;
	u32 prune = fail;
	const char *interleave_level_name = NULL;

	for (branches = 0U; branches < F_RUNLIST_INTERLEAVE_LEVEL_LAST;
		branches++) {

		if (pruned(branches, prune)) {
			unit_verbose(m, "%s branches=%u (pruned)\n", __func__,
				branches);
			continue;
		}
		unit_verbose(m, "%s branches=%u\n", __func__, branches);
		subtest_setup(branches);

		interleave_level_name =
			nvgpu_runlist_interleave_level_name(get_log2(branches));
		unit_assert(strcmp(interleave_level_name,
			f_runlist_interleave_level_name[
					get_log2(branches)]) == 0, goto done);
	}

	ret = UNIT_SUCCESS;

done:
	if (ret != UNIT_SUCCESS) {
		unit_err(m, "%s failed\n", __func__);
	}

	return ret;
}

static void stub_runlist_write_state(struct gk20a *g, u32 runlists_mask,
					u32 runlist_state)
{
	stub[0].count = runlists_mask;
}

#define F_RUNLIST_SET_STATE_DISABLED		BIT(0)
#define F_RUNLIST_SET_STATE_ENABLED		BIT(1)
#define F_RUNLIST_SET_STATE_LAST		BIT(2)

static const char *f_runlist_set_state[] = {
	"set_state_disabled",
	"set_state_enabled",
};

int test_runlist_set_state(struct unit_module *m, struct gk20a *g, void *args)
{
	struct gpu_ops gops = g->ops;
	u32 branches = 0U;
	int ret = UNIT_FAIL;
	u32 fail = F_RUNLIST_SET_STATE_ENABLED | F_RUNLIST_SET_STATE_DISABLED;
	u32 prune = fail;

	g->ops.runlist.write_state = stub_runlist_write_state;

	for (branches = 1U; branches < F_RUNLIST_SET_STATE_LAST; branches++) {

		if (pruned(branches, prune)) {
			unit_verbose(m, "%s branches=%u (pruned)\n", __func__,
				branches);
			continue;
		}
		unit_verbose(m, "%s branches=%u\n", __func__, branches);
		subtest_setup(branches);

		if (branches & F_RUNLIST_SET_STATE_DISABLED) {
			nvgpu_runlist_set_state(g, 0U, RUNLIST_DISABLED);
			unit_assert(stub[0].count == 0U, goto done);
		} else {
			nvgpu_runlist_set_state(g, 1U, RUNLIST_ENABLED);
			unit_assert(stub[0].count == 1U, goto done);
		}
	}

	ret = UNIT_SUCCESS;

done:
	if (ret != UNIT_SUCCESS) {
		unit_err(m, "%s branches=%s\n", __func__,
			branches_str(branches, f_runlist_set_state));
	}

	g->ops = gops;
	return ret;
}

#define F_RUNLIST_LOCK_UNLOCK_ACTIVE_RUNLISTS_LAST		BIT(0)

int test_runlist_lock_unlock_active_runlists(struct unit_module *m,
				struct gk20a *g, void *args)
{
	int err = 0;
	u32 branches = 0U;
	int ret = UNIT_FAIL;
	u32 fail = 0U;
	u32 prune = fail;

	err = nvgpu_runlist_setup_sw(g);
	unit_assert(err == 0, goto done);

	for (branches = 0U;
		branches < F_RUNLIST_LOCK_UNLOCK_ACTIVE_RUNLISTS_LAST;
		branches++) {

		if (pruned(branches, prune)) {
			unit_verbose(m, "%s branches=%u (pruned)\n", __func__,
				branches);
			continue;
		}
		unit_verbose(m, "%s branches=%u\n", __func__, branches);
		subtest_setup(branches);

		nvgpu_runlist_lock_active_runlists(g);
		nvgpu_runlist_unlock_active_runlists(g);

		nvgpu_runlist_lock_active_runlists(g);
		nvgpu_runlist_unlock_runlists(g, 3U);

		nvgpu_runlist_unlock_runlists(g, 0U);
	}

	ret = UNIT_SUCCESS;

done:
	if (ret != UNIT_SUCCESS) {
		unit_err(m, "%s failed\n", __func__);
	}

	nvgpu_runlist_cleanup_sw(g);
	return ret;
}

#define F_RUNLIST_GET_MASK_ID_TYPE_KNOWN		BIT(0)
#define F_RUNLIST_GET_MASK_ID_TYPE_TSG			BIT(1)
#define F_RUNLIST_GET_MASK_ACT_ENG_BITMASK_NONZERO	BIT(2)
#define F_RUNLIST_GET_MASK_PBDMA_BITMASK_NONZERO	BIT(3)
#define F_RUNLIST_GET_MASK_LAST				BIT(4)

static const char *f_runlist_get_mask[] = {
	"ID_type_known",
	"act_eng_bitmask_nonzero",
	"pbdma_bitmask_nonzero",
};

int test_runlist_get_mask(struct unit_module *m, struct gk20a *g, void *args)
{
	unsigned int id_type = ID_TYPE_UNKNOWN;
	u32 act_eng_bitmask = 0U;
	u32 pbdma_bitmask = 0U;
	u32 ret_mask = 0U;
	int err = 0;
	u32 branches = 0U;
	int ret = UNIT_FAIL;
	u32 fail = 0U;
	u32 prune = fail;

	err = nvgpu_runlist_setup_sw(g);
	unit_assert(err == 0, goto done);

	for (branches = 0U; branches < F_RUNLIST_GET_MASK_LAST; branches++) {

		if (pruned(branches, prune)) {
			unit_verbose(m, "%s branches=%s (pruned)\n", __func__,
				branches_str(branches, f_runlist_get_mask));
			continue;
		}
		unit_verbose(m, "%s branches=%s\n", __func__,
			branches_str(branches, f_runlist_get_mask));
		subtest_setup(branches);

		id_type = (branches & F_RUNLIST_GET_MASK_ID_TYPE_KNOWN) ?
				((branches & F_RUNLIST_GET_MASK_ID_TYPE_TSG) ?
					ID_TYPE_TSG : ID_TYPE_CHANNEL) :
				ID_TYPE_UNKNOWN;

		act_eng_bitmask = (branches &
				F_RUNLIST_GET_MASK_ACT_ENG_BITMASK_NONZERO) ?
				1U : 0U;

		pbdma_bitmask = (branches &
				F_RUNLIST_GET_MASK_PBDMA_BITMASK_NONZERO) ?
				1U : 0U;

		ret_mask = nvgpu_runlist_get_runlists_mask(g, 0U, id_type,
			act_eng_bitmask, pbdma_bitmask);

	}

	if (branches == 0U) {
		unit_assert(ret_mask == 3U, goto done);
	} else {
		unit_assert(ret_mask == 1U, goto done);
	}

	ret = UNIT_SUCCESS;

done:
	if (ret != UNIT_SUCCESS) {
		unit_err(m, "%s branches=%s\n", __func__,
			branches_str(branches, f_runlist_get_mask));
	}

	nvgpu_runlist_cleanup_sw(g);
	return ret;
}

#define F_RUNLIST_SETUP_ALLOC_RUNLIST_INFO_FAIL		BIT(0)
#define F_RUNLIST_SETUP_ALLOC_ACTIVE_RUNLIST_INFO_FAIL	BIT(1)
#define F_RUNLIST_SETUP_ALLOC_ACTIVE_CHANNELS_FAIL	BIT(2)
#define F_RUNLIST_SETUP_ALLOC_ACTIVE_TSGS_FAIL		BIT(3)
#define F_RUNLIST_SETUP_ALLOC_DMA_FLAGS_SYS_FAIL	BIT(4)
#define F_RUNLIST_SETUP_GPU_IS_VIRTUAL			BIT(5)
#define F_RUNLIST_SETUP_LAST				BIT(6)

static const char *f_runlist_setup[] = {
	"alloc_runlist_info_fail",
	"alloc_active_runlist_info_fail",
	"alloc_active_channels_fail",
	"alloc_active_tsgs_fail",
	"alloc_dma_flags_sys_fail",
	"GPU_is_virtual"
};

int test_runlist_setup_sw(struct unit_module *m, struct gk20a *g, void *args)
{
	struct nvgpu_posix_fault_inj *kmem_fi;
	struct nvgpu_posix_fault_inj *dma_fi;
	u32 branches = 0U;
	int err = 0;
	int ret = UNIT_FAIL;
	u32 fail = F_RUNLIST_SETUP_ALLOC_RUNLIST_INFO_FAIL |
			F_RUNLIST_SETUP_ALLOC_ACTIVE_RUNLIST_INFO_FAIL |
			F_RUNLIST_SETUP_ALLOC_ACTIVE_CHANNELS_FAIL |
			F_RUNLIST_SETUP_ALLOC_ACTIVE_TSGS_FAIL |
			F_RUNLIST_SETUP_ALLOC_DMA_FLAGS_SYS_FAIL;
	u32 prune = fail;

	kmem_fi = nvgpu_kmem_get_fault_injection();
	dma_fi = nvgpu_dma_alloc_get_fault_injection();

	for (branches = 0U; branches < F_RUNLIST_SETUP_LAST; branches++) {

		if (pruned(branches, prune)) {
			unit_verbose(m, "%s branches=%s (pruned)\n", __func__,
				branches_str(branches, f_runlist_setup));
			continue;
		}
		unit_verbose(m, "%s branches=%s\n", __func__,
			branches_str(branches, f_runlist_setup));
		subtest_setup(branches);

		if ((branches >= F_RUNLIST_SETUP_ALLOC_RUNLIST_INFO_FAIL) &&
			(branches <= F_RUNLIST_SETUP_ALLOC_ACTIVE_TSGS_FAIL)) {
			nvgpu_posix_enable_fault_injection(kmem_fi,
				branches & fail ? true : false,
				get_log2(branches));
		}

		nvgpu_posix_enable_fault_injection(dma_fi,
			branches & F_RUNLIST_SETUP_ALLOC_DMA_FLAGS_SYS_FAIL ?
			true : false, 0);

		if (branches & F_RUNLIST_SETUP_GPU_IS_VIRTUAL) {
			g->is_virtual = true;
		}

		err = nvgpu_runlist_setup_sw(g);

		g->is_virtual = false;
		nvgpu_posix_enable_fault_injection(kmem_fi, false, 0);
		nvgpu_posix_enable_fault_injection(dma_fi, false, 0);

		if (branches & fail) {
			unit_assert(err != 0, goto done);
		} else {
			unit_assert(err == 0, goto done);
			nvgpu_runlist_cleanup_sw(g);
		}
	}

	ret = UNIT_SUCCESS;

done:
	if (ret != UNIT_SUCCESS) {
		unit_err(m, "%s branches=%s\n", __func__,
			branches_str(branches, f_runlist_setup));
	}

	return ret;
}

static int stub_runlist_wait_pending_timedout(struct gk20a *g, u32 runlist_id)
{
	return -ETIMEDOUT;
}

static int stub_runlist_wait_pending_interrupted(struct gk20a *g,
								u32 runlist_id)
{
	return -EINTR;
}

static int stub_runlist_wait_pending_success(struct gk20a *g, u32 runlist_id)
{
	return 0;
}

static void stub_runlist_hw_submit(struct gk20a *g, u32 runlist_id,
			u32 count, u32 buffer_index)
{
	return;
}

#define F_RUNLIST_RELOAD_IDS_GPU_NULL		BIT(0)
#define F_RUNLIST_RELOAD_IDS_NO_RUNLIST		BIT(1)
#define F_RUNLIST_RELOAD_IDS_WAIT_TIMEOUT	BIT(2)
#define F_RUNLIST_RELOAD_IDS_WAIT_INTERRUPT	BIT(3)
#define F_RUNLIST_RELOAD_IDS_REMOVE_CHANNELS	BIT(4)
#define F_RUNLIST_RELOAD_IDS_RESTORE_CHANNELS	BIT(5)
#define F_RUNLIST_RELOAD_IDS_LAST		BIT(6)

static const char *f_runlist_reload_ids[] = {
	"null_gpu_pointer",
	"no_runlist_selected",
	"runlist_wait_pending_timeout",
	"runlist_wait_pending_interrupted",
	"remove_active_channels_from_runlist",
	"restore_active_channels_from_runlist",
};

int test_runlist_reload_ids(struct unit_module *m, struct gk20a *g, void *args)
{
	struct gpu_ops gops = g->ops;
	bool add = false;
	u32 runlist_ids = 0U;
	u32 branches = 0U;
	int ret = UNIT_FAIL;
	int err = 0;
	u32 fail = F_RUNLIST_RELOAD_IDS_GPU_NULL |
			F_RUNLIST_RELOAD_IDS_WAIT_TIMEOUT |
			F_RUNLIST_RELOAD_IDS_WAIT_INTERRUPT;
	u32 prune = F_RUNLIST_RELOAD_IDS_NO_RUNLIST |
			F_RUNLIST_RELOAD_IDS_REMOVE_CHANNELS |
			F_RUNLIST_RELOAD_IDS_RESTORE_CHANNELS |
			fail;

	g->ops.runlist.hw_submit = stub_runlist_hw_submit;
	err = nvgpu_runlist_setup_sw(g);
	unit_assert(err == 0, goto done);

	for (branches = 1U; branches < F_RUNLIST_RELOAD_IDS_LAST;
		branches++) {

		if (pruned(branches, prune)) {
			unit_verbose(m, "%s branches=%u (pruned)\n", __func__,
				branches);
			continue;
		}
		unit_verbose(m, "%s branches=%u\n", __func__, branches);
		subtest_setup(branches);

		runlist_ids = (branches & F_RUNLIST_RELOAD_IDS_NO_RUNLIST) ?
				0U : 1U;
		add = (branches & F_RUNLIST_RELOAD_IDS_RESTORE_CHANNELS) ?
				true : false;

		if (branches & F_RUNLIST_RELOAD_IDS_WAIT_TIMEOUT) {
			g->ops.runlist.wait_pending =
					stub_runlist_wait_pending_timedout;
		} else if (branches & F_RUNLIST_RELOAD_IDS_WAIT_INTERRUPT) {
			g->ops.runlist.wait_pending =
					stub_runlist_wait_pending_interrupted;
		} else {
			g->ops.runlist.wait_pending =
					stub_runlist_wait_pending_success;
		}

		if (branches & F_RUNLIST_RELOAD_IDS_GPU_NULL) {
			err = nvgpu_runlist_reload_ids(NULL, runlist_ids, add);
		} else {
			err = nvgpu_runlist_reload_ids(g, runlist_ids, add);
		}

		if (branches & fail) {
			unit_assert(err != 0, goto done);
		} else {
			unit_assert(err == 0, goto done);
		}

	}
	ret = UNIT_SUCCESS;

done:
	if (ret != UNIT_SUCCESS) {
		unit_err(m, "%s branches=%s\n", __func__,
		branches_str(branches, f_runlist_reload_ids));
	}

	g->ops = gops;
	return ret;
}

static void stub_runlist_get_ch_entry(struct nvgpu_channel *ch, u32 *runlist)
{
}

#define F_RUNLIST_UPDATE_ADD				BIT(0)
#define F_RUNLIST_UPDATE_CH_NULL			BIT(1)
#define F_RUNLIST_UPDATE_CH_TSGID_INVALID		BIT(2)
#define F_RUNLIST_UPDATE_ADD_AGAIN			BIT(3)
#define F_RUNLIST_UPDATE_RECONSTRUCT_FAIL		BIT(4)
#define F_RUNLIST_UPDATE_REMOVE_ALL_CHANNELS		BIT(5)
#define F_RUNLIST_UPDATE_LAST				BIT(6)

static const char *f_runlist_update[] = {
	"add_ch",
	"add_ch_again",
	"update_null_ch",
	"update_ch_with_invalid_tsgid",
	"wait_for_finish",
	"update_reconstruct_fail",
	"remove_all_channels",
};

int test_runlist_update_locked(struct unit_module *m, struct gk20a *g,
			void *args)
{
	struct gpu_ops gops = g->ops;
	struct nvgpu_channel *ch = NULL;
	struct nvgpu_tsg *tsg = NULL;
	u32 num_runlist_entries_orig = 0U;
	u32 ch_tsgid_orig = 0U;
	bool add = false;
	u32 branches = 0U;
	int ret = UNIT_FAIL;
	int err = 0;
	u32 fail = F_RUNLIST_UPDATE_RECONSTRUCT_FAIL;
	u32 prune = F_RUNLIST_UPDATE_CH_TSGID_INVALID |
			F_RUNLIST_UPDATE_ADD_AGAIN |
			F_RUNLIST_UPDATE_REMOVE_ALL_CHANNELS | fail;

	g->ops.runlist.hw_submit = stub_runlist_hw_submit;
	g->ops.runlist.get_ch_entry = stub_runlist_get_ch_entry;
	num_runlist_entries_orig = g->fifo.num_runlist_entries;
	g->ptimer_src_freq = 31250000;

	tsg = nvgpu_tsg_open(g, getpid());
	unit_assert(tsg != NULL, goto done);

	ch = nvgpu_channel_open_new(g, NVGPU_INVALID_RUNLIST_ID, false,
			getpid(), getpid());
	unit_assert(ch != NULL, goto done);

	err = nvgpu_tsg_bind_channel(tsg, ch);
	unit_assert(err == 0, goto done);

	ch_tsgid_orig = ch->tsgid;

	for (branches = 0U; branches < F_RUNLIST_UPDATE_LAST;
		branches++) {

		if (pruned(branches, prune)) {
			unit_verbose(m, "%s branches=%u (pruned)\n", __func__,
				branches);
			continue;
		}
		unit_verbose(m, "%s branches=%u\n", __func__, branches);
		subtest_setup(branches);

		ch->tsgid = (branches & F_RUNLIST_UPDATE_CH_TSGID_INVALID) ?
				NVGPU_INVALID_TSG_ID : ch_tsgid_orig;

		add = branches & F_RUNLIST_UPDATE_ADD ? true : false;

		if (branches & F_RUNLIST_UPDATE_ADD_AGAIN) {
			err = nvgpu_runlist_update_locked(g,
							0U, ch, true, false);
			unit_assert(err == 0, goto done);

			add = true;
		}

		if (branches & F_RUNLIST_UPDATE_RECONSTRUCT_FAIL) {
			g->fifo.num_runlist_entries = 0U;
			/* force null ch and add = true to execute fail path */
			branches |= F_RUNLIST_UPDATE_CH_NULL;
			add = true;
		} else {
			g->fifo.num_runlist_entries = num_runlist_entries_orig;
		}

		if (branches & F_RUNLIST_UPDATE_REMOVE_ALL_CHANNELS) {
			/* Add additional channel to cover more branches */
			struct nvgpu_channel *chA = NULL;

			chA = nvgpu_channel_open_new(g,
						NVGPU_INVALID_RUNLIST_ID, false,
						getpid(), getpid());
			unit_assert(chA != NULL, goto done);

			err = nvgpu_tsg_bind_channel(tsg, chA);
			unit_assert(err == 0, goto done);

			err = nvgpu_runlist_update_locked(g,
							0U, chA, true, false);
			unit_assert(err == 0, goto done);

			err = nvgpu_runlist_update_locked(g,
							0U, chA, false, false);
			unit_assert(err == 0, goto done);

			err = nvgpu_tsg_unbind_channel(tsg, chA);
			if (err != 0) {
				unit_err(m, "Cannot unbind channel A\n");
			}
			if (chA != NULL) {
				nvgpu_channel_close(chA);
			}
		}

		if (branches & F_RUNLIST_UPDATE_CH_NULL) {
			err = nvgpu_runlist_update_locked(g,
							0U, NULL, add, false);
		} else {
			err = nvgpu_runlist_update_locked(g,
							0U, ch, add, false);
		}

		if (branches & fail) {
			unit_assert(err != 0, goto done);
		} else {
			unit_assert(err == 0, goto done);
		}
		ch->tsgid = ch_tsgid_orig;
	}

	ret = UNIT_SUCCESS;

done:
	if (ret != UNIT_SUCCESS) {
		unit_err(m, "%s branches %u=%s\n", __func__, branches,
		branches_str(branches, f_runlist_update));
	}

	err = nvgpu_tsg_unbind_channel(tsg, ch);
	if (err != 0) {
		unit_err(m, "Cannot unbind channel\n");
	}
	if (ch != NULL) {
		nvgpu_channel_close(ch);
	}
	if (tsg != NULL) {
		nvgpu_ref_put(&tsg->refcount, nvgpu_tsg_release);
	}
	g->ptimer_src_freq = 0;
	g->ops = gops;
	return ret;
}

int test_runlist_update_for_channel(struct unit_module *m, struct gk20a *g,
			void *args)
{
	struct gpu_ops gops = g->ops;
	struct nvgpu_channel *ch = NULL;
	struct nvgpu_tsg *tsg = NULL;
	int ret = UNIT_FAIL;
	int err = 0;

	g->ops.runlist.hw_submit = stub_runlist_hw_submit;
	g->ops.runlist.get_ch_entry = stub_runlist_get_ch_entry;
	g->ptimer_src_freq = 31250000;

	tsg = nvgpu_tsg_open(g, getpid());
	unit_assert(tsg != NULL, goto done);

	ch = nvgpu_channel_open_new(g, NVGPU_INVALID_RUNLIST_ID, false,
			getpid(), getpid());
	unit_assert(ch != NULL, goto done);

	err = nvgpu_tsg_bind_channel(tsg, ch);
	unit_assert(err == 0, goto done);

	err = nvgpu_runlist_update_for_channel(g, 0U, ch, false, false);
	unit_assert(err == 0, goto done);

	ret = UNIT_SUCCESS;

done:
	if (ret != UNIT_SUCCESS) {
		unit_err(m, "%s failed\n", __func__);
	}

	err = nvgpu_tsg_unbind_channel(tsg, ch);
	if (err != 0) {
		unit_err(m, "Cannot unbind channel\n");
	}
	if (ch != NULL) {
		nvgpu_channel_close(ch);
	}
	if (tsg != NULL) {
		nvgpu_ref_put(&tsg->refcount, nvgpu_tsg_release);
	}
	g->ptimer_src_freq = 0;
	g->ops = gops;
	return ret;
}

struct unit_module_test nvgpu_runlist_tests[] = {

	UNIT_TEST(init_support, test_fifo_init_support, &unit_ctx, 0),
	UNIT_TEST(setup_sw, test_runlist_setup_sw, NULL, 0),
	UNIT_TEST(get_mask, test_runlist_get_mask, NULL, 0),
	UNIT_TEST(lock_unlock_active_runlists, test_runlist_lock_unlock_active_runlists, NULL, 0),
	UNIT_TEST(set_state, test_runlist_set_state, NULL, 0),
	UNIT_TEST(interleave_level_name, test_runlist_interleave_level_name, NULL, 0),
	UNIT_TEST(reload_ids, test_runlist_reload_ids, NULL, 0),
	UNIT_TEST(runlist_update, test_runlist_update_locked, NULL, 0),
	UNIT_TEST(update_for_channel, test_runlist_update_for_channel, NULL, 0),
	UNIT_TEST(remove_support, test_fifo_remove_support, &unit_ctx, 0),
	UNIT_TEST(tsg_format_flat, test_tsg_format_gen, NULL, 0),
	UNIT_TEST(flat, test_flat_gen, NULL, 0),
	UNIT_TEST(interleave_single, test_interleave_single, NULL, 0),
	UNIT_TEST(interleave_dual, test_interleave_dual, NULL, 0),
	UNIT_TEST(interleave_level, test_interleaving_levels, NULL, 0),
};

UNIT_MODULE(nvgpu_runlist, nvgpu_runlist_tests, UNIT_PRIO_NVGPU_TEST);
