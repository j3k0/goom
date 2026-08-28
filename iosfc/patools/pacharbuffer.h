/**
* @class CharBuffer
*
* An ASCII buffer.
*
* A CharBuffer contains a string and a list of separators.
* It is used to manipulate ASCII buffers.
*
* @since 23/01/2001
*
* @author Jean-Christophe HOELT <jeko@free.fr>
* @author David GAYERIE
*
* @version 16/04/2001
*
* (c) 2000-2001, iOS software
*/

#ifndef _CHARBUFFER_H
#define _CHARBUFFER_H

#include <iostream>
#include <stdlib.h>

namespace patools {
class CharBuffer {
public:

	/// Create an empty CharBuffer with no separator
	CharBuffer () ;

	/// Create a CharBuffer which contains a copy of string and no separator
	CharBuffer (const char * string) ;

	/// Create a copy of cb (including the list of separators)
	CharBuffer (const CharBuffer &cb) ;

	/// Destructor
	~CharBuffer () ;

/**
* @name Cast methods.
* @{
*/

	/** Cast to a C string.
	* @warning Use these methods carefully !
	*   Do not alter the content of the CharBuffer with these methods.
	* @see operator const char* ()
	* @see operator char *()
	*/
	char * CharPtr () {return data ;}

	/** Cast to a C string.
	* @warning Use these methods carefully !
	*   Do not alter the content of the CharBuffer with these methods.
	* @see CharPtr()
	* @see operator char *()
	*/
	operator const char* () const {return data ;}

	/** Cast to a C string.
	* @warning Use these methods carefully !
	*   Do not alter the content of the CharBuffer with these methods.
	* @see operator const char* ()
	* @see CharPtr()
	*/
	operator char* () {return data ;}
	
	/** @} */

/**
* @name Assignement operators.
* @{
*/

	/** Copy of a CharBuffer.
	*
	* Copy a CharBuffer into the current CharBuffer.
	* @param cb The CharBuffer to copy.
	* @warning The previous list of separators is lost.
	*/
	CharBuffer & operator = (const CharBuffer &cb) ;
	
	/**
	* Copy a string into the current CharBuffer.
	*
	* @attention The list of separators is unchanged.
	*/
	CharBuffer & operator = (const char * str) ;
	
	/** @} */

/**
* @name Access methods.
* @{
*/

	/** Overloaded operator.
	* function result :
	* 	- If i < 0, the method returns a reference to the first character.
	* 	- If i > highest rank, the method returns a reference to the last character.
	* 	- If the CharBuffer is empty, it will be resize up to one element
	* 		and the method will return a reference to this single chararacter
	* 		(initialized with the null character '\0') whatever is the value of i.
	*/
	char & operator [] (const int i) ;

	/** Overloaded Operator.
	* function result :
	* 	- If i < 0, the method returns the first character.
	* 	- If i > highest rank, the method returns the last character.
	* 	- If the CharBuffer is empty, the method returns the '\0' character.
	*/
	char operator [] (const int i) const ;

	/** @} */

/**
* @name Access to a character without operator overloading
* @{
*/

	/** Char acces.
	* function result :
	* 	- If i < 0, the method returns the first character.
	* 	- If i > highest rank, the method returns the last character.
	* 	- If the CharBuffer is empty, the method returns the '\0' character.
	*/
	char GetChar (const int i) const ;

	/** Char acces.
	* function result :
	* 	- If i is an invalid index, nothing happens.
	* 	- If c is the null character '\0', nothing happens.
	*/
	void SetChar (const int i, const char c) ;

	/** @} */

	/** Size of the CharBuffer.
	* @return The size of the string (not including the ending null character '\0')
	*/
	int Length () const { return size ;}

/**
* @name Comparison operators.
* @{
*/
	/** Check equality */
	bool operator == (const CharBuffer &cb) const ;
	/** Check equality */
	bool operator == (const char * str) const ;

	/** Check equality */
	bool operator != (const CharBuffer &cb) const ;
	/** Check equality */
	bool operator != (const char * str) const ;
	
	/** @} */
	
/**
* @name Concatenation operators.
* @{
*/

	/** Concatenate two CharBuffer.
	* Concatenate the string of the current CharBuffer with cb's one.
	*
	* The returned CharBuffer inherits the list of separators from the
	* current CharBuffer and cb.
	*
	* The separators are guaranteed not to be duplicated.
	*/
	CharBuffer operator + (const CharBuffer &cb) const ;
	
	/** Concatenate two CharBuffer.
	* @see operator+()
	*/
	CharBuffer operator + (const char * str) const ;

	/** Concatenate two CharBuffer.
	* Same methods than +, without the creation of an auxiliary CharBuffer.
	*
	* @see operator+
	*/
	CharBuffer & operator += (const CharBuffer &cb) ;

	/** Concatenate two CharBuffer.
	* Same methods than +, without the creation of an auxiliary CharBuffer.
	*
	* @see operator+
	*/
	CharBuffer & operator += (const char * str) ;

	/** @} */
	
/**
* @name Sub-CharBuffer creation
* @{
*/

	/** Parial copy of a CharBuffer.
	* function result :
	* @li If imin < 0 then imin = 0.
	* @li If imax > highest rank than imax = highest rank.
	* @li If imax <= imin or the CharBuffer is empty, the method returns an empty CharBuffer.
	*
	* @attention The set of separators is inherited by the returned CharBuffer but the copy of the sub-CharBuffer
	*   does NOT take into account the occurence of any separators.
	*
	* @returns A CharBuffer which contains the characters between imin and imax indexes (imax excluded).
	*/
	CharBuffer PartialCopy (const int imin, const int imax) const ;

	/**
	* Operator overloading. Same behavior as CharBuffer::PartialCopy()
	*
	* @see PartialCopy()
	*/
	CharBuffer operator () (const int imin, const int imax) const ;

	/** Partial copy of a CharBuffer.
	* function result :
	* @li If imin < 0 then imin = 0.
	* @li If imin >= highest rank than the returned CharBuffer is empty.
	*
	* @attention The set of separators is inherited by the returned CharBuffer and the copy of the sub-CharBuffer
	*   is DELIMITED BY THE NEXT SEPARATOR OR THE END OF THE CHARBUFFER.
	*
	* @return A CharBuffer which contains the characters between imin and imax indexes (imax excluded).
	*/
	CharBuffer operator () (const int imin) const ;
	
	/** @} */

	/** Complex access method.
	* function result :
	* @li If i < 0 then the search starts at the beginning of the CharBuffer.
	* @li If i >= highest rank, the returned CharBuffer is empty and i is unchanged.
	*
	* However, the list of separators is inherited by the returned CharBuffer.
	*
	* @return a CharBuffer which contains the current word i.e. the string from the i index to the next
	* separator or the end of the CharBuffer. i is updated to the index of the next word.
	*/
	CharBuffer GetNextWord (int &i) const ;

	/** Complex access method.
	* i is updated to the index of the end of the previous word
	* (or -1 if the current word is the first word).
	*
	* function result :
	* @li If i < 0 then the returned CharBuffer is empty and i == -1.
	* @li If i >= highest rank, the search starts from the last character.
	*
	* However, the list of separators is inherited by the returned CharBuffer.
	*
	* @return a CharBuffer which contains the current word i.e. the string from 
	* the previous separator or the beginning of the CharBuffer to the i index.
	*
	* @see GetNextWord()
	*/
	CharBuffer GetPreviousWord (int &i) const ;

/**
* @name Search methods
* @{
*/

	/** Find the next word.
	* function result :
	* @li If i < 0 then the search starts at the beginning of the CharBuffer.
	* @li If i >= highest rank, the method returns the highest rank.
	*
	* @return The index of the next word i.e. the index of the first character between i and
	* the end of the CharBuffer which is not a separator.
	*/
	int GetNextWordIndex (const int i = 0) const ;

	/** Find the next separator.
	* function result :
	* @li If i < 0 then the search starts at the beginning of the CharBuffer.
	* @li If i >= highest rank or no separator has been found, the method returns the highest rank.
	*
	* @return The index of the next separator between i and the end of the CharBuffer.
	*/
	int GetNextSeparatorIndex (const int i = 0) const ;

	/** Find a word in the CharBuffer.
	* function result :
	* @li If i < 0 then the search starts at the beginning of the CharBuffer.
	* @li If i >= highest rank, the method returns the highest rank.
	*
	* @warning THE SEPARATORS ARE IGNORED DURING THE SEARCH.
	*
	* @return The index of the first occurence of word between i and the end
	* of the CharBuffer.
	*/
	int FindWordIndex (const char* word, const int i = 0) const ;

	/** Find a word in the CharBuffer.
	* @see FindWordIndex
	*/
	int FindWordIndex (const CharBuffer & cb, const int i = 0) const ;
	
	/** @} */

/**
* @name Miscellaneous methods
* @{
*/

	/** 
	* Replace all occurences of separators in the string by the c character.
	*
	* if c == '\0' then nothing happens.
	*/
	void Replace (const char c) ;

	/** Transform the CharBuffer.
	* Transform any lowercase unstressed alphabetic character of the string to
	* uppercase character
	*/
	void ToUpper () ;

	/** Transform the CharBuffer.
	* Transform any uppercase unstressed alphabetic character of the string to
	* lowercase character
	*/
	void ToLower () ;

	/** Check if a valid number is present.
	* @retval true if the sub-string beginning at the index position is a valid representation of a number.
	* @retval false if not.
	*/
	bool IsNumeric (const int index = 0) const ;
	
	/** @} */

/**
* @name Numeric Conversions
* All these methods try to convert the string from the beginnning or
* the index th position to a numeric value until a non numeric character
* is found.
* @{
*/

	/** Cast to double.
	* If the string cannot be converted, the methods return 0.
	* @attention The numeric conversions do not use the separators.
	*
	* <b>Supported format for double and float cast :</b>
	* [' ']+[+ | -][0-9]+[[.][0-9]+][{d | D | e | E}[+ | -][0-9]+]
	*/
	double DoubleValue (const int index = 0) const ;
	/** Cast to double.
	* @see DoubleValue
	*/
	operator double () const ;

	/** Cast to float.
	* @see DoubleValue
	*/
	float FloatValue (const int index = 0) const ;
	/** Cast to float.
	* @see DoubleValue
	* @see FloatValue
	*/
	operator float () const ;

	/** Cast to int.
	*
	* <b>Supported format for int cast :</b>
	* [' ']+{[+ | -][0-9]+ | {'0x' | '0X'}[0-9 | a-f | A-F]+}
	*/
	int IntValue (const int index = 0) const ;

	/** Cast to int.
	* @see IntValue
	*/
	operator int () const ;
	
	/** @} */

/**
* @name I/O methods
* @{
*/

	friend std::ostream&	operator << (std::ostream& os, const CharBuffer& cb) 
	{
		if (cb.size != 0)
			return os << cb.data ;
		else
			return os ;
	}
	
	/** @} */

/**
* @name Separators related methods.
* @{
*/

	/** Check if a char is a separator.
	* @warning The separators are stored in a character array
	*
	* There is not a null character '\0' at the end of the enumeration.
	*
	* Besides, the null character '\0' is not regarded as a
	* valid separator. Any attempt to load it as a separator
	* will be ignored.
	*
	* @attention The separators are guaranteed not to be duplicated.
	*
	* @return true if c is a member of the set of separators
	*/
	bool IsASeparator (const char c) const ;

	/** Get the separators list.
	* If charArray is not NULL, fill this array with an unordered list of separators.
	*
	* @attention Do not make any assumption about the order of the separators.
	* @return The number of separators.
	*/
	int GetSeparator (char* charArray = NULL) const ;
	
	/// Add c to the list of separators.
	void LoadSeparator (const char c) ;

	/// Add the nbChar first characters of charArray to the list of separators.
	void LoadSeparator (const char* charArray, const int nbChar) ;

	/**
	* Add the list of separators of cb to the list of separators of the
	* current CharBuffer.
	*/
	void LoadSeparator (const CharBuffer & cb) ;

	/// Remove c from the list of separators.
	void DeleteSeparator (const char c) ;

	/** Delete some separators.
	*
	* Remove the nbChar first characters of charArray from the list of
	* separators.
	*
	* @see DeleteSeparator
	*/
	void DeleteSeparator (const char* charArray, const int nbChar) ;

	/** Delete some separators.
	*
	* Remove the list of separators of cb from the list of separators of the
	* current CharBuffer.
	*
	* @see DeleteSeparator
	*/
	void DeleteSeparator (const CharBuffer & cb) ;

	/// Remove all separators.
	void DeleteAllSeparator () ;
	
	/** @} */

private:
	char * data ;
	int size ;
	char * separator ;
	int nbSeparator ;
	int separatorStorage ;
	static int separatorStorageGranularity ;
} ;
}

#endif // _CHARBUFFER_H

