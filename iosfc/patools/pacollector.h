#ifndef _PACOLLECTOR
#define _PACOLLECTOR
#include "patypes.h"

/**
@class PaCollector

the basic model for a lot of our data representation

@author iOS software <ios@free.fr>
@version 05/2001
*/

template <class T>
class PaCollector
{
public:
	/// The destructor.
	virtual ~PaCollector () {} ;

	/** Access method.
	* @return The number of elements in the collector.
	*/
	virtual guint32 GetSize () const = 0 ;
	
	/** Access method.
	* @param i The index of the desired element.
	* @return An element of the collector.
	*/
	virtual const T & Get (const guint32 i) const = 0 ;

	/** Write method.
	* Add a new element at the end of the collector.
	* @param t The element to add to the collector.
	*/
	virtual void Add (const T & t) = 0 ;

	/** Write method.
	* Set a new value to an element of the collector.
	* @param i Index of the element to change.
	* @param newValue The new value of this element.
	*/
	virtual void Set (const guint32 i, const T & newValue) = 0 ;

	/** Write method.
	* @param i Index of the element that will be inserted.
	* @param value The value of the element.
	*/
	virtual void Insert (const guint32 i, const T & value) = 0 ;
	
	/** Destroy method.
	* Delete an element of the collector.
	* @param i The index of the element.
	*/
	virtual void Delete (const guint32 i) = 0 ;

	/** Destroy method.
	* Delete all elements of the collector.
	*/
	virtual void Empty () = 0 ;

	/** overloaded operator.
	* Same as Get (i).
	* @see Get()
	*/
	virtual const T & operator [] (const guint32 i) const = 0 ;

	/** overloaded operator.
	* Same as Set (i).
	* @see Set()
	*/
	virtual T & operator [] (const guint32 i) = 0 ;

	/// Duplicate a collector.
	virtual PaCollector & operator = (const PaCollector<T> &) = 0 ;
	/// Concatenate two collectors.
	virtual PaCollector & operator += (const PaCollector<T> &) = 0 ;
	/// Compare two collector.
	virtual bool operator == (const PaCollector<T> &) const = 0 ;
	/// Compare two collector.
	virtual bool operator != (const PaCollector<T> &) const = 0 ;
} ;

#endif // _PACOLLECTOR
