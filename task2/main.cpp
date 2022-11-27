#include "Array.h"
#include <iostream>

int main() {
	Array<int> a;
	for (int i = 0; i < 10; ++i)
		a.insert(i + 1);

	for (int i = 0; i < a.size(); ++i)
		a[i] *= 2;

	for (auto it = a.begin(); it.hasNext(); ++it)
		std::cout << *it << std::endl;
	system("pause");
	return 0;
}