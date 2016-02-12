#ifndef __RED_BLACK_TREE_H__
#define __RED_BLACK_TREE_H__

#include <stdint.h>

struct rb_node {
	struct rb_node *left;
	struct rb_node *right;
	uintptr_t parent;
};

struct rb_tree {
	struct rb_node *root;
};

static inline void rb_link(struct rb_node *node, struct rb_node *parent,
			struct rb_node **plink)
{
	node->parent = (uintptr_t)parent;
	node->left = node->right = 0;
	*plink = node;
}

struct rb_node *rb_last(struct rb_tree *tree);
struct rb_node *rb_first(struct rb_tree *tree);
struct rb_node *rb_next(struct rb_node *node);
struct rb_node *rb_prev(struct rb_node *node);
void rb_erase(struct rb_node *node, struct rb_tree *tree);
void rb_insert(struct rb_node *node, struct rb_tree *tree);

#endif /*__RED_BLACK_TREE_H__*/
