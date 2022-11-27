#ifndef SORT_H
#define SORT_H

#include "../task2/Array.h"

constexpr int THRESHOLD = 21;

template<typename T, typename Compare>
T selectPivot(T a, T b, T c, Compare comp)
{
	if (comp(*b, *a) && comp(*a, *c) || comp(*c, *a) && comp(*a, *b)) {
		return a;
	}
	else if (comp(*a, *b) && comp(*b, *c) || comp(*c, *b) && comp(*b, *a)) {
		return b;
	}
	else {
		return c;
	}
}

template<typename T, typename Compare>
void insertionSort(T first, T last, Compare comp)
{
	if (first < last) {
		T j;
		for (T i = first + 1; i <= last; ++i) {
			auto key = *i;
			j = i - 1;
			while (j >= first && comp(key, *j)) {
				*(j + 1) = std::move(*j);
				--j;
			}
			*(j + 1) = key;
		}
	}
}

template<typename T, typename Compare>
void sort(T first, T last, Compare comp, bool insertionSortOptimization)
{
	while (first < last) {
		if (insertionSortOptimization && last - first < THRESHOLD) {
			insertionSort(first, last, comp);
			break;
		}
		auto pivot = *(selectPivot(first, last, first + (last - first) / 2, comp));
		T i = first;
		T j = last;
		while (true) {
			while (comp(*i, pivot)) {
				++i;
			}
			while (comp(pivot, *j)) {
				--j;
			}
			if (i >= j) {
				break;
			}
			std::swap(*i, *j);
			++i;
			--j;
		}
		if (j - first < last - j - 1) {
			sort(first, j, comp, insertionSortOptimization);
			first = j + 1;
		}
		else {
			sort(j + 1, last, comp, insertionSortOptimization);
			last = j;
		}
	}
}

#endif