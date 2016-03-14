#include <cassert>

#include "stack2.hpp"

int main()
{
	const int count = 10;
	Stack<int> stack(count + 1);

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
