/*
 * Copyright (c) 2019, NVIDIA CORPORATION.  All rights reserved.
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

#include <stdlib.h> /* for abs() */
#include <unit/unit.h>
#include <unit/io.h>
#include <unit/unit-requirement-ids.h>

#include <nvgpu/atomic.h>
#include <nvgpu/bug.h>

struct atomic_struct {
	nvgpu_atomic_t atomic;
	nvgpu_atomic64_t atomic64;
};
enum atomic_width {
	WIDTH_32,
	WIDTH_64,
};
enum atomic_op {
	op_inc,
	op_dec,
	op_add,
	op_sub,
	op_inc_and_test,
	op_dec_and_test,
	op_sub_and_test,
	op_add_unless,
};
struct atomic_test_args {
	enum atomic_op op;
	enum atomic_width width;
	long start_val;
	long loop_count;
	long value; /* for add/sub ops */
};
struct atomic_thread_info {
	struct atomic_struct *atomic;
	struct atomic_test_args *margs;
	pthread_t thread;
	int thread_num;
	int iterations;
	long final_val;
	long unless;
};

/*
 * Define macros for atomic ops that have 32b and 64b versions so we can
 * keep the code cleaner.
 */
#define ATOMIC_SET(width, ref, i)					\
	((width == WIDTH_32) ?						\
		nvgpu_atomic_set(&((ref)->atomic), i) :			\
		nvgpu_atomic64_set(&((ref)->atomic64), i))

#define ATOMIC_READ(width, ref)						\
	((width == WIDTH_32) ?						\
		nvgpu_atomic_read(&((ref)->atomic)) :			\
		nvgpu_atomic64_read(&((ref)->atomic64)))

#define ATOMIC_INC(width, ref)						\
	((width == WIDTH_32) ?						\
		nvgpu_atomic_inc(&((ref)->atomic)) :			\
		nvgpu_atomic64_inc(&((ref)->atomic64)))

#define ATOMIC_INC_RETURN(width, ref)					\
	((width == WIDTH_32) ?						\
		nvgpu_atomic_inc_return(&((ref)->atomic)) :		\
		nvgpu_atomic64_inc_return(&((ref)->atomic64)))

#define ATOMIC_INC_AND_TEST(width, ref)					\
	((width == WIDTH_32) ?						\
		nvgpu_atomic_inc_and_test(&((ref)->atomic)) :		\
		nvgpu_atomic64_inc_and_test(&((ref)->atomic64)))

#define ATOMIC_DEC(width, ref)						\
	((width == WIDTH_32) ?						\
		nvgpu_atomic_dec(&((ref)->atomic)) :			\
		nvgpu_atomic64_dec(&((ref)->atomic64)))

#define ATOMIC_DEC_RETURN(width, ref)					\
	((width == WIDTH_32) ?						\
		nvgpu_atomic_dec_return(&((ref)->atomic)) :		\
		nvgpu_atomic64_dec_return(&((ref)->atomic64)))

#define ATOMIC_DEC_AND_TEST(width, ref)					\
	((width == WIDTH_32) ?						\
		nvgpu_atomic_dec_and_test(&((ref)->atomic)) :		\
		nvgpu_atomic64_dec_and_test(&((ref)->atomic64)))

#define ATOMIC_ADD(width, x, ref)					\
	((width == WIDTH_32) ?						\
		nvgpu_atomic_add(x, &((ref)->atomic)) :			\
		nvgpu_atomic64_add(x, &((ref)->atomic64)))

#define ATOMIC_ADD_RETURN(width, x, ref)				\
	((width == WIDTH_32) ?						\
		nvgpu_atomic_add_return(x, &((ref)->atomic)) :		\
		nvgpu_atomic64_add_return(x, &((ref)->atomic64)))

#define ATOMIC_ADD_UNLESS(width, ref, a, u)				\
	(((width == WIDTH_32) ?						\
		nvgpu_atomic_add_unless(&((ref)->atomic), a, u) :	\
		nvgpu_atomic64_add_unless(&((ref)->atomic64), a, u)))

#define ATOMIC_SUB(width, x, ref)					\
	((width == WIDTH_32) ?						\
		nvgpu_atomic_sub(x, &((ref)->atomic)) :			\
		nvgpu_atomic64_sub(x, &((ref)->atomic64)))

#define ATOMIC_SUB_RETURN(width, x, ref)				\
	((width == WIDTH_32) ?						\
		nvgpu_atomic_sub_return(x, &((ref)->atomic)) :		\
		nvgpu_atomic64_sub_return(x, &((ref)->atomic64)))

#define ATOMIC_SUB_AND_TEST(width, x, ref)				\
	((width == WIDTH_32) ?						\
		nvgpu_atomic_sub_and_test(x, &((ref)->atomic)) :	\
		nvgpu_atomic64_sub_and_test(x, &((ref)->atomic64)))

#define ATOMIC_XCHG(width, ref, new)				\
	((width == WIDTH_32) ?						\
		nvgpu_atomic_xchg(&((ref)->atomic), new) :	\
		nvgpu_atomic64_xchg(&((ref)->atomic64), new))

#define ATOMIC_CMPXCHG(width, ref, old, new)				\
	((width == WIDTH_32) ?						\
		nvgpu_atomic_cmpxchg(&((ref)->atomic), old, new) :	\
		nvgpu_atomic64_cmpxchg(&((ref)->atomic64), old, new))

/*
 * Helper macro that takes an atomic op from the enum and returns +1/-1
 * to help doing arithemtic.
 */
#define ATOMIC_OP_SIGN(atomic_op)					\
	({								\
		long sign;						\
		switch (atomic_op) {					\
			case op_dec:					\
			case op_sub:					\
			case op_dec_and_test:				\
			case op_sub_and_test:				\
				sign = -1;				\
				break;					\
			default:					\
				sign = 1;				\
		}							\
		sign;							\
	})

/* Support function to do an atomic set and read verification */
static int single_set_and_read(struct unit_module *m,
			       struct atomic_struct *atomic,
			       enum atomic_width width, const long set_val)
{
	long read_val;

	if ((width == WIDTH_32) &&
	    ((set_val < INT_MIN) || (set_val > INT_MAX))) {
		unit_return_fail(m, "Invalid value for 32 op\n");
	}

	ATOMIC_SET(width, atomic, set_val);
	read_val = ATOMIC_READ(width, atomic);
	if (read_val != set_val) {
		unit_err(m, "Atomic returned wrong value. Expected: %ld "
			    "Received: %ld\n", (long)set_val, (long)read_val);
		return UNIT_FAIL;
	}
	return  UNIT_SUCCESS;
}

/*
 * Test atomic read and set operations single threaded for proper functionality
 *
 * Tests setting the limit values for each size.
 * Loops through setting each bit in a 32/64bit value.
 */
static int test_atomic_set_and_read(struct unit_module *m,
				    struct gk20a *g, void *__args)
{
	struct atomic_test_args *args = (struct atomic_test_args *)__args;
	const int loop_limit = args->width == WIDTH_32 ? (sizeof(int) * 8) :
							 (sizeof(long) * 8);
	const long min_value = args->width == WIDTH_32 ? INT_MIN :
							 LONG_MIN;
	const long max_value = args->width == WIDTH_32 ? INT_MAX :
							 LONG_MAX;
	struct atomic_struct atomic;
	int i;

	single_set_and_read(m, &atomic, args->width, min_value);
	single_set_and_read(m, &atomic, args->width, max_value);
	single_set_and_read(m, &atomic, args->width, 0);

	for (i = 0; i < loop_limit; i++) {
		if (single_set_and_read(m, &atomic, args->width, (1 << i))
							!= UNIT_SUCCESS) {
			return UNIT_FAIL;
		}
	}

	return UNIT_SUCCESS;
}

/*
 * Test arithmetic atomic operations single threaded for proper functionality
 *   inc, dec, add, sub and friends (except add_unless)
 * Sets a start value from args
 * Loops (iterations per args param)
 * Validates final result
 *
 * For *_and_test ops, the args should make sure the loop traverses across 0
 * to test the "test" part.
 */
static int test_atomic_arithmetic(struct unit_module *m,
			       struct gk20a *g, void *__args)
{
	struct atomic_test_args *args = (struct atomic_test_args *)__args;
	struct atomic_struct atomic;
	int i;
	long delta_magnitude;
	long read_val;
	long expected_val;
	bool result_bool;
	bool check_result_bool = false;

	if (single_set_and_read(m, &atomic, args->width, args->start_val)
							!= UNIT_SUCCESS) {
		return UNIT_FAIL;
	}

	for (i = 1; i <= args->loop_count; i++) {
		if (args->op == op_inc) {
			/* use 2 since we test both inc and inc_return */
			delta_magnitude = 2;
			ATOMIC_INC(args->width, &atomic);
			read_val = ATOMIC_INC_RETURN(args->width, &atomic);
		} else if (args->op == op_inc_and_test) {
			delta_magnitude = 1;
			check_result_bool = true;
			result_bool = ATOMIC_INC_AND_TEST(args->width, &atomic);
			read_val = ATOMIC_READ(args->width, &atomic);
		} else if (args->op == op_dec) {
			/* use 2 since we test both dec and dec_return */
			delta_magnitude = 2;
			ATOMIC_DEC(args->width, &atomic);
			read_val = ATOMIC_DEC_RETURN(args->width, &atomic);
		} else if (args->op == op_dec_and_test) {
			delta_magnitude = 1;
			check_result_bool = true;
			result_bool = ATOMIC_DEC_AND_TEST(args->width, &atomic);
			read_val = ATOMIC_READ(args->width, &atomic);
		} else if (args->op == op_add) {
			delta_magnitude = args->value * 2;
			ATOMIC_ADD(args->width, args->value, &atomic);
			read_val = ATOMIC_ADD_RETURN(args->width, args->value,
							&atomic);
		} else if (args->op == op_sub) {
			delta_magnitude = args->value * 2;
			ATOMIC_SUB(args->width, args->value, &atomic);
			read_val = ATOMIC_SUB_RETURN(args->width, args->value,
							&atomic);
		} else if (args->op == op_sub_and_test) {
			delta_magnitude = args->value;
			check_result_bool = true;
			result_bool = ATOMIC_SUB_AND_TEST(args->width,
						args->value, &atomic);
			read_val = ATOMIC_READ(args->width, &atomic);
		} else {
			unit_return_fail(m, "Test error: invalid op in %s\n",
					__func__);
		}

		expected_val = args->start_val +
			(i * delta_magnitude * ATOMIC_OP_SIGN(args->op));

		/* sanity check */
		if ((args->width == WIDTH_32) &&
		    ((expected_val > INT_MAX) || (expected_val < INT_MIN))) {
			unit_return_fail(m, "Test error: invalid value in %s\n",
					__func__);
		}

		if (read_val != expected_val) {
			unit_return_fail(m, "Atomic returned wrong value. "
				    "Expected: %ld Received: %ld\n",
				    (long)expected_val, (long)read_val);
		}

		if (check_result_bool) {
			if (((expected_val == 0) && !result_bool) ||
			    ((expected_val != 0) && result_bool)) {
				    unit_return_fail(m,
						"Test result incorrect\n");
			    }
		}
	}

	return UNIT_SUCCESS;
}

/*
 * Support function that runs in the threads for the arithmetic threaded
 * test below
 */
static void *arithmetic_thread(void *__args)
{
	struct atomic_thread_info *targs = (struct atomic_thread_info *)__args;
	int i;

	for (i = 0; i < targs->margs->loop_count; i++) {
		if (targs->margs->op == op_inc) {
			ATOMIC_INC(targs->margs->width, targs->atomic);
		} else if (targs->margs->op == op_dec) {
			ATOMIC_DEC(targs->margs->width, targs->atomic);
		} else if (targs->margs->op == op_add) {
			/*
			 * Save the last value to sanity that threads aren't
			 * running sequentially
			 */
			targs->final_val = ATOMIC_ADD_RETURN(
						targs->margs->width,
						targs->margs->value,
						targs->atomic);
		} else if (targs->margs->op == op_add) {
			ATOMIC_ADD(targs->margs->width, targs->margs->value,
					targs->atomic);
		} else if (targs->margs->op == op_sub) {
			ATOMIC_SUB(targs->margs->width, targs->margs->value,
					targs->atomic);
		} else if (targs->margs->op == op_inc_and_test) {
			if (ATOMIC_INC_AND_TEST(targs->margs->width,
							targs->atomic)) {
				/*
				 * Only increment if atomic op returns true
				 * (that the value is 0)
				 */
				targs->iterations++;
			}
		} else if (targs->margs->op == op_dec_and_test) {
			if (ATOMIC_DEC_AND_TEST(targs->margs->width,
							targs->atomic)) {
				/*
				 * Only increment if atomic op returns true
				 * (that the value is 0)
				 */
				targs->iterations++;
			}
		} else if (targs->margs->op == op_sub_and_test) {
			if (ATOMIC_SUB_AND_TEST(targs->margs->width,
						targs->margs->value,
						targs->atomic)) {
				/*
				 * Only increment if atomic op returns true
				 * (that the value is 0)
				 */
				targs->iterations++;
			}
		} else if (targs->margs->op == op_add_unless) {
			if (ATOMIC_ADD_UNLESS(targs->margs->width,
					targs->atomic, targs->margs->value,
					targs->unless) != targs->unless) {
				/*
				 * Increment until the atomic value is the
				 * "unless" value.
				 */
				targs->iterations++;
			}
		} else {
			/*
			 * Don't print an error here because it would print
			 * for each thread. The main thread will catch this.
			 */
			break;
		}
	}

	return NULL;
}

/*
 * Support function to make sure the threaded arithmetic tests ran the correct
 * number of iterations across threads, if applicable.
 */
static bool correct_thread_iteration_count(struct unit_module *m,
					   struct atomic_thread_info *threads,
					   int num_threads,
					   long expected_iterations)
{
	int i;
	long total_iterations = 0;

	for (i = 0; i < num_threads; i++) {
		total_iterations += threads[i].iterations;
	}

	if (total_iterations != expected_iterations) {
		unit_err(m, "threaded test op took wrong number of iterations "
			 "expected %ld took: %ld\n",
			 expected_iterations, total_iterations);
		return false;
	}

	return true;
}

/*
 * Test arithmetic operations in threads to verify atomicity.
 *
 * Sets initial start value
 * Kicks off threads to loop running ops
 * When threads finish loops, verify values
 *
 * With the ops that have a return, save the final value for each thread and
 * use that to try to ensure that the threads aren't executing sequentially.
 */
static int test_atomic_arithmetic_threaded(struct unit_module *m,
					struct gk20a *g, void *__args)
{
	struct atomic_test_args *args = (struct atomic_test_args *)__args;
	struct atomic_struct atomic;
	const int num_threads = 100;
	struct atomic_thread_info threads[num_threads];
	int i;
	long expected_val, val, expected_iterations;

	if (single_set_and_read(m, &atomic, args->width, args->start_val)
							!= UNIT_SUCCESS) {
		return UNIT_FAIL;
	}

	/* setup threads */
	for (i = 0; i < num_threads; i++) {
		threads[i].atomic = &atomic;
		threads[i].margs = args;
		threads[i].thread_num = i;
		threads[i].iterations = 0;
		/* For add_unless, add until we hit half the iterations */
		threads[i].unless = args->start_val +
				(num_threads * args->loop_count / 2);
	}
	/*
	 * start threads - This is done separately to try to increase
	 * parallelism of the threads by starting them as closely together
	 * as possible. It is also done in reverse to avoid compiler
	 * optimization.
	 */
	for (i = (num_threads - 1); i >= 0; i--) {
		pthread_create(&threads[i].thread, NULL, arithmetic_thread,
				&threads[i]);
	}

	/* wait for all threads to complete */
	for (i = 0; i < num_threads; i++) {
		pthread_join(threads[i].thread, NULL);
	}

	val = ATOMIC_READ(args->width, &atomic);

	switch (args->op) {
		case op_add_unless:
			/*
			 * For add_unless, the threads increment their iteration
			 * counts until the atomic reaches the unless value,
			 * but continue calling the op in the loop to make sure
			 * it doesn't actually add anymore.
			 */
			expected_iterations = (threads[0].unless -
						args->start_val + 1) /
						args->value;
			if (!correct_thread_iteration_count(m, threads,
					num_threads, expected_iterations)) {
				return  UNIT_FAIL;
			}
			expected_val = threads[0].unless;
			break;

		case op_inc_and_test:
		case op_dec_and_test:
		case op_sub_and_test:
			/*
			 * The threads only increment when the atomic op
			 * reports that it hit 0 which should only happen once.
			 */
			if (!correct_thread_iteration_count(m, threads,
				num_threads, 1)) {
				return  UNIT_FAIL;
			}
			/* fall through! */

		case op_add:
		case op_sub:
		case op_inc:
		case op_dec:
			expected_val = args->start_val +
				(args->loop_count * num_threads *
					ATOMIC_OP_SIGN(args->op) * args->value);
			break;

		default:
			unit_return_fail(m, "Test error: invalid op in %s\n",
					__func__);

	}

	/* sanity check */
	if ((args->width == WIDTH_32) &&
	    ((expected_val > INT_MAX) || (expected_val < INT_MIN))) {
		unit_return_fail(m, "Test error: invalid value in %s\n",
				__func__);
	}

	if (val != expected_val) {
		unit_return_fail(m, "threaded value incorrect "
				"expected: %ld result: %ld\n",
				expected_val, val);
	}

	if (args->op == op_add) {
		/* sanity test that the threads aren't all sequential */
		bool sequential = true;
		for (i = 0; i < (num_threads - 1); i++) {
			if (abs(threads[i].final_val - threads[i+1].final_val)
							!= args->loop_count) {
				sequential = false;
				break;
			}
		}
		if (sequential) {
			unit_return_fail(m, "threads appear to have run "
					 "sequentially!\n");
		}
	}

	return UNIT_SUCCESS;
}

/*
 * Test xchg op single threaded for proper functionality
 *
 * Loops calling xchg op with different values making sure the returned
 * value is the last one written.
 */
static int test_atomic_xchg(struct unit_module *m,
			    struct gk20a *g, void *__args)
{
	struct atomic_test_args *args = (struct atomic_test_args *)__args;
	struct atomic_struct atomic;
	int i;
	long new_val, old_val, ret_val;

	if (single_set_and_read(m, &atomic, args->width, args->start_val)
							!= UNIT_SUCCESS) {
		return UNIT_FAIL;
	}

	old_val = args->start_val;
	for (i = 0; i < args->loop_count; i++) {
		/*
		 * alternate positive and negative values while increasing
		 * based on the loop counter
		 */
		new_val = (i % 2 ? 1 : -1) * (args->start_val + i);
		/* only a 32bit xchg op */
		ret_val = ATOMIC_XCHG(args->width, &atomic, new_val);
		if (ret_val != old_val) {
			unit_return_fail(m, "xchg returned bad old val "
					"Expected: %ld, Received: %ld\n",
					old_val, ret_val);
		}
		old_val = new_val;
	}

	return UNIT_SUCCESS;
}

/*
 * Test cmpxchg single threaded for proper functionality
 *
 * Loop calling cmpxchg. Alternating between matching and not matching.
 * Verify correct behavior for each call.
 */
static int test_atomic_cmpxchg(struct unit_module *m,
			       struct gk20a *g, void *__args)
{
	struct atomic_test_args *args = (struct atomic_test_args *)__args;
	struct atomic_struct atomic;
	const int switch_interval = 5;
	int i;
	long new_val, old_val, ret_val;
	bool should_match = true;

	if (single_set_and_read(m, &atomic, args->width, args->start_val)
							!= UNIT_SUCCESS) {
		return UNIT_FAIL;
	}

	old_val = args->start_val;
	for (i = 0; i < args->loop_count; i++) {
		/*
		 * alternate whether the cmp should match each
		 * switch_interval
		 */
		if ((i % switch_interval) == 0) {
			should_match = !should_match;
		}

		new_val = args->start_val + i;
		if (should_match) {
			ret_val = ATOMIC_CMPXCHG(args->width, &atomic,
						old_val, new_val);
			if (ret_val != old_val) {
				unit_return_fail(m,
					"cmpxchg returned bad old val "
					"Expected: %ld, Received: %ld\n",
					old_val, ret_val);
			}
			ret_val = ATOMIC_READ(args->width, &atomic);
			if (ret_val != new_val) {
				unit_return_fail(m,
					"cmpxchg did not update "
					"Expected: %ld, Received: %ld\n",
					new_val, ret_val);
			}
			old_val = new_val;
		} else {
			ret_val = ATOMIC_CMPXCHG(args->width, &atomic,
						-1 * old_val, new_val);
			if (ret_val != old_val) {
				unit_return_fail(m,
					"cmpxchg returned bad old val "
					"Expected: %ld, Received: %ld\n",
					old_val, ret_val);
			}
			ret_val = ATOMIC_READ(args->width, &atomic);
			if (ret_val != old_val) {
				unit_return_fail(m,
					"cmpxchg should not have updated "
					"Expected: %ld, Received: %ld\n",
					old_val, ret_val);
			}
		}
	}

	return UNIT_SUCCESS;
}

/*
 * Test add_unless op single threaded for proper functionality
 *
 * Note: there is only a 32-bit operation
 *
 * Loop through calling the operation. Alternating whether the add should
 * occur or not (i.e. changing the "unless" value).
 * Verify correct behavior for each operation.
 */
static int test_atomic_add_unless(struct unit_module *m,
				  struct gk20a *g, void *__args)
{
	struct atomic_test_args *args = (struct atomic_test_args *)__args;
	struct atomic_struct atomic;
	const int switch_interval = 5;
	int i;
	int new_val, old_val, ret_val;
	bool should_update = true;

	if (single_set_and_read(m, &atomic, args->width, args->start_val)
							!= UNIT_SUCCESS) {
		return UNIT_FAIL;
	}
	old_val = args->start_val;
	for (i = 0; i < args->loop_count; i++) {
		/* alternate whether add should occur every switch_interval */
		if ((i % switch_interval) == 0) {
			should_update = !should_update;
		}

		if (should_update) {
			/* This will fail to match and do the add */
			ret_val = ATOMIC_ADD_UNLESS(args->width, &atomic,
						args->value, old_val - 1);
			if (ret_val != old_val) {
				unit_return_fail(m,
					"add_unless returned bad old val "
					"Expected: %d, Received: %d\n",
					old_val, ret_val);
			}
			new_val = old_val + args->value;
			ret_val = ATOMIC_READ(args->width, &atomic);
			if (ret_val != new_val) {
				unit_return_fail(m, "add_unless did not "
						"update Expected: %d, "
						"Received: %d\n",
						new_val, ret_val);
			}
			old_val = ret_val;
		} else {
			/* This will match the old value and won't add */
			ret_val = ATOMIC_ADD_UNLESS(args->width, &atomic,
						args->value, old_val);
			if (ret_val != old_val) {
				unit_return_fail(m,
					"add_unless returned bad old val "
					"Expected: %d, Received: %d\n",
					old_val, ret_val);
			}
			ret_val = ATOMIC_READ(args->width, &atomic);
			if (ret_val != old_val) {
				unit_return_fail(m, "add_unless should not "
						"have updated Expected: %d, "
						"Received: %d\n",
						old_val, ret_val);
			}
		}
	}

	return UNIT_SUCCESS;
}

static struct atomic_test_args set_and_read_32_arg = {
	.width = WIDTH_32,
};
static struct atomic_test_args set_and_read_64_arg = {
	.width = WIDTH_64,
};
static struct atomic_test_args inc_32_arg = {
	.op = op_inc,
	.width = WIDTH_32,
	.start_val = -500,
	.loop_count = 10000,
	.value = 1,
};
static struct atomic_test_args inc_and_test_32_arg = {
	/* must cross 0 */
	.op = op_inc_and_test,
	.width = WIDTH_32,
	.start_val = -500,
	.loop_count = 10000,
	.value = 1,
};
static struct atomic_test_args inc_and_test_64_arg = {
	/* must cross 0 */
	.op = op_inc_and_test,
	.width = WIDTH_64,
	.start_val = -500,
	.loop_count = 10000,
	.value = 1,
};
static struct atomic_test_args inc_64_arg = {
	.op = op_inc,
	.width = WIDTH_64,
	.start_val = INT_MAX - 500,
	.loop_count = 10000,
	.value = 1,
};
static struct atomic_test_args dec_32_arg = {
	.op = op_dec,
	.width = WIDTH_32,
	.start_val = 500,
	.loop_count = 10000,
	.value = 1,
};
static struct atomic_test_args dec_and_test_32_arg = {
	/* must cross 0 */
	.op = op_dec_and_test,
	.width = WIDTH_32,
	.start_val = 500,
	.loop_count = 10000,
	.value = 1,
};
static struct atomic_test_args dec_and_test_64_arg = {
	/* must cross 0 */
	.op = op_dec_and_test,
	.width = WIDTH_64,
	.start_val = 500,
	.loop_count = 10000,
	.value = 1,
};
static struct atomic_test_args dec_64_arg = {
	.op = op_dec,
	.width = WIDTH_64,
	.start_val = INT_MIN + 500,
	.loop_count = 10000,
	.value = 1,
};
static struct atomic_test_args add_32_arg = {
	.op = op_add,
	.width = WIDTH_32,
	.start_val = -500,
	.loop_count = 10000,
	.value = 7,
};
static struct atomic_test_args add_64_arg = {
	.op = op_add,
	.width = WIDTH_64,
	.start_val = INT_MAX - 500,
	.loop_count = 10000,
	.value = 7,
};
struct atomic_test_args sub_32_arg = {
	.op = op_sub,
	.width = WIDTH_32,
	.start_val = 500,
	.loop_count = 10000,
	.value = 7,
};
static struct atomic_test_args sub_64_arg = {
	.op = op_sub,
	.width = WIDTH_64,
	.start_val = INT_MIN + 500,
	.loop_count = 10000,
	.value = 7,
};
static struct atomic_test_args sub_and_test_32_arg = {
	/* must cross 0 */
	.op = op_sub_and_test,
	.width = WIDTH_32,
	.start_val = 500,
	.loop_count = 10000,
	.value = 5,
};
static struct atomic_test_args sub_and_test_64_arg = {
	/* must cross 0 */
	.op = op_sub_and_test,
	.width = WIDTH_64,
	.start_val = 500,
	.loop_count = 10000,
	.value = 5,
};
struct atomic_test_args xchg_32_arg = {
	.width = WIDTH_32,
	.start_val = 1,
	.loop_count = 10000,
};
struct atomic_test_args xchg_64_arg = {
	.width = WIDTH_64,
	.start_val = INT_MAX,
	.loop_count = 10000,
};
static struct atomic_test_args add_unless_32_arg = {
	/* must loop at least 10 times */
	.op = op_add_unless,
	.width = WIDTH_32,
	.start_val = -500,
	.loop_count = 10000,
	.value = 5,
};
static struct atomic_test_args add_unless_64_arg = {
	/* must loop at least 10 times */
	.op = op_add_unless,
	.width = WIDTH_64,
	.start_val = -500,
	.loop_count = 10000,
	.value = 5,
};

struct unit_module_test atomic_tests[] = {
	UNIT_TEST(atomic_set_and_read_32,		test_atomic_set_and_read,		&set_and_read_32_arg),
	UNIT_TEST(atomic_set_and_read_64,		test_atomic_set_and_read,		&set_and_read_64_arg),
	UNIT_TEST(atomic_inc_32,			test_atomic_arithmetic,			&inc_32_arg),
	UNIT_TEST(atomic_inc_and_test_32,		test_atomic_arithmetic,			&inc_and_test_32_arg),
	UNIT_TEST(atomic_inc_and_test_64,		test_atomic_arithmetic,			&inc_and_test_64_arg),
	UNIT_TEST(atomic_inc_64,			test_atomic_arithmetic,			&inc_64_arg),
	UNIT_TEST(atomic_dec_32,			test_atomic_arithmetic,			&dec_32_arg),
	UNIT_TEST(atomic_dec_64,			test_atomic_arithmetic,			&dec_64_arg),
	UNIT_TEST(atomic_dec_and_test_32,		test_atomic_arithmetic,			&dec_and_test_32_arg),
	UNIT_TEST(atomic_dec_and_test_64,		test_atomic_arithmetic,			&dec_and_test_64_arg),
	UNIT_TEST(atomic_add_32,			test_atomic_arithmetic,			&add_32_arg),
	UNIT_TEST(atomic_add_64,			test_atomic_arithmetic,			&add_64_arg),
	UNIT_TEST(atomic_sub_32,			test_atomic_arithmetic,			&sub_32_arg),
	UNIT_TEST(atomic_sub_64,			test_atomic_arithmetic,			&sub_64_arg),
	UNIT_TEST(atomic_sub_and_test_32,		test_atomic_arithmetic,			&sub_and_test_32_arg),
	UNIT_TEST(atomic_sub_and_test_64,		test_atomic_arithmetic,			&sub_and_test_64_arg),
	UNIT_TEST(atomic_xchg_32,			test_atomic_xchg,			&xchg_32_arg),
	UNIT_TEST(atomic_xchg_64,			test_atomic_xchg,			&xchg_64_arg),
	UNIT_TEST(atomic_cmpxchg_32,			test_atomic_cmpxchg,			&xchg_32_arg),
	UNIT_TEST(atomic_cmpxchg_64,			test_atomic_cmpxchg,			&xchg_64_arg),
	UNIT_TEST(atomic_add_unless_32,			test_atomic_add_unless,			&add_unless_32_arg),
	UNIT_TEST(atomic_add_unless_64,			test_atomic_add_unless,			&add_unless_64_arg),
	UNIT_TEST(atomic_inc_32_threaded,		test_atomic_arithmetic_threaded,	&inc_32_arg),
	UNIT_TEST(atomic_inc_64_threaded,		test_atomic_arithmetic_threaded,	&inc_64_arg),
	UNIT_TEST(atomic_dec_32_threaded,		test_atomic_arithmetic_threaded,	&dec_32_arg),
	UNIT_TEST(atomic_dec_64_threaded,		test_atomic_arithmetic_threaded,	&dec_64_arg),
	UNIT_TEST(atomic_add_32_threaded,		test_atomic_arithmetic_threaded,	&add_32_arg),
	UNIT_TEST(atomic_add_64_threaded,		test_atomic_arithmetic_threaded,	&add_64_arg),
	UNIT_TEST(atomic_sub_32_threaded,		test_atomic_arithmetic_threaded,	&sub_32_arg),
	UNIT_TEST(atomic_sub_64_threaded,		test_atomic_arithmetic_threaded,	&sub_64_arg),
	UNIT_TEST(atomic_inc_and_test_32_threaded,	test_atomic_arithmetic_threaded,	&inc_and_test_32_arg),
	UNIT_TEST(atomic_inc_and_test_64_threaded,	test_atomic_arithmetic_threaded,	&inc_and_test_64_arg),
	UNIT_TEST(atomic_dec_and_test_32_threaded,	test_atomic_arithmetic_threaded,	&dec_and_test_32_arg),
	UNIT_TEST(atomic_dec_and_test_64_threaded,	test_atomic_arithmetic_threaded,	&dec_and_test_64_arg),
	UNIT_TEST(atomic_sub_and_test_32_threaded,	test_atomic_arithmetic_threaded,	&sub_and_test_32_arg),
	UNIT_TEST(atomic_sub_and_test_64_threaded,	test_atomic_arithmetic_threaded,	&sub_and_test_64_arg),
	UNIT_TEST(atomic_add_unless_32_threaded,	test_atomic_arithmetic_threaded,	&add_unless_32_arg),
	UNIT_TEST(atomic_add_unless_64_threaded,	test_atomic_arithmetic_threaded,	&add_unless_64_arg),

};

UNIT_MODULE(atomic, atomic_tests, UNIT_PRIO_POSIX_TEST);
