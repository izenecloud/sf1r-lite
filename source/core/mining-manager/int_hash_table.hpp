#ifndef INT_HASH_TABLE_HPP
#define INT_HASH_TABLE_HPP

#include <types.h>
#include <ir/dup_det/integer_dyn_array.hpp>

NS_IZENELIB_AM_BEGIN

template<uint32_t ENTRY_SIZE = 1000000, class INT_TYPE = uint32_t>
class IntHashTable
{
	typedef izenelib::am::IntegerDynArray<INT_TYPE> Vector32;
	typedef izenelib::am::IntegerDynArray<Vector32*> Vector32Ptr;
	typedef INT_TYPE size_t;
	typedef IntHashTable<ENTRY_SIZE> SelfT;

protected:
	Vector32Ptr entry_;
	FILE* f_;
	size_t num_;

public:
	inline IntHashTable(const char* filenm)
	{
		num_ = 0;
		entry_.reserve(ENTRY_SIZE);
		for (uint32_t i = 0; i < ENTRY_SIZE; i++)
			entry_.push_back(NULL);

		f_ = fopen(filenm, "r+");
		if (f_ == NULL)
		{
			f_ = fopen(filenm, "w+");
			if (f_ == NULL)
			{
				std::cout << "Can't create file: " << filenm << std::endl;
				return;
			}
			fwrite(&num_, sizeof(size_t), 1, f_);
		}
		else
		{
			//std::cout<<"Loading....\n";
			load();
		}
	}

	inline ~IntHashTable()
	{
		fclose(f_);
		for (size_t i = 0; i < entry_.length(); i++)
		{
			Vector32* v = entry_.at(i);
			if (v == NULL)
				continue;
			delete v;
		}
	}

	inline void load()
	{
		num_ = 0;
		fseek(f_, 0, SEEK_SET);
		fread(&num_, sizeof(size_t), 1, f_);

		size_t index = 0;
		IASSERT(fread(&index, sizeof(size_t), 1, f_) == 1);
		for (size_t i = 0; i < entry_.length(); i++)
		{
			if (index != i)
				continue;

			size_t len = 0;
			IASSERT(fread(&len, sizeof(size_t), 1, f_) == 1);
			entry_[i] = new Vector32();
			IASSERT(fread(entry_.at(i)->array(len), len * sizeof(uint32_t), 1,
					f_) == 1);
			if (fread(&index, sizeof(size_t), 1, f_) != 1)
				index = -1;
		}
	}

	inline void flush()
	{
		fseek(f_, 0, SEEK_SET);
		fwrite(&num_, sizeof(size_t), 1, f_);

		for (size_t i = 0; i < entry_.length(); i++)
		{
			Vector32* v = entry_.at(i);
			if (v == NULL)
				continue;

			fwrite(&i, sizeof(size_t), 1, f_);
			size_t len = v->length();
			fwrite(&len, sizeof(size_t), 1, f_);
			fwrite(v->data(), v->size(), 1, f_);
		}

		fflush(f_);
	}

	inline void insert(INT_TYPE key, INT_TYPE value1, INT_TYPE value2)
	{
		size_t i = key % ENTRY_SIZE;
		if (entry_.at(i) == NULL)
			entry_[i] = new Vector32();

		for (size_t j = 0; j < entry_.at(i)->length(); ++j, ++j)
			if (entry_.at(i)->at(j++) == key)
				return;

		entry_.at(i)->push_back(key);
		entry_.at(i)->push_back(value1);
		entry_.at(i)->push_back(value2);
		num_++;
	}

	inline size_t length() const
	{
		return num_;
	}

	inline void compact()
	{
		for (size_t i = 0; i < entry_.length(); i++)
		{
			Vector32* v = entry_.at(i);
			if (v == NULL)
				continue;
			v->compact();
		}

	}

	inline INT_TYPE& operator[](size_t key)
	{
		static size_t r = -1;

		size_t i = key % ENTRY_SIZE;

		Vector32* p = entry_.at(i);

		for (size_t j = 0; j < p->length(); ++j, ++j)
			if (p->at(j++) == key)
				return (*p)[j];

		return r;
	}

	inline bool find(size_t key, size_t& value1, size_t& value2)
	{
		size_t i = key % ENTRY_SIZE;

		Vector32* p = entry_.at(i);

		for (size_t j = 0; j < p->length(); ++j, ++j)
			if (p->at(j++) == key)
			{
				value1 = p->at(j++);
				value2 = p->at(j++);
				return true;
			}

		return false;
	}

	/**
	 *This is for outputing into std::ostream, say, std::cout.
	 **/
	friend std::ostream& operator <<(std::ostream& os, const SelfT& v)
	{

		return os;
	}

};

NS_IZENELIB_AM_END
#endif
