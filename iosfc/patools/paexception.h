/**
* @file paexception.h
*
* @author Jean-Christophe Hoelt <jeko@free.fr>
* @since 24/05/2001
*
* content :
* 	a set of class to manipulate exceptions...
*	and a usefull macro to throw exceptions.
*
* copyright 2001, iOS software <ios@free.fr>
*/

#ifndef _PATHROW_H
#define _PATHROW_H

#include <exception>

/** throw an exception */
#if _DEBUG
#define PATHROW(s) \
	throw PaException (s, __LINE__, __FILE__)
#else
#define PATHROW(s) \
	throw PaException (s)
#endif

#define PASIMPLETHROW(s) \
	PATHROW(PaSimpleExceptionDesc(s))

namespace patools {

/**
* Abstract class used to describe an exception.
*
* This abstract class is used to get the description
* of an exception.
*
* If you want to throw exceptions using PATHROW, you must
* implement your own PaExceptionDesc, to inform catchers
* of the problem.
*
* @see PaException
* @see PATHROW
* @see PaSimpleExceptionDesc
*/
class PaExceptionDesc
{
public:
	/** return a short description of the exception */
	virtual const char * GetMsg () const = 0 ;
	
	/** return an integer identifying the exception */
	virtual int GetId () const = 0 ;
	
	/** return some informations about the exception launcher */
	virtual const char * GetMiscInfo () const = 0 ;
} ;

/**
* A simple PaExceptionDesc.
*/
class PaSimpleExceptionDesc : public PaExceptionDesc
{
public:
	/** Create the exception from a string */
	PaSimpleExceptionDesc (const char * c) ;
	/** Create the exception from an integer */
	PaSimpleExceptionDesc (int i) ;

	virtual const char * GetMsg () const ;
	virtual int GetId () const ;
	virtual const char * GetMiscInfo () const ;
private:
	char msg [512] ;
};


/**
* Hi-level exception usage.
*/
class PaException : public std::exception
{
public:
	/** generation of the message */
	PaException (const PaExceptionDesc & e, int line, const char * f) ;
	PaException (const PaExceptionDesc & c) ;
	virtual const char * what () const throw();
	int GetId () const ;

private:
	char msg [512] ;
	int id ;
} ;

}

#endif
