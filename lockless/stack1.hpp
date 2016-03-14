#ifndef __STACK_1_HPP__
#define __STACK_1_HPP__

#include <cstddef>
#include <cassert>
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
		expected = head.load(std::memory_order_relaxed);
		node->next = expected;
	} while (!head.compare_exchange_strong(expected, node));
}

template <typename T>
T *Stack<T>::pop()
{
	StackNode *node;
	StackNode *next;

	do {
		node = head.load(std::memory_order_relaxed);
		if (!node)
			return nullptr;
		next = node->next;
	} while (!head.compare_exchange_strong(node, next));

	T *data = node->data;
	delete node;
	return data;
}

#endif /*__STACK_1_HPP__*/
