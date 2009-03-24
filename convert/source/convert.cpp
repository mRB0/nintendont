/******************************************************************************
 * IT converter
 * by mukunda
 ******************************************************************************/
 
#include <string>
#include <stdio.h>
#include "tinyxml.h"
#include "inputdata.h"

enum {
	CHTAB	=0x09
};

const char USAGE[] = {

"\n\n\nUsage: convert input.xml\n"
"\n"
"see example.xml\n"

};

int main( int argc, char *argv[] ) {
	
	if( argc < 3 )
		printf( USAGE );
	
	TiXmlDocument doc( argv[1] );
	doc.LoadFile();
	
	ConversionInput::OperationData data( &doc );
	
		
	return 0;
}
