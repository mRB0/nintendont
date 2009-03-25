/******************************************************************************
 * super IT converter
 * by mukunda
 ******************************************************************************/
 
#include <string>
#include <stdio.h>
#include "tinyxml.h"
#include "inputdata.h"
#include "itloader.h"
#include "vrc6bot.h"

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
	
	// as easy as:
	// 1:
	ConversionInput::OperationData data( &doc );
	
	// 2:
	ITLoader::Bank bank( data.targets[0] );
		
	// 3:
	VRC6Bot::Bank result( bank );

	result.Export( data.targets[0]->output_i.c_str(), data.targets[0]->output_e.c_str() );

	return 0;
}
