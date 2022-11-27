#include "sort.h"
#include <windows.h>
#include <ctime>
#include <fstream>

#include <iostream>

double getTime()
{
	LARGE_INTEGER freq, val;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&val);
	return (double)val.QuadPart / (double)freq.QuadPart;
}

int main()
{
	constexpr int MIN_SIZE = 5;
	constexpr int MAX_SIZE = 100;
	constexpr int MAX_TESTS = 10000;
	srand(time(0));
	std::fstream output;
	output.open("test_results.txt", std::ios::out);
	for (int arraySize = MIN_SIZE; arraySize <= MAX_SIZE; ++arraySize) {
		double totalTimeQuickSort = 0.0;
		double totalTimeInsertionSort = 0.0;
		int* randomArray = (int*) malloc(sizeof(int) * arraySize);
		int* backwardArray = (int*)malloc(sizeof(int) * arraySize);
		for (int numTest = 0; numTest < MAX_TESTS; ++numTest) {
			for (int i = 0; i < arraySize; ++i) {
				randomArray[i] = rand() % arraySize;
				backwardArray[i] = arraySize - i;
			}
			double tStart = getTime();
			sort(randomArray, randomArray + arraySize - 1, [](int a, int b) { return a < b; }, false);
			totalTimeQuickSort += 1000 * (getTime() - tStart);
			tStart = getTime();
			insertionSort(backwardArray, backwardArray + arraySize - 1, [](int a, int b) { return a < b; });
			totalTimeInsertionSort += 1000 * (getTime() - tStart);
		}
		free(randomArray);
		free(backwardArray);
		totalTimeQuickSort /= MAX_TESTS;
		totalTimeInsertionSort /= MAX_TESTS;
		output << totalTimeQuickSort << " " << totalTimeInsertionSort << "\n";
	}
	output.close();
	return 0;
}