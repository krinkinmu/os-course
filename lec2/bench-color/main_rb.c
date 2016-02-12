#include <stdlib.h>
#include <malloc.h>
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <time.h>
#include "rb.h"

#define CACHE_LINE_SIZE 64

struct rb_int_node {
	struct rb_node node;
	int value;
	char dummy[CACHE_LINE_SIZE - sizeof(struct rb_node) - sizeof(int)];
};

static struct rb_int_node **alloc_nodes_color(size_t size)
{
	struct rb_int_node *nodes = calloc(size, sizeof(struct rb_int_node));
	struct rb_int_node **array = calloc(size, sizeof(struct rb_int_node *));

	for (size_t i = 0; i != size; ++i)
		array[i] = nodes + i;
	return array;
}

static void free_nodes_color(struct rb_int_node **nodes, size_t size)
{
	free(nodes[0]);
	free(nodes);
}

static struct rb_int_node **alloc_nodes(size_t size)
{
	struct rb_int_node **array = calloc(size, sizeof(struct rb_int_node *));

	for (size_t i = 0; i != size; ++i) {
		array[i] = memalign(512, sizeof(struct rb_int_node));
		assert(array[i] != 0);
	}
	return array;
}

static void free_nodes(struct rb_int_node **nodes, size_t size)
{
	for (size_t i = 0; i != size; ++i)
		free(nodes[i]);
	free(nodes);
}

static void insert(struct rb_tree *tree, struct rb_int_node *node)
{
	struct rb_node **plink = &tree->root;
	struct rb_node *parent = 0;

	while (*plink) {
		struct rb_int_node *x = (struct rb_int_node *)*plink;

		parent = *plink;
		if (x->value < node->value)
			plink = &parent->right;
		else
			plink = &parent->left;
	}

	rb_link(&node->node, parent, plink);
	rb_insert(&node->node, tree);
}

static struct rb_int_node *lookup(struct rb_tree *tree, int value)
{
	struct rb_node **plink = &tree->root;
	struct rb_node *parent = 0;

	while (*plink) {
		struct rb_int_node *x = (struct rb_int_node *)*plink;

		if (x->value == value)
			return x;

		parent = *plink;
		if (x->value < value)
			plink = &parent->right;
		else
			plink = &parent->left;
	}
	return 0;
}

static long long get_time(void)
{
	static long long NS = 1000000000ull;
	struct timespec spec;

	clock_gettime(CLOCK_MONOTONIC, &spec);
	return NS * spec.tv_sec + spec.tv_nsec;
}

static long long benchmark(struct rb_int_node **nodes, size_t size)
{
	struct rb_tree tree = { 0 };

	for (size_t i = 0; i != size; ++i)
		nodes[i]->value = rand();

	for (size_t i = 0; i != size; ++i)
		insert(&tree, nodes[i]);

	const long start = get_time();

	for (size_t i = 0; i != size; ++i) {
		struct rb_int_node *rc = lookup(&tree, rand());

		__asm__ volatile ("" : : : "memory");
		(void) rc;
	}

	return get_time() - start;
}

int main()
{
	const size_t size = 1000000ul;
	struct rb_int_node **nodes;

	nodes = alloc_nodes_color(size);
	printf("%lld\n", benchmark(nodes, size));
	free_nodes_color(nodes, size);

	nodes = alloc_nodes(size);
	printf("%lld\n", benchmark(nodes, size));
	free_nodes(nodes, size);

	return 0;
}
