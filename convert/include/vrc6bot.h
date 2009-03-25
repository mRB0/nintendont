#ifndef VRC6BOT_H
#define VRC6BOT_H

#include "itloader.h"

namespace VRC6Bot {
	
	class Sample {
		
	};
	
	class Instrument {
		
	};
	
	class SampleHeader {
	
	public:
		u8	DefaultVolume;
		u8	GlobalVolume;
		u8	SetPan;
		u8	RelativeNote;
		u8	Finetune;
		u16	SampleIndex;	// ignored for vrc6 samples
	};
	
	class IModule {
		u16	SampleCount;
		u16 InstrumentCount;
		u8 SequenceLength;

		SampleHeader **samples;

		// sample pointers..
		// instrument pointers..
	};
	
	class EModule {
	};

	class IBank {
		u16	SampleCount;

	};

	class EBank {
		
	};

	class Bank {
		
	public:
		Bank( const ITLoader::Bank & );

		void Export( const char *ibank, const char *ebank );
	};
};

#endif
