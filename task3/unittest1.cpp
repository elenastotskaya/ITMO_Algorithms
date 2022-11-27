#include "stdafx.h"
#include "CppUnitTest.h"
#include "../task3/sort.h"
#include "../task2/Array.h"
#include <ctime>


using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace task3_tests
{		
	TEST_CLASS(UnitTest1)
	{
	public:
		
		TEST_METHOD(SingleValueSort)
		{
			int a[1] = { 1 };
			sort(a, a, [](int a, int b) { return a < b; }, true);
			Assert::AreEqual(a[0], 1);
		}

		TEST_METHOD(IntegerDescendingSort)
		{
			srand(time(0));
			int a[100];
			for (int i = 0; i < 100; ++i) {
				a[i] = rand() % 100;
			}
			sort(a, a + 99, [](int a, int b) { return a > b; }, true);
			for (int i = 0; i < 99; ++i) {
				Assert::IsTrue(a[i] >= a[i + 1]);
			}
		}

		TEST_METHOD(DoubleAscendingSort)
		{
			srand(time(0));
			double a[100];
			for (int i = 0; i < 100; ++i) {
				a[i] = static_cast<double>(rand()) / RAND_MAX;
			}
			sort(a, a + 99, [](double a, double b) { return a < b; }, true);
			for (int i = 0; i < 99; ++i) {
				Assert::IsTrue(a[i] <= a[i + 1]);
			}
		}

		TEST_METHOD(StringSort)
		{
			std::string a[10] = { "bc", "a", "abc", "ab", "12", "10", "14", "144444444", "a", "114" };
			sort(a, a + 9, [](std::string a, std::string b) { return a.compare(b) < 0; }, true);
			for (int i = 0; i < 9; ++i) {
				Assert::IsTrue(a[i].compare(a[i + 1]) <= 0);
			}
		}

		TEST_METHOD(CustomArraySort)
		{
			Array<int> a(100);
			for (int i = 0; i < 100; ++i) {
				a.insert(i, rand() % 100);
			}
			sort(a.begin(), a.end() - 1, [](auto a, auto b) { return a < b; }, true);
			for (int i = 0; i < 99; ++i) {
				Assert::IsTrue(a[i] <= a[i + 1]);
			}
		}
	};
}