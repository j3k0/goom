/*******************************************************************************
 * file : charbuffer.cpp
 * author : JC HOELT <jeko@free.fr> & D GAYERIE
 * birth : 23/01/2001
 * revision : 16/04/2001
 *
 * content : the CharBuffer class, used to manipulate ASCII buffers
 * 
 * (c) 2000-2001, iOS software
 ******************************************************************************/

#include "pacharbuffer.h"

using namespace patools;

int CharBuffer::separatorStorageGranularity = 10 ;

// Constructors

CharBuffer::CharBuffer ()
        : data (NULL), size (0), separator (NULL), nbSeparator (0), separatorStorage (0)
{
}

CharBuffer::CharBuffer (const char * initialValue)
        : data (NULL), size (0), separator (NULL), nbSeparator (0), separatorStorage (0)
{
	if (initialValue != NULL)
	{
		for (; initialValue [size] != '\0' ; size ++) ;

		if (size > 0)
		{
			data = new char [size + 1] ;
	
			for (int i = 0 ; i <= size ; i++)
			{
				data [i] = initialValue [i] ;
			}
		}
	}
}

CharBuffer::CharBuffer (const CharBuffer &cb)
{
	size = cb.size ;
	nbSeparator = cb.nbSeparator ;

	if (size > 0)
	{
		data = new char [size + 1] ;
		for (int i = 0 ; i <= size ; i++) 
		{
			data [i] = cb.data [i] ;
		}
	}
	else data = NULL ;

	if (nbSeparator > 0)
	{
		separatorStorage = (nbSeparator / separatorStorageGranularity + 1) * separatorStorageGranularity ;
		separator = new char [separatorStorage] ;
		for (int j = 0 ; j < nbSeparator ; j++)
		{
			separator [j] = cb.separator [j] ;
		}
	}
	else
	{
		separator = NULL ;
		separatorStorage = 0 ;
	}
}

// Assignment methods

CharBuffer & CharBuffer::operator = (const CharBuffer &cb)
{
	if (this != &cb)
	{
		if (size != cb.size)
		{
			delete [] data ;
			size = cb.size ;
			if (size > 0)
			{
				data = new char [size + 1] ;
			}
			else 
				data = NULL ;
		}
        if (data != NULL) {
            for (int i = 0 ; i <= size ; i++)
            {
                data [i] = cb.data [i] ;
            }
        }

		nbSeparator = cb.nbSeparator ;

		if (separatorStorage < nbSeparator)
		{
			delete [] separator ;
			separatorStorage = (nbSeparator / separatorStorageGranularity + 1) * separatorStorageGranularity ;
			separator = new char [separatorStorage] ;
		}
		for (int j = 0 ; j < nbSeparator ; j++)
		{
			separator [j] = cb.separator [j] ;
		}
	}
	return *this ;
}

CharBuffer & CharBuffer::operator = (const char * str)
{
	if (str == NULL)
	{
		size = 0 ;
		delete [] data ;
		data = NULL ;
		return *this ;
	}

	if (data != str)
	{
		int strSize = 0 ;
		while (str [strSize++] != '\0') ;
		
		if (strSize == 1)
		{
			size = 0 ;
			delete [] data ;
			data = NULL ;
			return *this ;
		}
		else
		{
			if (size + 1 != strSize)
			{
				delete [] data ;
				data = new char [strSize] ;
			}
			size = strSize - 1 ;

			for (int i = 0 ; i < strSize ; i++)
			{
				data [i] = str [i] ;
			}
		}
	}
	return * this ;
}

// Destructor

CharBuffer::~CharBuffer ()
{
	delete [] data ;
	delete [] separator ;
}

// Access methods

char & CharBuffer::operator [] (const int i)
{
	if (i >= 0 && i < size)
		return data [i] ;
	else
	{
		if (size == 0)
		{
			size = 1 ;
			data = new char [2] ;
			data [0] = data [1] = '\0' ;
			return data [0] ;
		}
		else
		{
			if (i < 0)
				return data [0] ;
			else
				return data [size - 1] ;
		}
	}
}

char CharBuffer::operator [] (const int i) const 
{
	if (i >= 0 && i < size)
		return data [i] ;
	else
	{
		if (size == 0)
			return '\0' ;
		else
		{
			if (i < 0)
				return data [0] ;
			else
				return data [size - 1] ;
		}
	}
}

char CharBuffer::GetChar (const int i) const
{
	if ( i >= 0 && i < size)
		return data [i] ;
	else
	{
		if (size == 0)
			return '\0' ;
		else
		{
			if (i < 0)
				return data [0] ;
			else
				return data [size - 1] ;
		}
	}
}

void CharBuffer::SetChar (const int i, const char c)
{
	if (i >= 0 && i < size && c != '\0')
		data [i] = c ;
}

// comparison operators

bool CharBuffer::operator == (const CharBuffer &cb) const
{
	if (size != cb.size) return false ;

	for (int i = 0; i < size; i++)
	{
		if (data [i] != cb.data [i]) return false ;
	}
	return true ;
}

bool CharBuffer::operator == (const char* str) const
{
	if (data == str) return true ;

	if (str != NULL && data != NULL)
	{
		int i ;
		for (i = 0 ; i < size && str [i] != '\0' ; i ++)
		{
			if (data [i] != str [i]) return false ;
		}
		return data [i] == str [i] ; // if both are equal, it must be '\0'
	}
	else
		return false ;
}

bool CharBuffer::operator != (const CharBuffer &cb) const
{
	return !(*this==cb) ;
}

bool CharBuffer::operator != (const char * str) const
{
	return ! this->operator == (str) ;
}

// Concatenation operators

CharBuffer CharBuffer::operator + (const CharBuffer &cb) const
{
	CharBuffer c ;
	if (size != 0 || cb.size != 0)
	{
		c.size = size + cb.size ;
		c.data = new char [c.size + 1] ;

		int i ;
		for (i = 0; i < size ; i++)
		{
			c.data [i] =  data [i] ;
		}
		for (int j = 0 ; j <= cb.size ; i++, j++)
		{
			c.data [i] = cb.data [j] ;
		}
	}

	c.LoadSeparator (separator, nbSeparator) ;
	c.LoadSeparator (cb.separator, cb.nbSeparator) ;
	return c ;
}

CharBuffer CharBuffer::operator + (const char* str) const
{
	if (str == NULL)
	{
		return *this ;
	}
		
	CharBuffer c ;
	int strSize ;
	for (strSize = 0 ; str [strSize] != '\0' ; strSize ++) ;

	if (size != 0 || strSize != 0)
	{
		c.size = size + strSize ;
		c.data = new char [c.size + 1] ;

		int i ;
		for (i = 0; i < size ; i++)
		{
			c.data [i] =  data [i] ;
		}
		for (int j = 0 ; j <= strSize ; i++, j++)
		{
			c.data [i] = str [j] ;
		}
	}

	c.LoadSeparator (separator, nbSeparator) ;
	return c ;
}

CharBuffer & CharBuffer::operator += (const CharBuffer& cb)
{
	if (cb.size != 0)
	{
		char * tmpArray = new char [size + cb.size + 1] ;
		int i ;
		for (i = 0 ; i < size ; i++)
		{
			tmpArray [i] = data [i] ;
		}
		for (int j = 0 ; j <= cb.size ; i++, j++)
		{
			tmpArray [i] = cb.data [j] ;
		}
		size += cb.size ;
		delete [] data ;
		data = tmpArray ;
	}
	LoadSeparator (cb.separator, cb.nbSeparator) ;
	return * this ;
}

CharBuffer & CharBuffer::operator += (const char* str)
{
	if (str != NULL)
	{
		int strSize ;
		for (strSize = 0 ; str [strSize] != '\0' ; strSize ++) ;

		if (strSize > 0)
		{
			char * tmpArray = new char [size + strSize + 1] ;
			int i ;
			for (i = 0 ; i < size ; i++)
			{
				tmpArray [i] = data [i] ;
			}
			for (int j = 0 ; j <= strSize ; i++, j++)
			{
				tmpArray [i] = str [j] ;
			}
			size += strSize ;
			delete [] data ;
			data = tmpArray ;
		}
	}
	return *this ;
}

// Sub-CharBuffer creation

CharBuffer CharBuffer::PartialCopy (const int imin, const int imax) const
{
	return this->operator () (imin, imax) ;
}

CharBuffer CharBuffer::operator () (const int imin, const int imax) const
{
	CharBuffer cb ;

	if (imin < imax && imin >= 0 && imin < size && imax <= size)
	{
		int i, j ;
		cb.size = imax - imin ;
		cb.data = new char [cb.size + 1] ;
		for (i = imin, j = 0 ; i < imax ; i++, j++)
		{
			cb.data [j] = data [i] ;
		}
		cb.data [j] = '\0' ;
	}
	cb.LoadSeparator (separator, nbSeparator) ;
	return cb ;
}

CharBuffer CharBuffer::operator () (const int imin) const
{
	CharBuffer cb ;
	if (imin >= 0 && imin < size)
	{
		if (nbSeparator == 0)
		{
			cb = data + imin ;
		}
		else
		{
			int imax ;
			for (imax = imin ; imax < size && ! IsASeparator (data [imax]) ; imax ++) ;
			cb.size = imax - imin ;
			if (cb.size != 0)
			{
				int i, j ;
				cb.data = new char [cb.size + 1] ;
				for (i = imin, j = 0 ; i < imax ; i++, j++)
				{
					cb.data [j] = data [i] ;
				}
				cb.data [j] = '\0' ;
			}
		}
	}
	cb.LoadSeparator (separator, nbSeparator) ;
	return cb ;
}

CharBuffer CharBuffer::GetNextWord (int & i) const
{
	CharBuffer cb = this->operator () (i) ;
	for (i += cb.size ; i < size && IsASeparator (data [i]) ; i ++) ;
	return cb ;
}

CharBuffer CharBuffer::GetPreviousWord (int & i) const
{
	if (i >= size) i = size - 1 ;

	int iLast = i + 1 ;
	int iFirst = i > -1 ? i : -1 ;
	for ( ; iFirst >= 0 && ! IsASeparator (data [iFirst]) ; iFirst --) ;
	
	for (i = iFirst ; i > 0 && IsASeparator (data [i]) ; i--) ;

	return this->operator () (iFirst + 1, iLast) ;
}

// Search methods

int CharBuffer::GetNextWordIndex (const int i /* = 0 */) const
{
	if (i >= size)
		return size ;

	int index = i > 0 ? i : 0 ;
	for ( ; index < size && IsASeparator (data [index]) ; index ++) ;
	return index ;
}

int CharBuffer::GetNextSeparatorIndex (const int i /* = 0 */) const
{
	if (i >= size)
		return size ;

	int index = i > 0 ? i : 0 ;
	for ( ; index < size && ! IsASeparator (data [index]) ; index ++) ;
	return index ;
}

int CharBuffer::FindWordIndex (const char* word, const int i /* = 0 */) const
{
	if (word == NULL)
		return 0 ;
	
	if (i >= size)
		return size ;

	int index  = i > 0 ? i : 0 ;
	for ( ; index < size ; index ++)
	{
		if (data [index] == word [0])
		{
			int j ;
			int k = index + 1 ;
			for (j = 1 ; word [j] != '\0' && data [k] == word [j] ; j++, k++) ;
			if (word [j] == '\0')
				return index ;
		}
	}
	return size ;
}

int CharBuffer::FindWordIndex (const CharBuffer & cb, const int i /* = 0 */) const
{
	if (cb.size == 0)
		return 0 ;

	if (i >= size)
		return size ;

	int index  = i > 0 ? i : 0 ;
	for ( ; index < size ; index ++)
	{
		if (data [index] == cb.data [0])
		{
			int j ;
			int k = index + 1 ;
			for (j = 1 ; j < cb.size && data [k] == cb.data [j] ; j++, k++) ;
			if (cb.data [j] == '\0')
				return index ;
		}
	}
	return size ;
}

// Miscellaneous methods

void CharBuffer::Replace (const char c)
{
	if (c != '\0')
	{
		for (int i = 0 ; i < size ; i ++)
		{
			if (IsASeparator (data [i]))
			{
				data [i] = c ;
			}
		}
	}
}

void CharBuffer::ToUpper ()
{
	for (int i = 0 ; i < size ; i++)
	{
		if (data [i] >= 'a' && data [i] <= 'z')
		{
			data [i] -= 0x20 ;
		}
	}
}

void CharBuffer::ToLower ()
{
	for (int i = 0 ; i < size ; i++)
	{
		if (data [i] >= 'A' && data [i] <= 'Z')
		{
			data [i] += 0x20 ;
		}
	}
}

bool CharBuffer::IsNumeric (const int index /* = 0 */) const
{
	int i = index > 0 ? index : 0 ;

	for (; i < size && data [i] == ' ' ; i++) ;
	
	if (i < size && (data [i] == '+' || data [i] == '-')) i ++ ;
	if (i < size && data [i] == '.') i ++ ;

	return (i < size && data [i] >= '0' && data [i] <= '9') ;
}

// Numeric Conversions

float CharBuffer::FloatValue (const int index /* = 0 */) const
{
	int i = index > 0 ? index : 0 ;
	if (i < size)
		return (float) atof (data + i) ;
	else
		return 0.0 ;
}

CharBuffer::operator float () const
{
	if (data == NULL)
		return 0 ;
	else
		return (float) atof (data) ;
}

double CharBuffer::DoubleValue (const int index /* = 0 */) const
{
	int i = index > 0 ? index : 0 ;
	if (i < size)
		return atof (data + i) ;
	else
		return 0.0 ;
}

CharBuffer::operator double () const
{
	if (data == NULL)
		return 0 ;
	else
		return atof (data) ;
}

CharBuffer::operator int () const
{
	if (data == NULL)
		return 0 ;

	int i = 0 ;

	for (; i < size && data [i] == ' ' ; i++) ;

	if (i < size - 2 && data [i] == '0' && (data [i + 1] == 'x' || data [i + 1] == 'X'))
	{
		i += 2 ;
		int result = 0 ;
		int cpt = sizeof (int) * 2 ;
		for ( ; i < size && cpt > 0 ; cpt -- , i ++)
		{
			if (data [i] >= '0' && data [i] <= '9')
			{
				result <<= 4 ;
				result |= data [i] - '0' ;
				continue ;
			}
			if (data [i] >= 'a' && data [i] <= 'f')
			{
				result <<= 4 ;
				result |= data [i] - 'a' + 10 ;
				continue ;
			}
			if (data [i] >= 'A' && data [i] <= 'F')
			{
				result <<= 4 ;
				result |= data [i] - 'A' + 10 ;
				continue ;
			}
			break ;
		}
		return result ;
	}
	else
	{
		return atoi (data) ;
	}
}

int CharBuffer::IntValue (const int index /* = 0 */) const
{
	if (data == NULL)
		return 0 ;

	int i = index > 0 ? index : 0 ;

	for (; i < size && data [i] == ' ' ; i++) ;

	if (i < size - 2 && data [i] == '0' && (data [i + 1] == 'x' || data [i + 1] == 'X'))
	{
		i += 2 ;
		int result = 0 ;
		int cpt = sizeof (int) * 2 ;
		for ( ; i < size && cpt > 0 ; cpt -- , i ++)
		{
			if (data [i] >= '0' && data [i] <= '9')
			{
				result <<= 4 ;
				result |= data [i] - '0' ;
				continue ;
			}
			if (data [i] >= 'a' && data [i] <= 'f')
			{
				result <<= 4 ;
				result |= data [i] - 'a' + 10 ;
				continue ;
			}
			if (data [i] >= 'A' && data [i] <= 'F')
			{
				result <<= 4 ;
				result |= data [i] - 'A' + 10 ;
				continue ;
			}
			break ;
		}
		return result ;
	}
	else
	{
		return atoi (data + i) ;
	}
}

// Separators related methods

bool CharBuffer::IsASeparator (const char c) const
{
	for (int i = 0 ; i < nbSeparator ; i++)
	{
		if (separator [i] == c)
			return true ;
	}
	return false ;
}

int CharBuffer::GetSeparator (char* charArray /* = NULL */) const
{
	if (charArray != NULL)
	{
		for (int i = 0 ; i < nbSeparator ; i++)
		{
			charArray [i] = separator [i] ;
		}
	}
	return nbSeparator ;
}

void CharBuffer::LoadSeparator (const char c)
{
	if (c != '\0' && ! IsASeparator (c))
	{
		if (separatorStorage < nbSeparator + 1)
		{
			separatorStorage += separatorStorageGranularity ;
			char * tmpArray = new char [separatorStorage] ;
			for (int i = 0 ; i < nbSeparator ; i++)
			{
				tmpArray [i] = separator [i] ;
			}
			delete [] separator ;
			separator = tmpArray ;
		}
		separator [nbSeparator ++] = c ;
	}
}

void CharBuffer::LoadSeparator (const char* charArray, const int nbChar)
{
	if (nbChar > 0 && charArray != NULL)
	{
		for (int i = 0 ; i < nbChar ; i++)
		{
			LoadSeparator (charArray [i]) ;
		}
	}
}

void CharBuffer::LoadSeparator (const CharBuffer & cb)
{
	for (int i = 0 ; i < cb.nbSeparator ; i++)
	{
		LoadSeparator (cb.separator [i]) ;
	}
}

void CharBuffer::DeleteSeparator (const char c)
{
	for (int i = 0 ; i < nbSeparator ; i++)
	{
		if (separator [i] == c)
		{
			nbSeparator -- ;
			if (nbSeparator != i)
			{
				separator [i] = separator [nbSeparator] ;
			}
			return ;
		}
	}
}

void CharBuffer::DeleteSeparator (const char* charArray, const int nbChar)
{
	if (nbChar > 0 && charArray != NULL)
	{
		for (int i = 0 ; i < nbChar ; i++)
		{
			DeleteSeparator (charArray [i]) ;
		}
	}
}

void CharBuffer::DeleteSeparator (const CharBuffer & cb)
{
	if (separator == cb.separator)
	{
		DeleteAllSeparator () ;
	}
	else
	{
		if (cb.nbSeparator != 0)
		{
			for (int i = 0 ; i < nbSeparator ; i++)
			{
				DeleteSeparator (separator [i]) ;
			}
		}
	}
}

void CharBuffer::DeleteAllSeparator ()
{
	nbSeparator = 0 ;
}
