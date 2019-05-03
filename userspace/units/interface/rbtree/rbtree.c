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

#include <unit/io.h>
#include <unit/unit.h>
#include <unit/core.h>

#include <nvgpu/rbtree.h>
#include <stdlib.h>

/*
 * To make testing easier, most tests will use the same rbtree that is built
 * according to:
 * - The tree will contain 9 nodes (10 insertions, but one rejected as
 *   duplicate)
 * - The values in the tree express a range. All nodes have the same range.
 * - The values and the order in which they are inserted is carefully chosen
 *   to maximize code coverage by ensuring that all corner cases are hit
 */
#define INITIAL_ELEMENTS 10
#define RANGE_SIZE 10

/*
 * Sample tree used throughout this unit. Node values below are key_start.
 *
 *         130 (Black)
 *        /   \
 *       /     \
 *     50       200  (Red)
 *    /  \     /   \
 *   30  80   170   300  (Black)
 *  /        /
 * 10      120  (Red)
 *
 * NOTE: There is a duplicate entry that will be ignored during insertion.
 */
#define DUPLICATE_VALUE 300
u64 initial_key_start[] = {50, 30, 80, 100, 170, 10, 200, DUPLICATE_VALUE,
	DUPLICATE_VALUE, 120};


/*
 * The following key value should not exist or cover a range from the keys
 * above.
 */
#define INVALID_KEY_START 2000

/*
 * The following key will be used to search and range_search in the tree. It is
 * chosen so that paths taken will involve both left and right branches.
 */
#define SEARCH_KEY 120

/*
 * The values below will cause the red-black properties to be violated upon
 * insertion into the tree defined above. As a result, these will trigger
 * specific cases during the tree rebalancing procedure.
 */
#define RED_BLACK_VIOLATION_1 20
#define RED_BLACK_VIOLATION_2 320


struct nvgpu_rbtree_node *elements[INITIAL_ELEMENTS];

/*
 * Helper function to ensure a given tree satisfies all the properties to be
 * considered a red-black binary tree. That is:
 * 1. Every node is either red or black: implied since color is a bool with only
 *    two possible values.
 * 2. The root is black: checked by the function below.
 * 3. Every leaf is black: implied since all leaves are NULL.
 * 4. If a node is red, then both its children have to be black: checked by the
 *    function below.
 * 5. All simple paths from a node to its descendant leaves must contain the
 *    same number of black nodes: checked by the function below.
 *
 * So only properties 2, 4 and 5 need to be checked.
 *
 * Returns either:
 * - a negative value in case of error
 * - the number of black nodes to leaves (which is the black height of the tree
 *   when ran from the root).
 */
static int check_rbtree(struct unit_module *m, struct nvgpu_rbtree_node *node)
{
	int left_black_count, right_black_count;
	int black_count = 0;

	if (node == NULL) {
		/* This is a leaf, so black count is 1 */
		return 1;
	}

	/* Check property 2 (root is black) */
	if ((node->parent == NULL) && (node->is_red)) {
		unit_err(m, "check_rbtree: root is red\n");
		return -1;
	}

	/* Check property 4 (if red node, children must be black) */
	if (node->is_red) {
		/*
		 * If left or right is NULL then it is a leaf which is
		 * implicitly black
		 */
		if ((node->left != NULL) && (node->left->is_red)) {
			unit_err(m,
			 "check_rbtree: l_child of red parent is also red\n");
			return -1;
		}
		if ((node->right != NULL) && (node->right->is_red)) {
			unit_err(m,
			 "check_rbtree: r_child of red parent is also red\n");
			return -1;
		}
	}

	/* Count black nodes */
	if (!node->is_red) {
		black_count = 1;
	}

	/*
	 * Check property 5 (descendant leaves must have the same number of
	 * black nodes)
	 * Start by recursively checking the height of the left and right
	 * sub-trees.
	 */
	left_black_count = check_rbtree(m, node->left);
	right_black_count = check_rbtree(m, node->right);

	if ((left_black_count == -1) || (right_black_count == -1)) {
		/* There was an error in one of the subtrees, propagate it */
		return -1;
	}

	if (left_black_count != right_black_count) {
		unit_err(m, "check_rbtree: mismatch between left and right\n");
		return -1;
	}

	return left_black_count + black_count;
}

/*
 * Helper function to insert elements into a tree using the initial_key_start
 * values.
 */
static int fill_test_tree(struct unit_module *m,
				struct nvgpu_rbtree_node **root)
{
	int i;

	for (i = 0; i < INITIAL_ELEMENTS; i++) {
		elements[i] = (struct nvgpu_rbtree_node *)
				malloc(sizeof(struct nvgpu_rbtree_node));
		elements[i]->key_start = initial_key_start[i];
		elements[i]->key_end = initial_key_start[i]+RANGE_SIZE;
		nvgpu_rbtree_insert(elements[i], root);
	}

	return UNIT_SUCCESS;
}

/*
 * Helper function to free the test nodes of the tree.
 */
static int free_test_tree(struct unit_module *m, struct nvgpu_rbtree_node *root)
{
	int i;

	for (i = 0; i < INITIAL_ELEMENTS; i++) {
		free(elements[i]);
	}
	/* No need to explicitly free the root as it was one of the elements */

	return UNIT_SUCCESS;
}

/*
 * Test to check the nvgpu_rbtree_insert operation.
 * First will create the test tree and check that it is valid.
 * Then it will insert some well chosen values to target specific branches
 * in the re-balancing code.
 */
static int test_insert(struct unit_module *m, struct gk20a *g, void *args)
{
	struct nvgpu_rbtree_node *root = NULL;
	struct nvgpu_rbtree_node *node1, *node2 = NULL;
	int status = UNIT_FAIL;

	fill_test_tree(m, &root);
	if (check_rbtree(m, root) < 0) {
		goto free_tree;
	}

	node1 = (struct nvgpu_rbtree_node *)
				malloc(sizeof(struct nvgpu_rbtree_node));
	node1->key_start = RED_BLACK_VIOLATION_1;
	node1->key_end = RED_BLACK_VIOLATION_1+RANGE_SIZE;
	nvgpu_rbtree_insert(node1, &root);

	node2 = (struct nvgpu_rbtree_node *)
				malloc(sizeof(struct nvgpu_rbtree_node));
	node2->key_start = RED_BLACK_VIOLATION_2;
	node2->key_end = RED_BLACK_VIOLATION_2+RANGE_SIZE;
	nvgpu_rbtree_insert(node2, &root);

	if (check_rbtree(m, root) < 0) {
		goto free_nodes;
	}

	status = UNIT_SUCCESS;
free_nodes:
	free(node1);
	free(node2);
free_tree:
	free_test_tree(m, root);
	return status;
}

/*
 * Test to check the nvgpu_rbtree_unlink operation by removing every node from
 * the tree.
 * This test will also use the nvgpu_rbtree_search operation to check that
 * the node was effectively removed.
 */
static int test_unlink(struct unit_module *m, struct gk20a *g, void *args)
{
	struct nvgpu_rbtree_node *root = NULL;
	struct nvgpu_rbtree_node *result = NULL;
	int status = UNIT_FAIL;
	bool duplicate_handled = false;
	u64 key_start_search;
	int i;

	fill_test_tree(m, &root);

	for (i = 0; i < INITIAL_ELEMENTS; i++) {
		/*
		 * Search for a node from values in the initial_key_start table.
		 */
		key_start_search = initial_key_start[i];
		if ((key_start_search == DUPLICATE_VALUE) &&
			(!duplicate_handled)) {
				duplicate_handled = true;
				continue;
		}

		nvgpu_rbtree_search(key_start_search, &result, root);
		if (result == NULL) {
			unit_err(m, "Search failed for key_start=%lld\n",
					key_start_search);
			goto cleanup;
		} else {
			if (verbose_lvl(m) > 0) {
				unit_info(m, "Found node with key_start=%lld\n",
						result->key_start);
			}
		}

		/*
		 * Unlink will simply remove the node from the tree. It will not
		 * free the resources. It will be done at the end of this
		 * function.
		 */
		nvgpu_rbtree_unlink(result, &root);

		/* Make sure the node was actually removed */
		nvgpu_rbtree_search(key_start_search, &result, root);
		if (result != NULL) {
			unit_err(m, "Unlink failed, node still exists\n");
			goto cleanup;
		} else {
			if (verbose_lvl(m) > 0) {
				unit_info(m, "Node was removed as expected\n");
			}
		}
	}

	status = UNIT_SUCCESS;
cleanup:
	free_test_tree(m, root);
	return status;
}

/*
 * Test to check the nvgpu_rbtree_search and nvgpu_rbtree_range_search routines
 * and go over some error handling.
 */
static int test_search(struct unit_module *m, struct gk20a *g, void *args)
{
	struct nvgpu_rbtree_node *root = NULL;
	struct nvgpu_rbtree_node *result1, *result2 = NULL;
	int status = UNIT_FAIL;
	u64 key_start_search = SEARCH_KEY;

	fill_test_tree(m, &root);

	/* Search with a NULL root should not crash and keep result as NULL */
	nvgpu_rbtree_search(key_start_search, &result1, NULL);
	if (result1 != NULL) {
		unit_err(m, "Search did not fail as expected\n");
		goto cleanup;
	}

	/* Same thing with the range_search operation */
	nvgpu_rbtree_range_search(key_start_search, &result2, NULL);
	if (result1 != NULL) {
		unit_err(m, "Range search did not fail as expected\n");
		goto cleanup;
	}

	/* Now search for a real value */
	if (verbose_lvl(m) > 0) {
		unit_info(m, "Searching for key_start=%lld\n",
			key_start_search);
	}
	nvgpu_rbtree_search(key_start_search, &result1, root);
	if (result1 == NULL) {
		unit_err(m, "Search failed\n");
		goto cleanup;
	} else if (verbose_lvl(m) > 0) {
		unit_info(m, "Found node with key_start=%lld key_end=%lld\n",
				result1->key_start, result1->key_end);
	}

	/*
	 * Now do a range search by just incrementing key_start_search by 1
	 * which should yield the exact same result as the previous search
	 * (since it will fall in the same range)
	 */
	key_start_search++;
	if (verbose_lvl(m) > 0) {
		unit_info(m, "Range searching for key=%lld\n",
			key_start_search);
	}
	nvgpu_rbtree_range_search(key_start_search, &result2, root);
	if (result2 == NULL) {
		unit_err(m, "Range search failed\n");
		goto cleanup;
	} else if (result1 != result2) {
		unit_err(m, "Range search did not find the expected result\n");
		goto cleanup;
	} else if (verbose_lvl(m) > 0) {
		unit_info(m, "Found node with key_start=%lld key_end=%lld\n",
				result1->key_start, result1->key_end);
	}

	status = UNIT_SUCCESS;
cleanup:
	free_test_tree(m, root);
	return status;
}

/*
 * Test to check the nvgpu_rbtree_enum_start routine and go over some error
 * handling.
 */
static int test_enum(struct unit_module *m, struct gk20a *g, void *args)
{
	struct nvgpu_rbtree_node *root = NULL;
	struct nvgpu_rbtree_node *node = NULL;
	int status = UNIT_FAIL;
	u64 key_start;
	int i;

	fill_test_tree(m, &root);

	/* Enum with a NULL root should not crash and keep result as NULL */
	nvgpu_rbtree_enum_start(0, &node, NULL);
	if (node != NULL) {
		unit_err(m, "Enum did not fail as expected (NULL root)\n");
		goto cleanup;
	}

	/* Enum all the nodes we know are in the tree */
	for (i = 0; i < INITIAL_ELEMENTS; i++) {
		key_start = initial_key_start[i];
		nvgpu_rbtree_enum_start(key_start, &node, root);
		if (node->key_start != key_start) {
			unit_err(m, "Enum mismatch\n");
			goto cleanup;
		}
	}

	/* If the key_start does not exist, enum should return a NULL node */
	nvgpu_rbtree_enum_start(INVALID_KEY_START, &node, root);
	if (node != NULL) {
		unit_err(m, "Enum did not fail as expected (wrong key_start\n");
		goto cleanup;
	}

	status = UNIT_SUCCESS;
cleanup:
	free_test_tree(m, root);
	return status;
}

/*
 * Test to check the nvgpu_rbtree_enum_next routine and go over some error
 * handling.
 * nvgpu_rbtree_enum_next will find the next node whose key_start value is
 * greater than the one in the provided node.
 */
static int test_enum_next(struct unit_module *m, struct gk20a *g, void *args)
{
	struct nvgpu_rbtree_node *root = NULL;
	struct nvgpu_rbtree_node *node = NULL;
	int status = UNIT_FAIL;
	u64 prev_key_start;

	fill_test_tree(m, &root);

	/* Enum with a NULL root should not crash and keep result as NULL */
	nvgpu_rbtree_enum_next(&node, NULL);
	if (node != NULL) {
		unit_err(m, "Enum_next did not fail as expected (NULL root)\n");
		goto cleanup;
	}

	/*
	 * The tree is balanced and we know there are INITIAL_ELEMENTS inside.
	 * Enumerate the next key_start value from root.
	 */
	node = root;
	prev_key_start = node->key_start;
	while (node != NULL) {
		nvgpu_rbtree_enum_next(&node, root);
		if (node != NULL) {
			if (verbose_lvl(m) > 0) {
				unit_info(m, "Node has key_start=%lld\n",
					node->key_start);
			}
			if (node->key_start < prev_key_start) {
				unit_err(m, "Enum_next returned a low value\n");
				goto cleanup;
			}
			prev_key_start = node->key_start;
		}
	}

	/* For branch coverage, test some error handling. */
	node = NULL;
	nvgpu_rbtree_enum_next(&node, root);
	nvgpu_rbtree_enum_next(&node, NULL);

	status = UNIT_SUCCESS;
cleanup:
	free_test_tree(m, root);
	return status;
}

/*
 * Test to check the nvgpu_rbtree_less_than_search routine.
 * Given a key_start value, find a node with a lower key_start value.
 */
static int test_search_less(struct unit_module *m, struct gk20a *g, void *args)
{
	struct nvgpu_rbtree_node *root = NULL;
	struct nvgpu_rbtree_node *result;
	int status = UNIT_FAIL;
	u64 key_start_search;

	fill_test_tree(m, &root);

	/*
	 * The tree is balanced, so the range in the root should be in the
	 * middle of the values, so searching for that value will guarantee
	 * a result.
	 */
	key_start_search = root->key_start;

	nvgpu_rbtree_less_than_search(key_start_search, &result, root);
	if (result == NULL) {
		unit_err(m, "less_than_search unexpectedly failed\n");
		goto cleanup;
	}

	if (result->key_start >= key_start_search) {
		unit_err(m, "less_than_search returned a wrong result\n");
		goto cleanup;
	}

	status = UNIT_SUCCESS;
cleanup:
	free_test_tree(m, root);
	return status;
}

struct unit_module_test interface_rbtree_tests[] = {
	UNIT_TEST(insert, test_insert, NULL, 0),
	UNIT_TEST(search, test_search, NULL, 0),
	UNIT_TEST(unlink, test_unlink, NULL, 0),
	UNIT_TEST(enum, test_enum, NULL, 0),
	UNIT_TEST(enum_next, test_enum_next, NULL, 0),
	UNIT_TEST(search_less_than, test_search_less, NULL, 0),
};

UNIT_MODULE(interface_rbtree, interface_rbtree_tests, UNIT_PRIO_NVGPU_TEST);
