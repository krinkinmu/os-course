#ifndef __STACK_1_HPP__
#define __STACK_1_HPP__

#include "haz_ptr.hpp"
#include <cassert>
#include <cstddef>
#include <atomic>

template <typename T>
class Stack {
	struct StackNode {
		StackNode *next;
		T *data;

		StackNode(T *data) : next(nullptr), data(data) { }
	};

	std::atomic<StackNode *> head;

public:
	Stack();
	~Stack();

	void push(T *value);
	T *pop();
};

template <typename T>
Stack<T>::Stack()
{
	this->head.store(nullptr);
}

template <typename T>
Stack<T>::~Stack()
{
	assert(head.load(std::memory_order_relaxed) == nullptr);
}

template <typename T>
void Stack<T>::push(T *value)
{
	StackNode *node = new StackNode(value);
	StackNode *expected;

	do {
		expected = this->head.load(std::memory_order_relaxed);
		node->next = expected;
	} while (!this->head.compare_exchange_strong(expected, node));
}

template <typename T>
T *Stack<T>::pop()
{
	HazardPtr<StackNode> hp(acquireRawHazardPtr(0));
	StackNode *node;
	StackNode *next;

	do {
		node = head.load();
		do {
			if (!node)
				return nullptr;
			hp.set(node);
		} while ((node = head.load()) != hp.get());

		next = node->next;
	} while (!head.compare_exchange_strong(node, next));

	hp.set(nullptr);
	T *data = node->data;
	releasePtr(node);
	return data;
}

#endif /*__STACK_1_HPP__*/
