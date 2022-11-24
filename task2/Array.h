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
	Array& operator=(Array&& rhs);
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
		ConstIterator(Array* p, bool isReverse)
			: arrayData_(p),
			isReverse_(isReverse)
		{
			if (isReverse_) {
				index_ = arrayData_->size() - 1;
			}
			else {
				index_ = 0;
			}
		};

		const T& get() const
		{
			return (*arrayData_)[index_];
		}		

		void next()
		{
			if (isReverse_) {
				--index_;
			}
			else {
				++index_;
			}			
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

	protected:
		Array* arrayData_;
		int index_;
		bool isReverse_;
	};

	class Iterator : public ConstIterator
	{
	public:
		Iterator(Array* p, bool isReverse)
			: ConstIterator(p, isReverse)
		{};

		void set(const T& value)
		{
			(*ConstIterator::arrayData_)[ConstIterator::index_] = value;
		}
	};

	Iterator iterator()
	{
		return Iterator(this, false);
	}
	ConstIterator iterator() const
	{
		return ConstIterator(this, false);
	}
	Iterator reverseIterator()
	{
		return Iterator(this, true);
	}
	ConstIterator reverseIterator() const
	{
		return ConstIterator(this, true);
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
Array<T>& Array<T>::operator=(Array&& rhs)
{
	if (this != &rhs) {
		for (int i = 0; i < size_; ++i) {
			data_[i].~T();
		}
		free(data_);
		data_ = rhs.data_;
		capacity_ = rhs.capacity_;
		size_ = rhs.size_;
		rhs.data_ = nullptr;
		rhs.size_ = 0;
	}
	return *this;
}

template<typename T>
int Array<T>::insert(const T& value)
{
	if (size_ == capacity_) {
		capacity_ *= EXPANSION_RATIO;
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
			capacity_ *= EXPANSION_RATIO;
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