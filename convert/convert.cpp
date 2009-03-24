/******************************************************************************
 * IT converter
 * by mukunda
 ******************************************************************************/
 
#include <string.h>
#include <stdio.h>

enum {
	CHTAB	=0x09
};

const char USAGE[] = {

"\n\n\nUsage: convert [OPTIONS] input.xml output\n"
"\n"
"see example.xml\n"
"output will be output.ibank (internal bank) and output.ebank (external bank)\n\n\n"

};

int main( int argc, char *argv[] ) {
	
	printf( USAGE );
	
	
	return 0;
}
