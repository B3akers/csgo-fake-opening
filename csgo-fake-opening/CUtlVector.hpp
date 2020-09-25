#pragma once

template < typename T, typename I = int >
class CUtlMemory {
public:
	T* GetBuffer( void ) {
		return m_pMemory;
	}

	int GetAllocationCount( void ) {
		return m_nAllocationCount;
	}

	T& operator[]( I i ) {
		return m_pMemory[ i ];
	}


	int GetGrowSize( void ) {
		return m_nGrowSize;
	}

	T* OffsetBufferByIndex( std::size_t index ) {
		return m_pMemory + index;
	}

private:
	T* m_pMemory;
	int m_nAllocationCount;
	int m_nGrowSize;
};


template <class T>
inline T* Construct( T* pMemory ) {
	return ::new( pMemory ) T;
}

template <class T>
inline void Destruct( T* pMemory ) {
	pMemory->~T( );
}

template < typename T, typename Allocator = CUtlMemory< T > >
class CUtlVector {
public:
	Allocator GetMemory( void ) {
		return m_Memory;
	}

	T& Element( int i ) {
		return m_Memory[ i ];
	}

	T& operator[]( int i ) {
		return m_Memory[ i ];
	}

	int GetSize( void ) {
		return m_Size;
	}

	T* GetElements( void ) {
		return m_pElements;
	}
	void RemoveAll( ) {
		for ( int i = m_Size; --i >= 0; )
			Destruct( &Element( i ) );

		m_Size = 0;
	}
private:
	Allocator m_Memory;
	int m_Size;
	T* m_pElements;
};
