#ifndef _PA_ARRAY
#define _PA_ARRAY

namespace patools {

#include "pacollector.h"
#define kPaArrayGranularityDefault 256

template <class T>
class PaArray : public PaCollector<T>
{
public:

	PaArray (const guint32 size = 0, const guint32 granularity = kPaArrayGranularityDefault) ;
	PaArray (const T *data, const guint32 size, const guint32 granularity) ;
	PaArray (const PaArray<T> &, const guint32 granularity = 0) ;
	PaArray (const PaCollector<T> &, const guint32 granularity = kPaArrayGranularityDefault) ;

	~PaArray () ;

	guint32 GetSize () const {return size;}

	const T& Get (const guint32 i) const ;

	void Add (const T &) ;
	void Set (const guint32 i, const T & newValue) ;
	void Insert (const guint32 i, const T & value) ;
	void Delete (const guint32 i) ;

	void Empty () ;

	const T & operator [] (const guint32 i) const ;
	T & operator [] (const guint32 i) ;

	PaCollector<T> & operator = (const PaCollector<T> &) ;
	PaArray & operator = (const PaArray<T> &) ;
	PaArray operator + (const PaCollector<T> &) const ;
	PaArray operator + (const PaArray<T> &) const ;
	PaCollector<T> & operator += (const PaCollector<T> &) ;
	PaArray & operator += (const PaArray<T> &) ;
	bool operator == (const PaArray<T> &) const ;
	bool operator != (const PaArray<T> &) const ;
	bool operator == (const PaCollector<T> &) const ;
	bool operator != (const PaCollector<T> &) const ;
	PaArray operator () (const guint32 startPos, const guint32 endPos) const ;

	guint32 GetGranularity () const { return granularity ; }

private :

	T ** blockArray ;
	guint32 nbBlock ;
	guint32 size ;

	const guint32 granularity ;
} ;

#ifndef _GENERATE_DOC

template <class T>
PaArray<T>::PaArray (const guint32 size, const guint32 granularity /* = kPaArrayGranularityDefault */)
    : granularity (granularity > 0 ? granularity : kPaArrayGranularityDefault)
{
	if (size > 1)
		nbBlock = (size - 1) / this->granularity + 1  ;
	else
		nbBlock = 1 ;

	blockArray = new T* [nbBlock] ;
	this->size = size ;
	for (guint32 i = 0 ; i < nbBlock ; i++)
	{
		blockArray [i] = new T [this->granularity] ;
	}
}

template <class T>
PaArray<T>::PaArray (const T *data, const guint32 size /* = 0 */, const guint32 granularity /* = 0 */)
    : granularity (granularity > 0 ? granularity : kPaArrayGranularityDefault)
{
	this->size = size ;
	if (size > 1)
		nbBlock = (size - 1) / this->granularity + 1  ;
	else
		nbBlock = 1 ;
    
	blockArray = new T* [nbBlock] ;
    
	blockArray [0] = new T [this->granularity] ;
	for (guint32 i = 0, j = 0, k = 0 ; i < size ; i++, k++)
	{
		if (k >= this->granularity)
		{
			k = 0 ;
			blockArray [++j] = new T [this->granularity] ;
		}
		blockArray [j][k] = data [i] ;
	}
}

template <class T>
PaArray<T>::PaArray (const PaCollector<T> & pc, const guint32 granularity /* = kPaArrayGranularityDefault */)
    : granularity (granularity > 0 ? granularity : kPaArrayGranularityDefault)
{
	size = pc.GetSize () ;
	if (size > 1)
		nbBlock = (size - 1) / this->granularity + 1  ;
	else
		nbBlock = 1 ;

	blockArray = new T* [nbBlock] ;

	blockArray [0] = new T [this->granularity] ;
	for (guint32 i = 0, j = 0, k = 0 ; i < size ; i++, k++)
	{
		if (k >= this->granularity)
		{
			k = 0 ;
			blockArray [++j] = new T [this->granularity] ;
		}
		blockArray [j][k] = pc [i] ;
	}
}

template <class T>
PaArray<T>::PaArray (const PaArray<T> & pa, const guint32 granularity /* = 0 */)
    : granularity (granularity > 0 ? granularity : pa.granularity)
{
	size = pa.size ;
	if (size > 1)
		nbBlock = (size - 1) / this->granularity + 1  ;
	else
		nbBlock = 1 ;

	blockArray = new T* [nbBlock] ;

	blockArray [0] = new T [this->granularity] ;

	for (guint32 i = 0, j = 0, k = 0, l = 0, m = 0 ; i < size ; i++, k++, m++)
	{
		if (k >= this->granularity)
		{
			k = 0 ;
			blockArray [++j] = new T [this->granularity] ;
		}
		if (m >= pa.granularity)
		{
			m = 0 ;
			l ++ ;
		}
		blockArray [j][k] = pa.blockArray [l][m] ;
	}
}

template <class T>
PaArray<T>::~PaArray ()
{
	for (guint32 i = 0 ; i < nbBlock && blockArray [i] != NULL ; i++)
	{
		delete [] (blockArray [i]) ;
	}
	delete [] blockArray ;
}

template <class T>
T& PaArray<T>::operator [] (const guint32 index)
{
	guint32 i ;
	if (size == 0) i = 0 ;
	else i = index < size ? index : size - 1 ;

	return blockArray [i / granularity][i % granularity] ;
}

template <class T>
const T& PaArray<T>::operator [] (const guint32 index) const
{
	guint32 i ;
	if (size == 0) i = 0 ;
	else i = index < size ? index : size - 1 ;

	return blockArray [i / granularity][i % granularity] ;
}

template <class T>
const T & PaArray<T>::Get (const guint32 index) const
{
	guint32 i ;
	if (size == 0) i = 0 ;
	else i = index < size ? index : size - 1 ;

	return blockArray [i / granularity][i % granularity] ;
}

template <class T>
void PaArray<T>::Set (const guint32 index, const T & newValue)
{
	if (index < size)
		blockArray [index / granularity][index % granularity] = newValue ;
}

template <class T>
void PaArray<T>::Insert (const guint32 index, const T & value)
{
	guint32 index2 = index ;

	if (index >= size)
	{
		Add (value) ;
		return ;
	}
	guint32 iBlock = size / granularity ;
	guint32 iPos = size % granularity ;

	if (size != 0 && iPos == 0)
	{
		if (iBlock < nbBlock)
		{
			blockArray [iBlock] = new T [granularity] ;
		}
		else
		{
			guint32 i ;
			T** tmpArray = new T* [nbBlock + 1]  ;
			for (i = 0 ; i < nbBlock ; i++)
			{
				tmpArray [i] = blockArray [i] ;
			}
			tmpArray [i] = new T [granularity] ;
			delete [] blockArray ;
			blockArray = tmpArray ;
			nbBlock ++ ;
		}
	}

	T* array = blockArray [iBlock] ;
	for (guint32 j = size ; j > index2 ; j--)
	{
		if (iPos == 0)
		{
			array [0] = blockArray [iBlock - 1][granularity - 1] ;
			iPos = granularity - 1 ;
			array = blockArray [-- iBlock] ;
		}
		else
		{
			array [iPos] = array [iPos - 1] ;
			iPos -- ;
		}
	}
	array [iPos] = value ;
	size ++ ;
}

template <class T>
void PaArray<T>::Add (const T & value)
{
	guint32	iBlock = size / granularity ;
	guint32 iPos = size % granularity ;
	if (size != 0 && iPos == 0)
	{
		if (iBlock < nbBlock)
		{
			blockArray [iBlock] = new T [granularity] ;
		}
		else
		{
			guint32 i ;
			T** tmpArray = new T* [nbBlock + 1] ;
			for (i = 0 ; i < nbBlock ; i++)
			{
				tmpArray [i] = blockArray [i] ;
			}
			tmpArray [i] = new T [granularity] ;
			delete [] blockArray ;
			blockArray = tmpArray ;
			nbBlock ++ ;
		}
	}
	size ++ ;
	blockArray [iBlock] [iPos] = value ;
}

template <class T>
void PaArray<T>::Delete (const guint32 index)
{
	if (index >= size)
		return ;

	guint32 iBlock = index / granularity ;
	guint32 iPos = index % granularity ;
	T* array = blockArray [iBlock] ;
	
	for (guint32 i = index ; i + 1 < size ; i ++)
	{
		if (iPos == granularity - 1)
		{
			array [iPos] = *(blockArray [++ iBlock]) ;
			array = blockArray [iBlock] ;
			iPos = 0 ;
		}
		else
		{
			array [iPos] = array [iPos + 1] ;
			iPos ++ ;
		}
	}
	size -- ;
	if (size != 0 && size % granularity == 0)
	{
		delete [] array ;
		blockArray [size / granularity] = NULL ;
	}
}

template <class T>
void PaArray<T>::Empty ()
{
	for (guint32 i = 1 ; i < nbBlock && blockArray [i] != NULL ; i++)
	{
		delete [] (blockArray [i]) ;
		blockArray [i] = NULL ;
	}
	size = 0 ;
}

template <class T>
PaArray<T> & PaArray<T>::operator = (const PaArray<T> & pa)
{
	guint32 i, j, k, l, m ;

	if (this == &pa)
		return *this ;

	if (size < pa.size)
	{
		guint32 cptBlock = pa.size > 0 ? (pa.size - 1) / granularity + 1  : 1 ;

		if (cptBlock <= nbBlock)
		{
			i = size > 0 ? (size - 1) / granularity + 1 : 1 ;
			for ( ; i < cptBlock ; i++)
			{
				blockArray [i] = new T [granularity] ;
			}
		}
		else
		{
			T** tmpBlockArray = new T* [cptBlock] ;
			guint32 ceil = size > 0 ? (size - 1) / granularity + 1 : 1 ;
			for (i = 0 ; i < ceil ; i ++)
			{
				tmpBlockArray [i] = blockArray [i] ;
			}
			for ( ; i < cptBlock ; i++)
			{
				tmpBlockArray [i] = new T [granularity] ;
			}
			delete [] blockArray ;
			blockArray = tmpBlockArray ;
			nbBlock = cptBlock ;
		}
	}
	else
	{
		if (size > pa.size)
		{
			i = pa.size > 0 ? (pa.size - 1) / granularity + 1  : 1 ;
			for ( ; i < nbBlock ; i ++)
			{
				delete [] (blockArray [i]) ;
				blockArray [i] = NULL ;
			}
		}
	}

	size = pa.size ;

	for (i = 0, j = 0, k = 0, l = 0, m = 0 ; i < size ; i++, k++, m++)
	{
		if (k >= granularity)
		{
			k = 0 ;
			j ++ ;
		}
		if (m >= pa.granularity)
		{
			m = 0 ;
			l ++ ;
		}
		blockArray [j][k] = pa.blockArray [l][m] ;
	}
	return * this ;
}

template <class T>
PaCollector<T> & PaArray<T>::operator = (const PaCollector<T> & pc)
{
	guint32 i, j, k ;
	if (this == &pc)
		return *this ;

	guint32 pcSize = pc.GetSize () ;

	if (size < pcSize)
	{
		guint32 cptBlock = pcSize > 0 ? (pcSize - 1) / granularity + 1  : 1 ;
		if (cptBlock <= nbBlock)
		{
			i = size > 0 ? (size - 1) / granularity + 1 : 1 ;
			for ( ; i < cptBlock ; i++)
			{
				blockArray [i] = new T [granularity] ;
			}
		}
		else
		{
			T** tmpBlockArray = new T* [cptBlock] ;
			guint32 ceil = size > 0 ? (size - 1) / granularity + 1 : 1 ;
			for (i = 0 ; i < ceil  ; i ++)
			{
				tmpBlockArray [i] = blockArray [i] ;
			}
			for ( ; i < cptBlock ; i++)
			{
				tmpBlockArray [i] = new T [granularity] ;
			}
			delete [] blockArray ;
			blockArray = tmpBlockArray ;
			nbBlock = cptBlock ;
		}
	}
	else
	{
		if (size > pcSize)
		{
			i = pcSize > 0 ? (pcSize - 1) / granularity + 1 : 1 ;
			for ( ; i < nbBlock ; i ++)
			{
				delete [] (blockArray [i]) ;
				blockArray [i] = NULL ;
			}
		}
	}

	size = pcSize ;

	for (i = 0, j = 0, k = 0 ; i < size ; i++, k++)
	{
		if (k >= this->granularity)
		{
			k = 0 ;
			j ++ ;
		}
		blockArray [j][k] = pc [i] ;
	}
	return * this ;
}

template <class T>
PaArray<T> PaArray<T>::operator + (const PaArray<T> & pa) const
{
	PaArray<T> result = *this ;
	if (pa.size > 0)
	{
		guint32 i, j, k, l, m ;
		guint32 blockSup = (pa.size + result.size - 1) / result.granularity + 1  ;
		if (blockSup > result.nbBlock)
		{
			blockSup -= result.nbBlock ;
			T** tmpArray = new T* [result.nbBlock + blockSup] ;
			for (i = 0 ; i < result.nbBlock ; i++)
			{
				tmpArray [i] = result.blockArray [i] ;
			}
			for (j = 0 ; j < blockSup ; i++, j++)
			{
				tmpArray [i] = new T [result.granularity] ;
			}
			result.nbBlock += blockSup ;
			delete [] result.blockArray ;
			result.blockArray = tmpArray ;
		}
		for (i = 0, j = result.size / result.granularity, k = result.size % result.granularity
			 , l = 0, m = 0 ; i < pa.size ; i++, k++, m++)
		{
			if (k >= result.granularity)
			{
				k = 0 ;
				j ++ ;
			}
			if (m >= pa.granularity)
			{
				m = 0 ;
				l ++ ;
			}
			result.blockArray [j][k] = pa.blockArray [l][m] ;
		}
		result.size += pa.size ;
	}
	return result ;
}

template <class T>
PaArray<T> PaArray<T>::operator + (const PaCollector<T> & pc) const
{
	PaArray<T> result = *this ;
	guint32 pcSize = pc.GetSize () ;
	if (pcSize > 0)
	{
		guint32 i, j, k ;
		gint32 blockSup = (pc.size + result.size - 1) / result.granularity + 1  ;
		if (blockSup > result.nbBlock)
		{
			blockSup -= result.nbBlock ;
			T** tmpArray = new T* [result.nbBlock + blockSup] ;
			for (i = 0 ; i < result.nbBlock ; i++)
			{
				tmpArray [i] = result.blockArray [i] ;
			}
			for	(j = 0 ; j < blockSup ; i++, j++)
			{
				tmpArray [i] = new T [result.granularity] ;
			}
			result.nbBlock += blockSup ;
			delete [] result.blockArray ;
			result.blockArray = tmpArray ;
		}
		for (i = 0, j = result.size / result.granularity, k = result.size % result.granularity
			 ; i < pcSize ; i++, k++)
		{
			if (k >= result.granularity)
			{
				k = 0 ;
				j ++ ;
			}
			result.blockArray [j][k] = pc [i] ;
		}
		result.size += pcSize ;
	}
	return result ;
}

template <class T>
PaArray<T> & PaArray<T>::operator += (const PaArray<T> & pa)
{
	if (pa.size > 0)
	{
		guint32 i, j, k, l, m ;
		guint32 blockSup = (pa.size + size - 1) / granularity + 1  ;
		if (blockSup > nbBlock)
		{
			blockSup -= nbBlock ;
			T** tmpArray = new T* [nbBlock + blockSup] ;
			for (i = 0 ; i < nbBlock && blockArray [i] != NULL ; i++)
			{
				tmpArray [i] = blockArray [i] ;
			}
			for ( ; i < blockSup + nbBlock ; i++)
			{
				tmpArray [i] = new T [granularity] ;
			}
			nbBlock += blockSup ;
			delete [] blockArray ;
			blockArray = tmpArray ;
		}
		for (i = 0, j = size / granularity, k = size % granularity
			 , l = 0, m = 0 ; i < pa.size ; i++, k++, m++)
		{
			if (k >= granularity)
			{
				k = 0 ;
				j ++ ;
			}
			if (m >= pa.granularity)
			{
				m = 0 ;
				l ++ ;
			}

			blockArray [j][k] = pa.blockArray [l][m] ;
		}
		size += pa.size ;
	}
	return *this ;
}

template <class T>
PaCollector<T> & PaArray<T>::operator += (const PaCollector<T> & pc)
{
	guint32 pcSize = pc.GetSize () ;
	if (pcSize > 0)
	{
		guint32 i, j, k ;
		guint32 blockSup = (pcSize + size - 1) / granularity + 1  ;
		if (blockSup > nbBlock)
		{
			blockSup -= nbBlock ;
			T** tmpArray = new T* [nbBlock + blockSup] ;
			for (i = 0 ; i < nbBlock && blockArray [i] != NULL ; i++)
			{
				tmpArray [i] = blockArray [i] ;
			}
			for ( ; i < blockSup + nbBlock; i++)
			{
				tmpArray [i] = new T [granularity] ;
			}
			nbBlock += blockSup ;
			delete [] blockArray ;
			blockArray = tmpArray ;
		}
		for (i = 0, j = size / granularity, k = size % granularity
			 ; i < pcSize ; i++, k++)
		{
			if (k >= granularity)
			{
				k = 0 ;
				j ++ ;
			}
			blockArray [j][k] = pc [i] ;
		}
		size += pcSize ;
	}
	return *this ;
}

template <class T>
bool PaArray<T>::operator == (const PaArray<T> & pa) const
{
	if (this == & pa)
		return true ;

	if (size != pa.size)
		return false ;

	for (guint32 i = 0, j = 0, k = 0, l = 0, m = 0 ; i < size ; i ++, k++, m++)
	{
		if (k >= granularity)
		{
			k = 0 ;
			j ++ ;
		}
		if (m >= pa.granularity)
		{
			m = 0 ;
			l ++ ;
		}
		if (blockArray [j][k] != pa.blockArray [l][m])
			return false ;
	}
	return true ;
}

template <class T>
inline bool PaArray<T>::operator != (const PaArray<T> & pa) const
{
	return ! operator == (pa) ;
}

template <class T>
bool PaArray<T>::operator == (const PaCollector<T> & pc) const
{
	if (this == & pc)
		return true ;

	if (size != pc.GetSize ())
		return false ;

	for (guint32 i = 0, j = 0, k = 0; i < size ; i ++, k++)
	{
		if (k >= granularity)
		{
			k = 0 ;
			j ++ ;
		}
		if (blockArray [j][k] != pc [i])
			return false ;
	}
	return true ;
}

template <class T>
inline bool PaArray<T>::operator != (const PaCollector<T> & pc) const
{
	return ! operator == (pc) ;
}

template <class T>
PaArray<T> PaArray<T>::operator () (const guint32 startPos, const guint32 endPos) const
{
	guint32 end = endPos < size ? endPos : size ;

	if (end <= startPos)
		return PaArray<T> (0, granularity) ;

	PaArray<T> result (end - startPos, granularity) ;
	for (guint32 i = startPos, j = startPos / granularity, k = startPos % granularity, l = 0, m = 0
		 ; i < end ; i ++, k++, m++)
	{
		if (k >= granularity)
		{
			k = 0 ;
			j ++ ;
		}
		if (m >= granularity)
		{
			m = 0 ;
			l ++ ;
		}
		result.blockArray [l][m] = blockArray [j][k] ;
	}
	return result ;
}

#endif // #ifndef _GENERATE_DOC

}

#endif

