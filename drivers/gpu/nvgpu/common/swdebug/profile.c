/*
 * Copyright (c) 2020, NVIDIA CORPORATION.  All rights reserved.
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

#include <nvgpu/swprofile.h>
#include <nvgpu/lock.h>
#include <nvgpu/kref.h>
#include <nvgpu/debug.h>
#include <nvgpu/kmem.h>
#include <nvgpu/timers.h>
#include <nvgpu/sort.h>
#include <nvgpu/log.h>

/*
 * A simple profiler, capable of generating histograms.
 */

/*
 * The sample array is a 1d array comprised of repeating rows of data. To
 * index the array as though it were a row-major matrix, we need to do some
 * simple math.
 */
static inline u32 matrix_to_linear_index(struct nvgpu_swprofiler *p,
					 u32 row, u32 col)
{
	return (row * p->psample_len) + col;
}

void nvgpu_swprofile_initialize(struct gk20a *g,
				struct nvgpu_swprofiler *p,
				const char *col_names[])
{
	if (p->col_names != NULL) {
		/*
		 * Profiler is already initialized.
		 */
		return;
	}

	nvgpu_mutex_init(&p->lock);
	p->g = g;

	p->col_names = col_names;

	p->psample_len = 0U;
	while (col_names[p->psample_len] != NULL) {
		p->psample_len++;
	}
}

int nvgpu_swprofile_open(struct gk20a *g, struct nvgpu_swprofiler *p)
{
	int ret = 0;

	nvgpu_mutex_acquire(&p->lock);

	/*
	 * If this profiler is already opened, just take a ref and return.
	 */
	if (p->samples != NULL) {
		nvgpu_ref_get(&p->ref);
		goto done;
	}

	p->samples = nvgpu_vzalloc(g,
				   PROFILE_ENTRIES * p->psample_len *
				   sizeof(*p->samples));
	if (p->samples == NULL) {
		ret = -ENOMEM;
		goto done;
	}

	/*
	 * Otherwise allocate the necessary data structures, etc.
	 */
	nvgpu_ref_init(&p->ref);

done:
	nvgpu_mutex_release(&p->lock);
	return ret;
}

static void nvgpu_swprofile_free(struct nvgpu_ref *ref)
{
	struct nvgpu_swprofiler *p = container_of(ref, struct nvgpu_swprofiler, ref);

	nvgpu_vfree(p->g, p->samples);
	p->samples = NULL;
}

void nvgpu_swprofile_close(struct nvgpu_swprofiler *p)
{
	nvgpu_ref_put(&p->ref, nvgpu_swprofile_free);
}

/*
 * Note: this does _not_ lock the profiler. This is a conscious choice. If we
 * do lock the profiler then there's the possibility that you get bad data due
 * to the snapshot blocking on some other user printing the contents of the
 * profiler.
 *
 * Instead, this way, it's possible that someone printing the data in the
 * profiler gets a sample that's a mix of old and new. That's not great, but
 * IMO worse than a completely bogus sample.
 *
 * Also it's really quite unlikely for this race to happen in practice as the
 * print function is executed as a result of a debugfs call.
 */
void nvgpu_swprofile_snapshot(struct nvgpu_swprofiler *p, u32 idx)
{
	u32 index;

	/*
	 * Handle two cases: the first allows calling code to simply skip
	 * any profiling by passing in a NULL profiler; see the CDE code
	 * for this. The second case is if a profiler is not "opened".
	 */
	if (p == NULL || p->samples == NULL) {
		return;
	}

	/*
	 * p->sample_index is the current row, aka sample, we are writing to.
	 * idx is the column - i.e the sub-sample.
	 */
	index = matrix_to_linear_index(p, p->sample_index, idx);

	p->samples[index] = nvgpu_current_time_ns();
}

void nvgpu_swprofile_begin_sample(struct nvgpu_swprofiler *p)
{
	nvgpu_mutex_acquire(&p->lock);
	p->sample_index++;

	/* Handle wrap. */
	if (p->sample_index >= PROFILE_ENTRIES) {
		p->sample_index = 0U;
	}
	nvgpu_mutex_release(&p->lock);
}

static int profile_cmp(const void *a, const void *b)
{
	return *((const u64 *) a) - *((const u64 *) b);
}

#define PERCENTILE_WIDTH	5
#define PERCENTILE_RANGES	(100/PERCENTILE_WIDTH)

static u32 nvgpu_swprofile_build_ranges(struct nvgpu_swprofiler *p,
					u64 *storage,
					u64 *percentiles,
					u32 index_end,
					u32 index_start)
{
	u32 i;
	u32 nelem = 0U;

	/*
	 * Iterate through a column and build a temporary slice array of samples
	 * so that we can sort them without corrupting the current data.
	 *
	 * Note that we have to first convert the row/column indexes into linear
	 * indexes to access the underlying sample array.
	 */
	for (i = 0; i < PROFILE_ENTRIES; i++) {
		u32 linear_idx_start = matrix_to_linear_index(p, i, index_start);
		u32 linear_idx_end = matrix_to_linear_index(p, i, index_end);

		if (p->samples[linear_idx_end] <=
		    p->samples[linear_idx_start]) {
			/* This is an invalid element */
			continue;
		}

		storage[nelem] = p->samples[linear_idx_end] -
				 p->samples[linear_idx_start];
		nelem++;
	}

	/* sort it */
	sort(storage, nelem, sizeof(u64), profile_cmp, NULL);

	/* build ranges */
	for (i = 0; i < PERCENTILE_RANGES; i++) {
		percentiles[i] = nelem < PERCENTILE_RANGES ? 0 :
			storage[(PERCENTILE_WIDTH * (i + 1) * nelem)/100 - 1];
	}

	return nelem;
}

/*
 * Print a list of percentiles spaced by 5%. Note that the debug_context needs
 * to be special here. _Most_ print functions in NvGPU automatically add a new
 * line to the end of each print statement. This function _specifically_
 * requires that your debug print function does _NOT_ do this.
 */
void nvgpu_swprofile_print_ranges(struct gk20a *g,
				  struct nvgpu_swprofiler *p,
				  struct nvgpu_debug_context *o)
{
	u32 nelem = 0U, i, j;
	u64 *sorted_data = NULL;
	u64 *percentiles = NULL;

	nvgpu_mutex_acquire(&p->lock);

	if (p->samples == NULL) {
		gk20a_debug_output(o, "Profiler not enabled.\n");
		goto done;
	}

	sorted_data = nvgpu_vzalloc(g,
				    PROFILE_ENTRIES * p->psample_len *
				    sizeof(u64));
	percentiles = nvgpu_vzalloc(g,
				    PERCENTILE_RANGES * p->psample_len *
				    sizeof(u64));
	if (!sorted_data || !percentiles) {
		nvgpu_err(g, "vzalloc: OOM!");
		goto done;
	}

	/*
	 * Loop over each column; sort the column's data and then build
	 * percentile ranges based on that sorted data.
	 */
	for (i = 0U; i < p->psample_len; i++) {
		nelem = nvgpu_swprofile_build_ranges(p,
						   &sorted_data[i * PROFILE_ENTRIES],
						   &percentiles[i * PERCENTILE_RANGES],
						   i, 0U);
	}

	gk20a_debug_output(o, "Samples: %u\n", nelem);
	gk20a_debug_output(o, "%6s", "Perc");
	for (i = 0U; i < p->psample_len; i++) {
		gk20a_debug_output(o, " %15s", p->col_names[i]);
	}
	gk20a_debug_output(o, "\n");
	gk20a_debug_output(o, "%6s", "----");
	for (i = 0U; i < p->psample_len; i++) {
		gk20a_debug_output(o, " %15s", "---------------");
	}
	gk20a_debug_output(o, "\n");

	/*
	 * percentiles is another matrix, but this time it's using column major indexing.
	 */
	for (i = 0U; i < PERCENTILE_RANGES; i++) {
		gk20a_debug_output(o, "%3upc ", PERCENTILE_WIDTH * (i + 1));
		for (j = 0U; j < p->psample_len; j++) {
			gk20a_debug_output(o, " %15llu",
					   percentiles[(j * PERCENTILE_RANGES) + i]);
		}
		gk20a_debug_output(o, "\n");
	}
	gk20a_debug_output(o, "\n");

done:
	nvgpu_vfree(g, sorted_data);
	nvgpu_vfree(g, percentiles);
	nvgpu_mutex_release(&p->lock);
}

/*
 * Print raw data for the profiler. Can be useful if you want to do more sophisticated
 * analysis in python or something like that.
 *
 * Note this requires a debug context that does not automatically add newlines.
 */
void nvgpu_swprofile_print_raw_data(struct gk20a *g,
				    struct nvgpu_swprofiler *p,
				    struct nvgpu_debug_context *o)
{
	u32 i, j;

	nvgpu_mutex_acquire(&p->lock);

	if (p->samples == NULL) {
		gk20a_debug_output(o, "Profiler not enabled.\n");
		goto done;
	}

	gk20a_debug_output(o, "max samples: %u, sample len: %u\n",
			   PROFILE_ENTRIES, p->psample_len);

	for (i = 0U; i < p->psample_len; i++) {
		gk20a_debug_output(o, " %15s", p->col_names[i]);
	}
	gk20a_debug_output(o, "\n");

	for (i = 0U; i < PROFILE_ENTRIES; i++) {
		for (j = 0U; j < p->psample_len; j++) {
			u32 index = matrix_to_linear_index(p, i, j);

			gk20a_debug_output(o, " %15llu", p->samples[index]);
		}
		gk20a_debug_output(o, "\n");
	}

done:
	nvgpu_mutex_release(&p->lock);
}
