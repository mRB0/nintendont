#include "inputdata.h"

namespace ConversionInput {

	template<typename T> void deletePtrVector( std::vector<T*> &vecs ) {
		
		for(typename std::vector<T*>::iterator iter = vecs.begin(), ending = vecs.end();
			iter != ending; 
			iter++ ) {
		
			if( *iter )
				delete (*iter);
		}
		
		vecs.clear();
	}
	
	OperationData::OperationData( const TiXmlDocument *doc ) {
		GenerateFromXML( doc );
	}
	
	OperationData::~OperationData() {
		deletePtrVector<SoundbankData>( targets );
	}

	void OperationData::GenerateFromXML( const TiXmlDocument *doc ) {
		
		for( const TiXmlNode *child = doc->FirstChild(); 
			 child; 
			 child = child->NextSibling() ) {

			if( child->Type() == TiXmlNode::ELEMENT ) {
				if( child->ValueTStr() == "soundbank" ) {
					
					targets.push_back( new SoundbankData( child->ToElement() ) );
				}
			}
		}
	}

	SoundbankData::SoundbankData( const TiXmlElement *source ) {

		// search for attributes
		output_i = source->Attribute( "ibank" );
		output_e = source->Attribute( "ebank" );

		if( output_i.empty() || output_e.empty() ) {
			printf( "Missing ibank/ebank attributes.\n" );
			return;
		}
		
		for( const TiXmlNode *child = source->FirstChild();
			 child;
			 child = child->NextSibling() ) {
				
			if( child->Type() == TiXmlNode::ELEMENT ) {

				modules.push_back( new ModuleData(child->ToElement()) );
			}
			
		}
	}

	u8 ModuleData::ConvertBitString( const char *data ) {
		u8 value = 0;
		for( u32 i = 0; i < 8; i++ ) {
			if( !data[i] ) {
				return value;
			}
			if( data[i] == '1' ) {
				value |= (1<<i);
			}
		}
		return value;
	}

	u8 ModuleData::TranslatePercentage( int value ) {
		return (value * 127) / 100;
	}

	static const char *coef_names[] = { 
		"e0", "e1", "e2", "e3", "e4", "e5", "e6", "e7", "e8" 
	};

	ModuleData::ModuleData( const TiXmlElement *source ) {

		// set default attributes
		EDL = 0;
		EFB = 0;
		EVL = 0;
		EVR = 0;
		EON = 0;
		COEF[0] = 127;
		for( int i = 1; i < 8; i++ )
			COEF[i] = 0;

		// search for attributes
		for( const TiXmlAttribute *a = source->FirstAttribute();
			 a;
			 a = a->Next() ) {
			
			std::string name = a->Name();

			if( name == "file" ) filename = name;
			else if( name == "edl" ) EDL = a->IntValue();
			else if( name == "efb" ) EFB = TranslatePercentage(a->IntValue());
			else if( name == "evl" ) EVL = TranslatePercentage(a->IntValue());
			else if( name == "evr" ) EVR = TranslatePercentage(a->IntValue());
			else if( name == "eon" ) EON = ConvertBitString(a->Value());
			else {
	
				bool f = false;
				for( u32 i = 0; i < 8; i++ ) {
					if( name == coef_names[i] ) {
						COEF[i] = TranslatePercentage(a->IntValue());
						f = true;
						break;
					}
				}
				if( !f ) {
					printf( "Encountered unknown module attribute.\n" );
				}
			}
		}

		for( const TiXmlNode *child = source->FirstChild();
			 child;
			 child = child->NextSibling() ) {
				
			if( child->Type() == TiXmlNode::ELEMENT ) {

				if( child->ValueTStr() == "sample" ) {
					samples.push_back( new SampleData( child->ToElement() ) );
				}
			}
			
		}
	}

	ModuleData::~ModuleData() {
		deletePtrVector( samples );
	}
	
	SampleData::SampleData( const TiXmlElement *source ) {

		// set default attributes
		index = -1;
		force_filter = -1;
		force_loop_filter = -1;

		// search for attributes
		for( const TiXmlAttribute *a = source->FirstAttribute();
			 a;
			 a = a->Next() ) {
			
			std::string name = a->Name();

			if( name == "index" ) index = a->IntValue();
			else if( name == "filter" ) force_filter = a->IntValue();
			else if( name == "loopfilter" ) force_loop_filter = a->IntValue();
			else {
				printf( "Encountered unknown sample attribute.\n" );
			}
		}

		// no children
	}
}
