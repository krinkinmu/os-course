#ifndef __STACK_1_HPP__
#define __STACK_1_HPP__

#include <cstddef>
#include <cassert>
#include <cstdint>
#include <limits>
#include <atomic>

template <typename T>
class Stack {
	typedef std::uint16_t	StackNodePtr;
	typedef std::uint16_t	StackNodeTag;
	typedef std::uint32_t	StackHeadPtr;

	struct StackNode {
		StackNodePtr next;
		T *data;

		StackNode(T *data = nullptr) : next(0), data(data) { }
	};

	StackNode *			pool;
	std::atomic<StackHeadPtr>	head;
	std::atomic<StackHeadPtr>	free;

	static StackHeadPtr createPtr(StackNodePtr ptr, StackNodeTag tag)
	{ return ptr | ((StackHeadPtr)tag << 16); }

	static StackNodePtr getPtr(StackHeadPtr ptr)
	{ return ptr & 0xffff; }

	static StackNodeTag getTag(StackHeadPtr ptr)
	{ return ptr >> 16; }

	StackNode *getNode(StackNodePtr ptr)
	{ return &pool[ptr]; }

	StackNodePtr allocatePtr()
	{
		StackHeadPtr head, next;
		StackNodePtr ptr;

		do {
			head = free.load(std::memory_order_relaxed);
			ptr = getPtr(head);

			if (!ptr)
				continue;

			StackNode *node = getNode(ptr);
			next = createPtr(node->next, getTag(head) + 1);
		} while (!free.compare_exchange_strong(head, next));

		return ptr;
	}

	void freePtr(StackNodePtr ptr)
	{
		StackHeadPtr head, new_head;
		StackNode *node = getNode(ptr);

		do {
			head = free.load(std::memory_order_relaxed);
			node->next = getPtr(head);
			new_head = createPtr(ptr, getTag(head) + 1);
		} while (!free.compare_exchange_strong(head, new_head));
	}

public:
	Stack(std::size_t size);
	~Stack();

	void push(T *value);
	T *pop();
};

template <typename T>
Stack<T>::Stack(std::size_t size)
{
	assert(size <= std::numeric_limits<StackNodePtr>::max());

	this->pool = new StackNode[size];

	for (std::size_t i = 1; i != size; ++i)
		this->pool[i].next = i - 1;

	this->free.store(createPtr(size - 1, 0));
	this->head.store(createPtr(0, 0));
}

template <typename T>
Stack<T>::~Stack()
{
	assert(getPtr(head.load(std::memory_order_relaxed)) == 0);
	delete[] pool;
}

template <typename T>
void Stack<T>::push(T *value)
{
	StackHeadPtr old_head, new_head;
	StackNodePtr ptr = allocatePtr();
	StackNode *node = getNode(ptr);
	node->data = value;
	do {
		old_head = head.load(std::memory_order_relaxed);
		new_head = createPtr(ptr, getTag(old_head) + 1);
		node->next = getPtr(old_head);
	} while (!head.compare_exchange_strong(old_head, new_head));
}

template <typename T>
T *Stack<T>::pop()
{
	StackHeadPtr old_head, new_head;
	StackNodePtr ptr;
	StackNode *node;

	do {
		old_head = head.load(std::memory_order_relaxed);
		ptr = getPtr(old_head);
		if (!ptr)
			return nullptr;
		node = getNode(ptr);
		new_head = createPtr(node->next, getTag(old_head) + 1);
	} while (!head.compare_exchange_strong(old_head, new_head));

	T *data = node->data;
	freePtr(ptr);
	return data;
}

#endif /*__STACK_1_HPP__*/
