#include <cassert>

#include "stack1.hpp"

int main()
{
	const int count = 1000;
	Stack<int> stack;

	for (int i = 0; i != count; ++i)
		stack.push(new int(i));

	for (int i = 0; i != count; ++i) {
		int *value = stack.pop();

		assert(value != nullptr);
		assert(*value == count - i - 1);
		delete value;
	}

	return 0;
}
