#ifndef ARRAY_H
#define ARRAY_H

#include <cstdlib>

template<typename T>
class Array final
{
public:
	Array();
	Array(int capacity);
	Array(const Array& rhs);
	Array(Array&& rhs);
	~Array();

	void swap(Array& rhs);
	Array& operator=(Array rhs);
	int insert(const T& value);
	int insert(int index, const T& value);
	void remove(int index);

	const T& operator[](int index) const
	{
		return data_[index];
	}

	T& operator[](int index)
	{
		return data_[index];
	}

	int size() const
	{
		return size_;
	}

	int capacity() const
	{
		return capacity_;
	}

	int defaultCapacity() const
	{
		return DEFAULT_CAPACITY;
	}

	class ConstIterator
	{
	public:
		ConstIterator()
			: arrayData_(nullptr),
			isReverse_(false),
			index_(0)
		{};

		ConstIterator(Array* p, bool isReverse, int index)
			: arrayData_(p),
			isReverse_(isReverse),
			index_(index)
		{};

		bool operator==(const ConstIterator& rhs)
		{
			return (arrayData_ == rhs.arrayData_ && index_ == rhs.index_);
		}

		bool operator!=(const ConstIterator& rhs)
		{
			return !(*this == rhs);
		}

		bool operator>(const ConstIterator& rhs)
		{
			return (arrayData_ == rhs.arrayData_ && index_ > rhs.index_);
		}

		bool operator<(const ConstIterator& rhs)
		{
			return (arrayData_ == rhs.arrayData_ && index_ < rhs.index_);
		}

		bool operator>=(const ConstIterator& rhs)
		{
			return (arrayData_ == rhs.arrayData_ && index_ >= rhs.index_);
		}

		bool operator<=(const ConstIterator& rhs)
		{
			return (arrayData_ == rhs.arrayData_ && index_ <= rhs.index_);
		}

		ConstIterator& operator++()
		{
			if (isReverse_) {
				--index_;
			}
			else {
				++index_;
			}
			return *this;
		}

		ConstIterator operator++(int)
		{
			ConstIterator temp = *this;
			++*this;
			return temp;
		}

		ConstIterator& operator--()
		{
			if (isReverse_) {
				++index_;
			}
			else {
				--index_;
			}
			return *this;
		}

		ConstIterator operator--(int)
		{
			ConstIterator temp = *this;
			--*this;
			return temp;
		}

		ConstIterator operator+(int value)
		{
			ConstIterator result = *this;
			result.index_ += value;
			return result;
		}

		ConstIterator operator-(int value)
		{
			ConstIterator result = *this;
			result.index_ -= value;
			return result;
		}

		int operator-(const ConstIterator& rhs)
		{
			return (index_ - rhs.index_);
		}

		ConstIterator& operator+=(int value)
		{
			index_ += value;
			return *this;
		}

		ConstIterator& operator-=(int value)
		{
			index_ -= value;
			return *this;
		}

		const T& operator*()
		{
			return (*arrayData_)[index_];
		}

		bool hasNext() const
		{
			if (isReverse_) {
				return index_ >= 0;
			}
			else {
				return index_ < arrayData_->size();
			}			
		}

	private:
		Array* arrayData_;
		int index_;
		bool isReverse_;
	};

	class Iterator
	{
	public:
		Iterator()
			: arrayData_(nullptr),
			isReverse_(false),
			index_(0)
		{};

		Iterator(Array* p, bool isReverse, int index)
			: arrayData_(p),
			isReverse_(isReverse),
			index_(index)
		{};

		bool operator==(const Iterator& rhs)
		{
			return (arrayData_ == rhs.arrayData_ && index_ == rhs.index_);
		}

		bool operator!=(const Iterator& rhs)
		{
			return !(*this == rhs);
		}

		bool operator>(const Iterator& rhs)
		{
			return (arrayData_ == rhs.arrayData_ && index_ > rhs.index_);
		}

		bool operator<(const Iterator& rhs)
		{
			return (arrayData_ == rhs.arrayData_ && index_ < rhs.index_);
		}

		bool operator>=(const Iterator& rhs)
		{
			return (arrayData_ == rhs.arrayData_ && index_ >= rhs.index_);
		}

		bool operator<=(const Iterator& rhs)
		{
			return (arrayData_ == rhs.arrayData_ && index_ <= rhs.index_);
		}

		Iterator& operator++()
		{
			if (isReverse_) {
				--index_;
			}
			else {
				++index_;
			}
			return *this;
		}

		Iterator operator++(int)
		{
			ConstIterator temp = *this;
			++*this;
			return temp;
		}

		Iterator& operator--()
		{
			if (isReverse_) {
				++index_;
			}
			else {
				--index_;
			}
			return *this;
		}

		Iterator operator--(int)
		{
			Iterator temp = *this;
			--*this;
			return temp;
		}

		Iterator operator+(int value)
		{
			Iterator result = *this;
			result.index_ += value;
			return result;
		}

		Iterator operator-(int value)
		{
			Iterator result = *this;
			result.index_ -= value;
			return result;
		}

		int operator-(const Iterator& rhs)
		{
			return (index_ - rhs.index_);
		}

		Iterator& operator+=(int value)
		{
			index_ += value;
			return *this;
		}

		Iterator& operator-=(int value)
		{
			index_ -= value;
			return *this;
		}

		T& operator*()
		{
			return (*arrayData_)[index_];
		}

		bool hasNext() const
		{
			if (isReverse_) {
				return index_ >= 0;
			}
			else {
				return index_ < arrayData_->size();
			}
		}

	private:
		Array* arrayData_;
		int index_;
		bool isReverse_;
	};

	Iterator begin()
	{
		return Iterator(this, false, 0);
	}
	Iterator end()
	{
		return Iterator(this, false, size_);
	}

	ConstIterator cbegin()
	{
		return ConstIterator(this, false, 0);
	}
	ConstIterator cend()
	{
		return ConstIterator(this, false, size_);
	}

	Iterator rbegin()
	{
		return Iterator(this, true, size_ - 1);
	}
	Iterator rend()
	{
		return Iterator(this, true, -1);
	}

	ConstIterator crbegin()
	{
		return ConstIterator(this, true, size_ - 1);
	}
	ConstIterator crend()
	{
		return ConstIterator(this, true, -1);
	}

private:
	int size_;
	int capacity_;
	T* data_;

	static constexpr int DEFAULT_CAPACITY = 8;
	static constexpr double EXPANSION_RATIO = 2.0;
};

template<typename T>
Array<T>::Array()
{
	data_ = (T*)malloc(DEFAULT_CAPACITY * sizeof(T));
	capacity_ = DEFAULT_CAPACITY;
	size_ = 0;
}

template<typename T>
Array<T>::Array(int capacity)
{
	if (capacity > 0) {
		data_ = (T*)malloc(capacity * sizeof(T));
		capacity_ = capacity;
	}
	else {
		data_ = (T*)malloc(DEFAULT_CAPACITY * sizeof(T));
		capacity_ = DEFAULT_CAPACITY;
	}
	size_ = 0;
}

template<typename T>
Array<T>::Array(const Array& rhs)
{
	data_ = (T*)malloc(rhs.capacity_ * sizeof(T));
	for (int i = 0; i < rhs.size_; ++i) {
		new(data_ + i) T(rhs.data_[i]);
	}
	capacity_ = rhs.capacity_;
	size_ = rhs.size_;
}

template<typename T>
Array<T>::Array(Array&& rhs)
{
	data_ = rhs.data_;
	capacity_ = rhs.capacity_;
	size_ = rhs.size_;
	rhs.data_ = nullptr;
	rhs.size_ = 0;
}

template<typename T>
Array<T>::~Array()
{
	for (int i = 0; i < size_; ++i) {
		data_[i].~T();
	}
	free(data_);
}

template<typename T>
void Array<T>::swap(Array& rhs)
{
	std::swap(data_, rhs.data_);
	std::swap(size_, rhs.size_);
	std::swap(capacity_, rhs.capacity_);
}

template<typename T>
Array<T>& Array<T>::operator=(Array rhs)
{
	swap(rhs);
	return *this;
}

template<typename T>
int Array<T>::insert(const T& value)
{
	if (size_ == capacity_) {
		capacity_ *= static_cast<int>(EXPANSION_RATIO);
		T* p = (T*)malloc(capacity_ * sizeof(T));
		for (int i = 0; i < size_; ++i) {
			new(p + i) T(std::move(data_[i]));
			data_[i].~T();
		}
		free(data_);
		data_ = p;
	}
	new(data_ + size_) T(value);
	return size_++;
}

template <typename T>
int Array<T>::insert(int index, const T& value)
{
	if (index >= 0 && index < size_) {
		if (size_ == capacity_) {
			capacity_ *= static_cast<int>(EXPANSION_RATIO);
			T* p = (T*)malloc(capacity_ * sizeof(T));
			for (int i = 0; i < index; ++i) {
				new(p + i) T(std::move(data_[i]));
				data_[i].~T();
			}
			new(p + index) T(value);
			for (int i = index; i < size_; ++i) {
				new(p + i + 1) T(std::move(data_[i]));
				data_[i].~T();
			}
			free(data_);
			data_ = p;
		}
		else {
			new(data_ + size_) T(std::move(data_[size_ - 1]));
			for (int i = size_ - 1; i > index; --i) {
				data_[i] = std::move(data_[i - 1]);
			}
			data_[index] = value;
		}
		++size_;
		return index;
	}
	else {
		return insert(value);
	}
}

template <typename T>
void Array<T>::remove(int index)
{
	if (size_ == 0 || index < 0 || index >= size_) {
		return;
	}
	for (int i = index; i < size_ - 1; ++i) {
		data_[i] = std::move(data_[i + 1]);
	}
	data_[size_ - 1].~T();
	--size_;
}

#endif