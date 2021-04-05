/*
 * Copyright (c) 2017-2021, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef NVGPU_RBTREE_H
#define NVGPU_RBTREE_H

#include <nvgpu/types.h>

/**
 * @defgroup rbtree
 * @ingroup unit-common-utils
 * @{
 */

/**
 * A node in the rbtree
 */
struct nvgpu_rbtree_node {
	/**
	 * Start of range for the key used for searching/inserting in the tree
	 */
	u64 key_start;
	/**
	 * End of range for the key used for searching/inserting the tree
	 */
	u64 key_end;

	/**
	 * Is this a red node? (!is_red ==> is_black)
	 */
	bool is_red;

	/**
	 * Parent of this node
	 */
	struct nvgpu_rbtree_node *parent;
	/**
	 * Left child of this node (key is less than this node's key)
	 */
	struct nvgpu_rbtree_node *left;
	/**
	 * Right child of this node. (key is greater than this node's key)
	 */
	struct nvgpu_rbtree_node *right;
};

/**
 * @brief Insert a new node into rbtree.
 *
 * @param new_node [in]	Pointer to new node.
 * @param root [in]	Pointer to root of tree
 *
 * - Find the correct location in the tree for this node based on its key value
 *   and insert it by updating the pointers.
 * - Rebalance tree.
 *
 * NOTE: Nodes with duplicate key_start and overlapping ranges are not allowed.
 */
void nvgpu_rbtree_insert(struct nvgpu_rbtree_node *new_node,
		    struct nvgpu_rbtree_node **root);

/**
 * @brief Delete a node from rbtree.
 *
 * @param node [in]	Pointer to node to be deleted
 * @param root [in]	Pointer to root of tree
 *
 * - Update tree pointers to remove this node from tree while keeping its
 *   children.
 * - Rebalance tree.
 */
void nvgpu_rbtree_unlink(struct nvgpu_rbtree_node *node,
		    struct nvgpu_rbtree_node **root);

/**
 * @brief Search for a given key in rbtree
 *
 * @param key_start [in]	Key to be searched in rbtree
 * @param node [out]		Node pointer to be returned
 * @param root [in]		Pointer to root of tree
 *
 * This API will match given key against key_start of each node.
 * In case of a hit, node points to a node with given key.
 * In case of a miss, node is NULL.
 */
void nvgpu_rbtree_search(u64 key_start, struct nvgpu_rbtree_node **node,
			     struct nvgpu_rbtree_node *root);

/**
 * @brief Search a node with key falling in range
 *
 * @param key [in]	Key to be searched in rbtree
 * @param node [out]	Node pointer to be returned
 * @param root [in]	Pointer to root of tree
 *
 * This API will match given key and find a node where key value
 * falls within range of {start, end} keys.
 * In case of a hit, node points to a node with given key.
 * In case of a miss, node is NULL.
 */
void nvgpu_rbtree_range_search(u64 key,
			       struct nvgpu_rbtree_node **node,
			       struct nvgpu_rbtree_node *root);

/**
 * @brief Search a node with key lesser than given key
 *
 * @param key_start [in]	Key to be searched in rbtree
 * @param node [out]		Node pointer to be returned
 * @param root [in]		Pointer to root of tree
 *
 * This API will match given key and find a node with highest
 * key value lesser than given key.
 * In case of a hit, node points to a node with given key.
 * In case of a miss, node is NULL.
 */
void nvgpu_rbtree_less_than_search(u64 key_start,
			       struct nvgpu_rbtree_node **node,
			       struct nvgpu_rbtree_node *root);

/**
 * @brief Enumerate tree starting at the node with specified value.
 *
 * @param key_start [in]	Key value to begin enumeration from
 * @param node [out]		Pointer to first node in the tree
 * @param root [in]		Pointer to root of tree
 *
 * This API returns node pointer pointing to first node in the rbtree to begin
 * enumerating the tree. Call this API once per enumeration. Call
 * #nvgpu_rbtree_enum_next to get the next node.
 */
void nvgpu_rbtree_enum_start(u64 key_start,
			struct nvgpu_rbtree_node **node,
			struct nvgpu_rbtree_node *root);

/**
 * @brief Find next node in enumeration in order by key value.
 *
 * To get the next node in the tree, pass in the current \a node. This API
 * returns \a node pointer pointing to next node in the rbtree in order by key
 * value.
 *
 * @param node [in,out]	Pointer to current node is passed in.
 *			Pointer to next node in the tree is passed back.
 * @param root [in]	Pointer to root of tree
 */
void nvgpu_rbtree_enum_next(struct nvgpu_rbtree_node **node,
		       struct nvgpu_rbtree_node *root);

/**
 * @}
 */
#endif /* NVGPU_RBTREE_H */
