#include "stdafx.h"
#include "CppUnitTest.h"
#include "../task2/Array.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace task2_tests
{		
	TEST_CLASS(UnitTest1)
	{
	public:
		
		TEST_METHOD(DefaultConstructorAndInsertElement)
		{
			Array<int> a;
			Assert::AreEqual(a.size(), 0);
			Assert::AreEqual(a.capacity(), a.defaultCapacity());

			a.insert(1);
			Assert::AreEqual(a.size(), 1);
			Assert::AreEqual(a[0], 1);
			Assert::AreEqual(a.capacity(), a.defaultCapacity());
		}

		TEST_METHOD(CapacityConstructorAndMultipleInsert)
		{
			Array<int> a(3);
			Assert::AreEqual(a.size(), 0);
			Assert::AreEqual(a.capacity(), 3);

			a.insert(0, 1);
			a.insert(0, 2);
			a.insert(2, 3);
			Assert::AreEqual(a.size(), 3);
			Assert::AreEqual(a.capacity(), 3);
			Assert::AreEqual(a[0], 2);
			Assert::AreEqual(a[1], 1);
			Assert::AreEqual(a[2], 3);

			a.insert(3, 4);
			Assert::AreEqual(a.size(), 4);
			Assert::AreEqual(a.capacity(), 6);
			Assert::AreEqual(a[0], 2);
			Assert::AreEqual(a[1], 1);
			Assert::AreEqual(a[2], 3);
			Assert::AreEqual(a[3], 4);
		}

		TEST_METHOD(IncorrectInsertAndRemove)
		{
			Array<int> a(2);
			a.insert(5, 1);
			a.insert(-3, 2);
			Assert::AreEqual(a[0], 1);
			Assert::AreEqual(a[1], 2);

			a.remove(2);
			Assert::AreEqual(a.size(), 2);
			a.remove(0);
			Assert::AreEqual(a.size(), 1);
			Assert::AreEqual(a[0], 2);
		}

		TEST_METHOD(CopyConstructor)
		{
			Array<int> a(3);
			for (int i = 0; i < 3; ++i) {
				a.insert(i, i + 1);
			}
			Array<int> b(a);
			Assert::AreEqual(b[0], a[0]);
			Assert::AreEqual(b[1], a[1]);
			Assert::AreEqual(b[2], a[2]);
			Assert::AreEqual(b.size(), a.size());
			Assert::AreEqual(b.capacity(), a.capacity());

			a[0] = 10;
			Assert::AreEqual(b[0], 1);
		}

		TEST_METHOD(MoveConstructor)
		{
			Array<int> a(3);
			for (int i = 0; i < 3; ++i) {
				a.insert(i, i + 1);
			}
			Array<int> b(std::move(a));
			Assert::AreEqual(b[0], 1);
			Assert::AreEqual(b[1], 2);
			Assert::AreEqual(b[2], 3);
			Assert::AreEqual(b.size(), 3);
			Assert::AreEqual(b.capacity(), 3);
			Assert::AreEqual(a.size(), 0);
		}

		TEST_METHOD(CopyAssignment)
		{
			Array<int> a(3);
			for (int i = 0; i < 3; ++i) {
				a.insert(i, i + 1);
			}
			Array<int> b = a;
			Assert::AreEqual(b[0], a[0]);
			Assert::AreEqual(b[1], a[1]);
			Assert::AreEqual(b[2], a[2]);
			Assert::AreEqual(b.size(), a.size());
			Assert::AreEqual(b.capacity(), a.capacity());

			a[0] = 10;
			Assert::AreEqual(b[0], 1);
		}

		TEST_METHOD(MoveAssignment)
		{
			Array<int> a(3);
			for (int i = 0; i < 3; ++i) {
				a.insert(i, i + 1);
			}
			Array<int> b = std::move(a);
			Assert::AreEqual(b[0], 1);
			Assert::AreEqual(b[1], 2);
			Assert::AreEqual(b[2], 3);
			Assert::AreEqual(b.size(), 3);
			Assert::AreEqual(b.capacity(), 3);
			Assert::AreEqual(a.size(), 0);
		}

		TEST_METHOD(IteratorTest)
		{
			Array<int> a(5);
			for (int i = 0; i < 5; ++i) {
				a.insert(i, i + 1);
			}
			int i = 0;
			for (auto it = a.iterator(); it.hasNext(); it.next()) {
				Assert::IsTrue(it.get() == a[i]);
				++i;
			}
			i = a.size() - 1;
			for (auto it = a.reverseIterator(); it.hasNext(); it.next()) {
				Assert::IsTrue(it.get() == a[i]);
				--i;
			}
		}

		TEST_METHOD(StringInsertAndDelete)
		{
			Array<std::string> a(3);
			std::string str1("string1");
			std::string str2("string22222222222222222222222222222222222222222222222222222222");
			std::string str3("string33333333333333333333333333333333333333333333333333333333");
			a.insert(str1);
			a.insert(str2);
			a.insert(str3);
			a.remove(1);
			Assert::AreEqual(a[0], str1);
			Assert::AreEqual(a[1], str3);
		}

		TEST_METHOD(StringCopyConstructor)
		{
			Array<std::string> a(3);
			std::string str1("string1");
			std::string str2("string22222222222222222222222222222222222222222222222222222222");
			std::string str3("string33333333333333333333333333333333333333333333333333333333");
			a.insert(str1);
			a.insert(str2);
			a.insert(str3);
			Array<std::string> b(a);
			Assert::AreEqual(b[0], a[0]);
			Assert::AreEqual(b[1], a[1]);
			Assert::AreEqual(b[2], a[2]);
			Assert::AreEqual(b.size(), a.size());
			Assert::AreEqual(b.capacity(), a.capacity());

			a[0] = "NOT string1";
			Assert::AreEqual(b[0], str1);
		}

		TEST_METHOD(StringMoveConstructor)
		{
			Array<std::string> a(3);
			std::string str1("string1");
			std::string str2("string22222222222222222222222222222222222222222222222222222222");
			std::string str3("string33333333333333333333333333333333333333333333333333333333");
			a.insert(str1);
			a.insert(str2);
			a.insert(str3);
			Array<std::string> b(std::move(a));
			Assert::AreEqual(b[0], str1);
			Assert::AreEqual(b[1], str2);
			Assert::AreEqual(b[2], str3);
			Assert::AreEqual(b.size(), 3);
			Assert::AreEqual(b.capacity(), 3);
			Assert::AreEqual(a.size(), 0);
		}

		TEST_METHOD(StringCopyAssignment)
		{
			Array<std::string> a(3);
			std::string str1("string1");
			std::string str2("string22222222222222222222222222222222222222222222222222222222");
			std::string str3("string33333333333333333333333333333333333333333333333333333333");
			a.insert(str1);
			a.insert(str2);
			a.insert(str3);
			Array<std::string> b = a;
			Assert::AreEqual(b[0], a[0]);
			Assert::AreEqual(b[1], a[1]);
			Assert::AreEqual(b[2], a[2]);
			Assert::AreEqual(b.size(), a.size());
			Assert::AreEqual(b.capacity(), a.capacity());

			a[0] = "NOT string1";
			Assert::AreEqual(b[0], str1);
		}

		TEST_METHOD(StringMoveAssignment)
		{
			Array<std::string> a(3);
			std::string str1("string1");
			std::string str2("string22222222222222222222222222222222222222222222222222222222");
			std::string str3("string33333333333333333333333333333333333333333333333333333333");
			a.insert(str1);
			a.insert(str2);
			a.insert(str3);
			Array<std::string> b = std::move(a);
			Assert::AreEqual(b[0], str1);
			Assert::AreEqual(b[1], str2);
			Assert::AreEqual(b[2], str3);
			Assert::AreEqual(b.size(), 3);
			Assert::AreEqual(b.capacity(), 3);
			Assert::AreEqual(a.size(), 0);
		}
	};
}