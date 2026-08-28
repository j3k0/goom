#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "paexception.h"
using namespace patools;

PaException::PaException (const PaExceptionDesc & c, int line, const char* f)
	: id (c.GetId ())
{
	sprintf (msg, "Error at line %d on file %s : %s\n"
	              "Comes from : %s",
		line, f, c.GetMsg (), c.GetMiscInfo ()) ;
}

PaException::PaException (const PaExceptionDesc & c)
{
	sprintf (msg, "Internal error : %s", c.GetMsg ()) ;
}

const char * PaException::what () const throw()
{
	return (const char *)msg ;
}

int PaException::GetId () const
{
	return id ;
}

/** Create the exception from a string */
PaSimpleExceptionDesc::PaSimpleExceptionDesc (const char * c)
{
	strcpy (msg, c) ;
}

/** Create the exception from an integer */
PaSimpleExceptionDesc::PaSimpleExceptionDesc (int i)
{
	sprintf (msg, "error %d", i) ;
}

const char * PaSimpleExceptionDesc::GetMsg () const
{
	return (const char *)msg ;
}

int PaSimpleExceptionDesc::GetId () const
{
	return atoi (msg) ;
}

const char * PaSimpleExceptionDesc::GetMiscInfo () const
{
	return "No more informations about the launcher..." ;
}
