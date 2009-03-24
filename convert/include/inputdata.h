#ifndef __INPUTDATA_H
#define __INPUTDATA_H

// classes to hold results of xml parsing

#include <stdlib.h>
#include <string>
#include <vector>
#include "tinyxml.h"
#include "basetypes.h"

namespace ConversionInput {

	class SampleData {

	public:
		int index;
		int force_filter;
		int force_loop_filter;
	
		SampleData( const TiXmlElement * );
	};

	class ModuleData {

	private:
		u8 ConvertBitString( const char * );
		u8 TranslatePercentage( int );
		
	public:

		ModuleData( const TiXmlElement *source );
		~ModuleData();
		std::string filename;
		
		u8	EDL;
		u8	EFB;
		u8	EVL;
		u8	EVR;
		u8	EON;
		u8	COEF[8];
		
		std::vector<SampleData*> samples;
	};

	class SoundbankData {

	public:

		SoundbankData( const TiXmlElement *source );

		std::string output_i;
		std::string output_e;

		std::vector<ModuleData*> modules;
	};

	class OperationData {

	private:
		void GenerateFromXML( const TiXmlDocument *doc );

	public:

		~OperationData();

		OperationData( const TiXmlDocument *doc );

		std::vector<SoundbankData*> targets;
	};
}

#endif
