// Copyright GarageGames.  See /license/info.txt in this distribution for licensing terms.

// Maintain a reserve of half the public size
struct array_half_reserve_policy
{
   inline static uint32 grow_to(uint32 public_size, uint32 array_size) {
      return public_size + public_size / 2;
   }
   inline static uint32 shrink_to(uint32 public_size, uint32 array_size) {
      return (public_size < array_size / 2)?
         array_size + array_size / 4: array_size;
   };
};

template<class type, class policy = array_half_reserve_policy> class array
{
public:
	typedef type value_type;
	typedef type &reference;
	typedef const type &const_reference;
	typedef type *iterator;
	typedef const type *const_iterator;

	array()
	{
		_public_size = 0;
		_array_size = 0;
		_array = 0;
	}
	
	array(const uint32 initial_reserve)
	{
		_public_size = 0;
		_array_size = initial_reserve;
		_array = initial_reserve ? (type *) memory_allocate(initial_reserve * sizeof(type)) : 0;
	}
	
	array(const array &p)
	{
		_public_size = 0;
		_array_size = 0;
		_array = 0;
		*this = p;
	}
	
	~array()
	{
		destroy(begin(), end());
		memory_deallocate(_array);
	}
	
	uint32 size() const
	{
		return _public_size;
	}
	
	uint32 capacity() const
	{
		return _array_size;
	}
	
	void reserve(uint32 size)
	{
		if(size > _array_size)
			_resize(size);
	}
	
	void resize(uint32 size)
	{
		if(size > _public_size)
		{
			if(size > _array_size)
				_resize(policy::grow_to(size, _array_size));
			construct(end(), end() + (size - _public_size));
			_public_size = size;
		}
		else if(size < _public_size)
		{
			destroy(end() - (_public_size - size), end());
			_public_size = size;
			uint32 new_size = policy::shrink_to(size, _array_size);
			if(new_size != _array_size)
				_resize(new_size);
		}
	}
	
	void compact()
	{
		if(_public_size < _array_size)
			_resize(_public_size);
	}
	
	void clear()
	{
		destroy(begin(), end());
		memory_deallocate(_array);
		_public_size = 0;
		_array_size = 0;
		_array = 0;
	}
	
	bool is_empty()
	{
		return _public_size == 0;
	}
	
	iterator insert(uint32 index, const type &x)
	{
		if(_public_size + 1 > _array_size)
		{
			uint32 new_size = policy::grow_to(_public_size + 1, _array_size);
			assert(new_size > _public_size);
			type *new_array = (type *) memory_allocate(new_size * sizeof(type));
			uninitialized_copy(_array + index, end(), new_array + index + 1);
			destroy(begin(), end());
			memory_deallocate(_array);
			_array = new_array;
			_array_size = new_size;
		} else {
			construct(end());
			copy_backwards(_array + index, end(), end() + 1);
			_array[index] = x;
		}
		_public_size++;
		return _array + index;
	}
	
	iterator insert(iterator itr, const type &x)
	{
		return insert(uint32(itr - _array), x);
	}
	
	iterator insert(uint32 index)
	{
		return insert(index, type());
	}
	
	iterator insert(iterator itr)
	{
		return insert(uint32(itr - _array));
	}
	
	iterator push_front(const type &x)
	{
		return insert(uint32(0), x);
	}
	
	iterator push_back(const type &x)
	{
		if(_public_size + 1 > _array_size)
			_resize(policy::grow_to(_public_size + 1, _array_size));
		return construct(_array + _public_size++, x);
	}
	
	iterator push_front()
	{
		return insert(uint32(0));
	}
	
	iterator push_back()
	{
		return push_back(type());
	}
	
	void erase(uint32 index)
	{
		if(has_trivial_copy<type>::is_true)
		{
			copy(_array + index + 1, end(), _array + index);
			destroy(&last());
			uint32 new_size = policy::shrink_to(--_public_size, _array_size);
			assert(new_size >= _public_size);
			if(new_size != _array_size)
			{
				_array = (type *) memory_reallocate(_array, new_size * sizeof(type));
				_array_size = new_size;
			}
		}
		else
		{
			uint32 new_size = policy::shrink_to(_public_size - 1, _array_size);
			if(new_size != _array_size)
			{
				assert(new_size >= _public_size - 1);
				type *new_array = (type *) memory_allocate(new_size * sizeof(type));
				uninitialized_copy(begin(), _array + index, new_array);
				uninitialized_copy(_array + index + 1, end(), new_array + index);
				destroy(begin(), end());
				memory_deallocate(_array);
				_array = new_array;
				_array_size = new_size;
			}
			else
			{
				copy(_array + index + 1, end(), _array + index);
				destroy(&last());
			}
			_public_size--;
		}
	}
	
	void erase(iterator itr)
	{
		erase(uint32(itr - _array));
	}
	
	void erase_unstable(iterator itr)
	{
		if(itr != end() - 1)
			core::swap(*itr, last());
		pop_back();
	}
	
	void erase_unstable(uint32 index)
	{
		erase_unstable(&_array[index]);
	}
	
	void pop_front()
	{
		erase(uint32(0));
	}
	
	void pop_back()
	{
		destroy(&last());
		uint32 size = policy::shrink_to(--_public_size, _array_size);
		if(size != _array_size)
			_resize(size);
	}
	
	iterator begin()
	{
		return _array;
	}
	
	const_iterator begin() const
	{
		return _array;
	}
	
	iterator end()
	{
		return _array + _public_size;
	}
	
	const_iterator end() const
	{
		return _array + _public_size;
	}
	
	iterator rbegin()
	{
		return end() - 1;
	}
	
	const_iterator rbegin() const
	{
		return end() - 1;
	}
	
	iterator rend()
	{
		return begin() - 1;
	}
	
	const_iterator rend() const
	{
		return begin() - 1;
	}
	
	type &first()
	{
		return _array[0];
	}
	
	const type &first() const
	{
		return _array[0];
	}
	
	type &last()
	{
		return _array[_public_size - 1];
	}
	
	const type &last() const
	{
		return _array[_public_size - 1];
	}
	
	type &operator[](uint32 index)
	{
		return _array[index];
	}
	
	const type &operator[](uint32 index) const
	{
		return _array[index];
	}
	
	void operator=(const array &p)
	{
		if(_array_size != p._array_size)
		{
			destroy(begin(), end());
			memory_deallocate(_array);
			_array_size = p._array_size;
			_array = (type *) memory_allocate(p._array_size * sizeof(type));
			uninitialized_copy(p.begin(), p.end(), _array);
		}
		else if(_public_size > p._public_size)
		{
			destroy(&_array[p._public_size], end());
			copy(p.begin(), p.end(), begin());
		}
		else
		{
			copy(p.begin(), &p[_public_size], begin());
			uninitialized_copy(&p[_public_size], p.end(), end());
		}
		_public_size = p._public_size;
	}
protected:
	uint32 _public_size;
	uint32 _array_size;
	type *_array;
	
	void _resize(uint32 size)
	{
		assert(size >= _public_size);
		if(has_trivial_copy<type>::is_true)
			_array = (type *) memory_reallocate(_array, size * sizeof(type));
		else
		{
			type *new_array = (type *) memory_allocate(size * sizeof(type));
			uninitialized_copy(begin(), end(), new_array);
			destroy(begin(), end());
			memory_deallocate(_array);
			_array = new_array;
		}
		_array_size = size;
	}
};

/*template<class type, class policy> struct container_manipulator<array<type, policy> >
{
	static type_record *get_key_type()
	{
		return get_global_type_record<uint32>();
	}
	static type_record *get_value_type()
	{
		return get_global_type_record<type>();
	}
};*/
