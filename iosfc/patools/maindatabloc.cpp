#include "pasequentialdatabloc.h"
#include <iostream>
using namespace std;
using namespace patools;

void display (const PaSequentialDataBloc * psdb) ;

int main ()
{
	try {
	// creation du bloc racine
	PaSequentialDataBloc * root = PaSequentialDataBloc::CreateBaseBloc () ;

	// test d'ecritures
	root->Write ((guint32)123456789) ;
	root->Write ((gint32)-666) ;
	root->Write ("Chaine 1") ;
	root->Write ("Chaine 2") ;
	root->Write ((gfloat)-25.0f) ;

	// creation de sous-blocs
	root->CreateSubBloc ("INFORMATIONS") ;
	root->SubBloc ("INFORMATIONS")->CreateSubBloc ("NAME") ;
	root->SubBloc ("INFORMATIONS")->CreateSubBloc ("PRENOM") ;

	// ecritures de donnés dans les sous-blocs
	root->SubBloc ("INFORMATIONS/NAME")->Write ("Henri Jones Junior") ;
	root->SubBloc ("INFORMATIONS/PRENOM")->Write ("bob dylan") ;

	// affichage d'info sur les blocs
	display (root) ;
	display (root->SubBloc ("INFORMATIONS")) ;
	display (root->SubBloc ("INFORMATIONS/NAME")) ; // notation allegee
	// ~ display (root->SubBloc ("INFORMATION")->SubBloc ("NAME")) ;
	display (root->SubBloc ("INFORMATIONS/PRENOM")) ;

	// tests de lecture
	cout << endl << root->ReadUInt32 () << endl ;
	cout << root->ReadInt32 () << endl ;
	cout << root->ReadCharBuffer() << endl ;
	cout << root->ReadCharBuffer() << endl ;
	cout << root->ReadFloat () << endl ;

	cout << "\nFinalisation\n" ;
	root->FinalizeAndWriteToDisk ("test.psd") ;
	
	PaSequentialDataBloc::Free (root) ;
	
	cout << "\nloading..\n" ;
	PaSequentialDataBloc * root2 =
		PaSequentialDataBloc::LoadBaseBloc ("test.psd") ;
		
	cout << "taille des blocs\n" ;

	display (root2) ;
	display (root2->SubBloc ("INFORMATIONS")) ;
	display (root2->SubBloc ("INFORMATIONS/NAME")) ; // notation allegee
	// ~ display (root->SubBloc ("INFORMATION")->SubBloc ("NAME")) ;
	display (root2->SubBloc ("INFORMATIONS/PRENOM")) ;

	cout << "tests de lecture\n" ;
	
	cout << endl << root2->ReadUInt32 () << endl ;
	cout << root2->ReadInt32 () << endl ;
    cout << root2->SubBloc("INFORMATIONS/NAME")->ReadCharBuffer() << endl;
	cout << root2->ReadCharBuffer() << endl ;
	cout << root2->ReadCharBuffer() << endl ;
	cout << root2->ReadFloat () << endl ;
	
//	cout << root2->SubBloc ("CA VA PLANTER")->ReadFloat () << endl ;

	PaSequentialDataBloc::Free (root2) ;

	}
	catch (PaException s)
	{
		cout << s.what () << endl ;
	}

	return 0 ;
}

void display (const PaSequentialDataBloc * psdb)
{
	cout << psdb->LongLabel () << endl ;
	cout << "Total size : " << psdb->TotalSize () << " bytes\n" ;
	cout << "Self size  : " << psdb->SelfSize () << " bytes\n" ;
}
