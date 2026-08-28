#ifndef PA_STACK
#define PA_STACK

#include "paarray.h"
namespace patools {

template <class T>
class PaStack : private PaArray<T>
{
public :
	PaStack (const guint32 frame = 10) ;
	PaStack (const PaStack<T> & stack) ;
	~PaStack () ;
	PaStack<T> & operator = (const PaStack<T> & stack) ;

	bool operator == (const PaStack<T> & stack) const ;
	bool operator != (const PaStack<T> & stack) const ;

	void Empty () ;
	bool IsEmpty () const ;
	guint32 GetDepth () const ;
	T & GetTop () ;
	const T & GetTop () const ;
	void Push (const T & element) ;
	void Pop () ;
} ;

#ifndef _GENERATE_DOC

template <class T>
PaStack<T>::PaStack (const guint32 frame) : PaArray<T> (0, frame)
{
}

template <class T>
PaStack<T>::PaStack (const PaStack<T> & stack) : PaArray<T> (stack)
{
}

template <class T>
PaStack<T>::~PaStack ()
{
}

template <class T>
inline PaStack<T> & PaStack<T>::operator = (const PaStack<T> & stack)
{
	PaArray<T>::operator = (stack) ;
	return *this ;
}


template <class T>
inline bool PaStack<T>::operator == (const PaStack<T> & stack) const
{
	return PaArray<T>::operator == (stack) ;
}

template <class T>
inline bool PaStack<T>::operator != (const PaStack<T> & stack) const
{
	return PaArray<T>::operator != (stack) ;
}

template <class T>
inline void PaStack<T>::Empty ()
{
	PaArray<T>::Empty () ;
}

template <class T>
inline bool PaStack<T>::IsEmpty () const
{
	return PaArray<T>::GetSize () == 0 ;
}

template <class T>
inline guint32 PaStack<T>::GetDepth () const 
{
	return PaArray<T>::GetSize () ;
}

template <class T>
inline T& PaStack<T>::GetTop ()
{
	return operator [] (PaArray<T>::GetSize () - 1) ;
}

template <class T>
inline const T& PaStack<T>::GetTop () const
{
	return operator [] (PaArray<T>::GetSize - 1) ;
}

template <class T>
inline void PaStack<T>::Push (const T & element)
{
	Add (element) ;
}

template <class T>
inline void PaStack<T>::Pop ()
{
	Delete (PaArray<T>::GetSize () - 1) ;
}

#endif // #ifndef _GENERATE_DOC
}

#endif

