/// @todo verifier que la lecture s'effectue toujours dans le buffer

#include "pasequentialdatabloc.h"

#include <iostream>
using namespace std;

#include <stdio.h> /* manip de fichier */

using namespace patools;

/***************************
* constructeur de BaseBloc *
***************************/

PaSequentialDataBloc * PaSequentialDataBloc::CreateBaseBloc ()
{
	PaSequentialDataBloc * bloc ;
	bloc = new PaSequentialDataBloc ("ROOT") ;

	return bloc ;
}


/***************
* constructeur *
***************/

PaSequentialDataBloc::PaSequentialDataBloc (const CharBuffer & longname)
	:	label (), longLabel (longname), datas (0, 500), readPos (0),
		subBlocs (0, 10)
{
	longLabel.DeleteAllSeparator () ;
	longLabel.LoadSeparator ('/') ;
	
	gint32 i = longLabel.Length () ;
	label = longLabel.GetPreviousWord (i) ;
}


/******************************
* chargement d'un ancien PSDB *
******************************/

void PaSequentialDataBloc::LoadBloc (
	PaSequentialDataBloc * bloc, const guint32 pos)
{
	// sauvegarde de la position d'origine
	guint32 oldpos = this->readPos ;
	this->Seek (pos) ;

	guint32 nb_elem = this->ReadUInt32 () ;
	guint32 nb_child = this->ReadUInt32 () ;
    
    if (nb_child > 0xffffff || nb_elem > 0xffffff) {
        // Most probably a corrupted file...
        this->Seek (oldpos) ;
        return;
    }

	for (guint32 i = 0 ; i < nb_child ; i ++)
	{
		CharBuffer subname = this->ReadCharBuffer (this->ReadUInt32 ()) ;
		bloc->CreateSubBloc (subname) ;
		this->LoadBloc (bloc->SubBloc (subname), this->ReadUInt32 ()) ;
	}
	
	for (guint32 j = 0 ; j < nb_elem ; j ++)
	{
		bloc->Write ((guint8)this->ReadUInt8 ()) ;
	}

	// retour a la position d'origine
	this->Seek (oldpos) ;
}

PaSequentialDataBloc * PaSequentialDataBloc::CreateBaseBloc (
	const PaArray <guint8> & data)
{
	PaSequentialDataBloc * root ;
	PaSequentialDataBloc initial ("LOADER") ;

	initial.datas = data ;

	root = CreateBaseBloc () ;
	initial.LoadBloc (root, 0) ;

	return root ;
}


PaSequentialDataBloc * PaSequentialDataBloc::LoadBaseBloc (
	const char * fileName)
	throw (PaException)
{
	PaArray <guint8> pa ;
	FILE * f = fopen (fileName, "r") ;
	
	if (f == NULL)
	{
		PATHROW (PSDBException (NO_SUCH_FILE)) ;
	}

	int c ;

	while ((c = fgetc (f)) != EOF)
	{
		pa.Add ((guint8)c) ;
	}
	
	fclose (f) ;
	
	return CreateBaseBloc (pa) ;
}


/**
* Finalisation d'une structure & enregistrement
*/

PaArray <guint8> PaSequentialDataBloc::Finalize ()
{
	PaSequentialDataBloc final ("FINALIZER") ;
	final.WriteBloc (*this) ;
	return final.datas ;
}

guint32 PaSequentialDataBloc::FinalTotalBlocSize () const
{
	guint32 ret = (8 + this->subBlocs.GetSize () * 8 + this->SelfSize ()) ;
	for (guint32 i = 0 ; i < this->subBlocs.GetSize () ; i ++)
	{
		ret += this->subBlocs [i]->Label ().Length () + 1 ;
		ret += this->subBlocs [i]->FinalTotalBlocSize () ;
	}
	return ret ;
}

void PaSequentialDataBloc::WriteBloc (const PaSequentialDataBloc & psb)
{
	guint32 endpos =
		this->SelfSize ()
		+ (8 + psb.subBlocs.GetSize () * 8 + psb.SelfSize ()) ;

	this->Write (psb.SelfSize ()) ;
	this->Write (psb.subBlocs.GetSize ()) ;

	for (guint32 i = 0 ; i < psb.subBlocs.GetSize () ; i ++)
	{
		this->Write (endpos) ;
		endpos += psb.subBlocs [i]->Label ().Length () + 1 ;
		this->Write (endpos) ;
		endpos += psb.subBlocs [i]->FinalTotalBlocSize () ;
	}
	
	for (guint32 j = 0 ; j < psb.datas.GetSize () ; j ++)
	{
		this->Write ((guint8)psb.datas [j]) ;
	}
	
	for (guint32 k = 0 ; k < psb.subBlocs.GetSize () ; k ++)
	{
		CharBuffer cb = psb.subBlocs [k]->Label () ;
		this->Write (cb.CharPtr ()) ;
		this->WriteBloc (*(psb.subBlocs [k])) ;
	}
}

void PaSequentialDataBloc::FinalizeAndWriteToDisk (const char * fileName)
	throw (PaException)
{
	PaArray <guint8> pa = this->Finalize () ;
	FILE * f = fopen (fileName, "w") ;
	if (f == NULL)
	{
		PATHROW (PSDBException (COULD_NOT_CREATE_FILE)) ;
	}

	for (guint32 i = 0 ; i < pa.GetSize () ; i++)
	{
		fputc (pa [i], f) ;
	}
	
	fclose (f) ;
}


PaSequentialDataBloc * PaSequentialDataBloc::CreateSubBloc (
	const CharBuffer & name) throw (PaException)
{
	// est-ce qu'un bloc de meme label existe deja ?
	for (guint32 i = 0 ; i < this->subBlocs.GetSize () ; i ++)
	{
		if (this->subBlocs [i]->label == name)
			PATHROW (PSDBException (EXISTING_SUBBLOC)) ;
	}

	// non, creation du sous-bloc
	PaSequentialDataBloc * sb =
		new PaSequentialDataBloc (this->longLabel + "/" + name) ;
	this->subBlocs.Add (sb) ;

	return sb ;
}


PaSequentialDataBloc * PaSequentialDataBloc::SubBloc (
	const CharBuffer & name) throw (PaException)
{
	CharBuffer child = name ;
	child.DeleteAllSeparator () ;
	child.LoadSeparator ('/') ;
	
	CharBuffer subchild ;
	PaSequentialDataBloc * psdb = this ;
	
	gint32 cpos = 0 ;
	bool fini = false ;

	while (!fini)
	{
		subchild = child.GetNextWord (cpos) ;
		fini = (cpos >= child.Length ()) ;
		
		bool founded = false ;

		for (guint32 i = 0 ;
			i < psdb->subBlocs.GetSize () && !founded ;
			i ++)
		{
			if (psdb->subBlocs [i]->label == subchild)
			{
				psdb = psdb->subBlocs [i] ;
				founded = true ;
			}
		}

		if (!founded)
			PATHROW (PSDBException (NO_SUCH_BLOC)) ;
	}
	return psdb ;
}

bool PaSequentialDataBloc::HasSubBloc (const CharBuffer & name)
{
	CharBuffer child = name ;
	child.DeleteAllSeparator () ;
	child.LoadSeparator ('/') ;
	
	CharBuffer subchild ;
	PaSequentialDataBloc * psdb = this ;
	
	gint32 cpos = 0 ;
	bool fini = false ;
    
	while (!fini)
	{
		subchild = child.GetNextWord (cpos) ;
		fini = (cpos >= child.Length ()) ;
		
		bool founded = false ;
        
		for (guint32 i = 0 ;
             i < psdb->subBlocs.GetSize () && !founded ;
             i ++)
		{
			if (psdb->subBlocs [i]->label == subchild)
			{
				psdb = psdb->subBlocs [i] ;
				founded = true ;
			}
		}
        
		if (!founded)
			return false ;
	}
	return true ;
}

/***************
* SIZE & LABEL *
***************/


guint32 PaSequentialDataBloc::SelfSize () const
{
	return this->datas.GetSize () ;
}


guint32 PaSequentialDataBloc::TotalSize () const
{
	guint32 totsize = this->SelfSize () ;
	for (guint32 i = 0 ; i < this->subBlocs.GetSize () ; i ++)
	{
		totsize += this->subBlocs [i]->TotalSize () ;
	}
	return totsize ;
}

const CharBuffer & PaSequentialDataBloc::Label () const
{
	return this->label ;
}

const CharBuffer & PaSequentialDataBloc::LongLabel () const
{
	return this->longLabel ;
}


/***************
* READ & WRITE *
***************/


void PaSequentialDataBloc::Write (const char * s)
{
	guint32 i = 0 ;
	while (s[i])
	{
		this->Write ((guint8) s[i]) ;
		i ++ ;
	}
	this->Write ((guint8) 0) ;
}

void PaSequentialDataBloc::Write (const CharBuffer & cb)
{
	// VC++ BUG CharBuffer::operator [] prend un int comme parametre et non pas un guint32
	//guint32 i = 0 ;
	int i = 0 ;
	while (cb[i])
	{
		this->Write ((guint8) cb[i]) ;
		i ++ ;
	}
	this->Write ((guint8) 0) ;
}

void PaSequentialDataBloc::Write (const guint32 i)
{
	this->Write ((guint8) (i >> 24)) ;
	this->Write ((guint8) (i >> 16)) ;
	this->Write ((guint8) (i >> 8)) ;
	this->Write ((guint8) i) ;
}

void PaSequentialDataBloc::Write (const gint32 i)
{
	this->Write ((guint8) (i >> 24)) ;
	this->Write ((guint8) (i >> 16)) ;
	this->Write ((guint8) (i >> 8)) ;
	this->Write ((guint8) i) ;
}

void PaSequentialDataBloc::Write (const guint16 i)
{
	this->Write ((guint8) (i >> 8)) ;
	this->Write ((guint8) i) ;
}

void PaSequentialDataBloc::Write (const gint16 i)
{
	this->Write ((guint8) (i >> 8)) ;
	this->Write ((guint8) i) ;
}

void PaSequentialDataBloc::Write (const guint8 i)
{
	this->datas.Add (i) ;
}

void PaSequentialDataBloc::Write (const gint8 i)
{
	this->datas.Add ((guint8)i) ;
}

void PaSequentialDataBloc::Write (const gfloat f)
{
	guint32 *i = (guint32 *) (& f) ;
	Write(*i);
	/*
	guint8 *i = (guint8 *) (& f) ;
	this->Write (*(i++)) ;
	this->Write (*(i++)) ;
	this->Write (*(i++)) ;
	this->Write (*(i)) ;
	*/
}

/* void PaSequentialDataBloc::Write (const gdouble f)
{
	guint8 *i = (guint8 *) (& f) ;
	this->Write (*(i++)) ;
	this->Write (*(i++)) ;
	this->Write (*(i++)) ;
	this->Write (*(i++)) ;
	this->Write (*(i++)) ;
	this->Write (*(i++)) ;
	this->Write (*(i++)) ;
	this->Write (*(i)) ;
}
*/

CharBuffer PaSequentialDataBloc::ReadCharBuffer () const
{
	guint8 c ;
	guint32 oldpos = readPos ;
	guint32 size = 0 ;

    if (readPos >= datas.GetSize()) {
        PATHROW(PSDBException (OUT_OF_BOUNDS));
        return CharBuffer();
    }

	while ((c = this->ReadUInt8 ()) != '\0')
	{
		size ++ ;
	}
	
	this->Seek (oldpos) ;
	// ANSI C++ BUG : une tableau doit etre declare avec une expression constante
	/*
		char s [size + 1] ;
		for (guint32 i = 0 ; i < size ; i ++)
		{
			s [i] = this->ReadUInt8 () ;
		}
		s [size] = '\0' ;

		return CharBuffer (s) ;
	*/
	char* s = new char [size + 1] ;
	for (guint32 i = 0 ; i < size ; i ++)
	{
		s [i] = this->ReadUInt8 () ;
	}
	s [size] = this->ReadUInt8() ; // '\0'
	CharBuffer result (s) ;
	delete [] s ;
	return result ;
}

guint32 PaSequentialDataBloc::ReadUInt32 () const
{
	guint32 c1,c2,c3,c4 ;
	c1 = (guint32) this->ReadUInt8 () ;
	c2 = (guint32) this->ReadUInt8 () ;
	c3 = (guint32) this->ReadUInt8 () ;
	c4 = (guint32) this->ReadUInt8 () ;
	c4 |= c1 << 24 ;
	c4 |= c2 << 16 ;
	c4 |= c3 << 8 ;
	return c4 ;
}

gint32 PaSequentialDataBloc::ReadInt32 () const
{
	guint32 c1,c2,c3 ;
	gint32 c4 ;
	c1 = this->ReadUInt8 () ;
	c2 = this->ReadUInt8 () ;
	c3 = this->ReadUInt8 () ;
	c4 = this->ReadUInt8 () ;
	c4 |= c1 << 24 ;
	c4 |= c2 << 16 ;
	c4 |= c3 << 8 ;
	return c4 ;
}

guint16 PaSequentialDataBloc::ReadUInt16 () const
{
	guint16 c1,c2 ;
	c1 = this->ReadUInt8 () ;
	c2 = this->ReadUInt8 () | (c1 << 8) ;
	return c2 ;
}

gint16 PaSequentialDataBloc::ReadInt16 () const
{
	guint16 c1 ;
	gint16 c2 ;
	c1 = this->ReadUInt8 () ;
	c2 = this->ReadUInt8 () | (c1 << 8) ;
	return c2 ;
}

guint8 PaSequentialDataBloc::ReadUInt8 () const
{
	return this->datas [this->readPos++] ;
}

gint8 PaSequentialDataBloc::ReadInt8 () const
{
	return (gint8) this->datas [this->readPos++] ;
}

gfloat PaSequentialDataBloc::ReadFloat () const
{
	// gfloat ret ;
	// guint8 *i = (guint8 *) (& ret) ;
    gint32 iret = this->ReadUInt32();
	// *(i++) = this->ReadUInt8 () ;
	// *(i++) = this->ReadUInt8 () ;
	// *(i++) = this->ReadUInt8 () ;
	// *(i) = this->ReadUInt8 () ;
	return *(gfloat*)&iret;
}

/*
gdouble PaSequentialDataBloc::ReadDouble () const
{
	gdouble ret ;
	guint8 *i = (guint8 *) (& ret) ;
    
	*(i++) = this->ReadUInt8 () ;
	*(i++) = this->ReadUInt8 () ;
	*(i++) = this->ReadUInt8 () ;
	*(i++) = this->ReadUInt8 () ;
	*(i++) = this->ReadUInt8 () ;
	*(i++) = this->ReadUInt8 () ;
	*(i++) = this->ReadUInt8 () ;
	*(i) = this->ReadUInt8 () ;
	
	return ret ;
}
*/

/** goto position i */
void PaSequentialDataBloc::Seek (const guint32 i) const
{
	readPos = i ;
}


CharBuffer PaSequentialDataBloc::ReadCharBuffer (const guint32 i) const
{
	guint32 old = this->readPos ;
	Seek (i) ;
	CharBuffer c = ReadCharBuffer () ;
	Seek (old) ;
	return c ;
}

guint32 PaSequentialDataBloc::ReadUInt32 (const guint32 i) const
{
	guint32 old = this->readPos ;
	Seek (i) ;
	guint32 c = ReadUInt32 () ;
	Seek (old) ;
	return c ;
}

gint32 PaSequentialDataBloc::ReadInt32 (const guint32 i) const
{
	guint32 old = this->readPos ;
	Seek (i) ;
	gint32 c = ReadInt32 () ;
	Seek (old) ;
	return c ;
}

guint16 PaSequentialDataBloc::ReadUInt16 (const guint32 i) const
{
	guint32 old = this->readPos ;
	Seek (i) ;
	guint16 c = ReadUInt16 () ;
	Seek (old) ;
	return c ;
}

gint16 PaSequentialDataBloc::ReadInt16 (const guint32 i) const
{
	guint32 old = this->readPos ;
	Seek (i) ;
	gint16 c = ReadInt16 () ;
	Seek (old) ;
	return c ;
}

guint8 PaSequentialDataBloc::ReadUInt8 (const guint32 i) const
{
	guint32 old = this->readPos ;
	Seek (i) ;
	guint8 c = ReadUInt8 () ;
	Seek (old) ;
	return c ;
}

gint8 PaSequentialDataBloc::ReadInt8 (const guint32 i) const
{
	guint32 old = this->readPos ;
	Seek (i) ;
	gint8 c = ReadInt8 () ;
	Seek (old) ;
	return c ;
}

gfloat PaSequentialDataBloc::ReadFloat (const guint32 i) const
{
	guint32 old = this->readPos ;
	Seek (i) ;
	gfloat c = ReadFloat () ;
	Seek (old) ;
	return c ;
}

/*
gdouble PaSequentialDataBloc::ReadDouble (const guint32 i) const
{
	guint32 old = this->readPos ;
	Seek (i) ;
	gdouble c = ReadDouble () ;
	Seek (old) ;
	return c ;
}
*/

/*************
* destructor *
*************/

void PaSequentialDataBloc::Free (PaSequentialDataBloc * & psb) throw (PaException)
{
	if (psb->longLabel == psb->label)
	{
		delete psb ;
		psb = NULL ;
	}
	else
	{
		cout << psb->label << endl ;
		cout << psb->longLabel << endl ;
		PATHROW (PSDBException (NOT_A_ROOT_BLOC)) ;
	}
}

PaSequentialDataBloc::~PaSequentialDataBloc ()
{
	for (guint32 i = 0 ; i < this->subBlocs.GetSize () ; i ++)
	{
		delete this->subBlocs [i] ;
	}
}

PaSequentialDataBloc::PSDBException::PSDBException (ExceptionId e)
{
	id = (int) e ;
}

const char * PaSequentialDataBloc::PSDBException::GetMsg () const
{
	static const char * Msg [] = {
			"NO SUCH FILE",
			"EXISTING SUBBLOC",
			"COULD NOT CREATE FILE",
			"NO SUCH BLOC",
			"NOT A ROOT BLOC",
			"END OF BLOC"
	} ;
	
	return Msg [id] ;
}

int PaSequentialDataBloc::PSDBException::GetId () const
{
	return id ;
}

const char * PaSequentialDataBloc::PSDBException::GetMiscInfo () const
{
	return
		"PaSequentialDataBloc. Member of the PaTools.\n"
		"Created on May 2001 by Jean-Christophe Hoelt <jeko@free.fr>\n"
		"(c)2001 by iOS software.\n" ;
}
