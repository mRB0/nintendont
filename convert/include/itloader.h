#ifndef ITLOADER_H
#define ITLOADER_H

#include "inputdata.h"
#include "io.h"

namespace ITLoader {

	class Pattern {

	public:
		Pattern( IO::File &file );
		~Pattern();

		u16 DataLength;
		u16 Rows;
		u8 *Data;
	};

	class Sample {

		void LoadData( IO::File &f );
	public:
		Sample( IO::File &f );
		~Sample();

		u8 GlobalVolume;

		bool HasSample;
		bool Bits16;
		bool Stereo;
		bool Compressed;
		bool Loop;
		bool Sustain;
		bool BidiLoop;
		bool BidiSustain;

		u8 DefaultVolume;
		u8 DefaultPanning;

		u8 Convert;
		
		int Length;
		int LoopStart;
		int LoopEnd;
		int C5Speed;
		int SustainStart;
		int SustainEnd;

		u8 VibratoSpeed;
		u8 VibratoDepth;
		u8 VibratoForm;
		u8 VibratoRate;

		union {
			s8* Data8;
			s16* Data16;
		};
	};

	typedef struct {
		u8	Note;
		u8	Sample;
	} NotemapEntry;

	typedef struct {
		u8	y;
		u16	x;
	} EnvelopeEntry;
	
	class Envelope {

	public:
		Envelope( IO::File & );
		bool Enabled;
		bool Loop;
		bool Sustain;
		bool IsFilter;
		
		int Length;
		
		int LoopStart;
		int LoopEnd;
		int SustainStart;
		int SustainEnd;
		
		EnvelopeEntry Nodes[25];
	};
	
	class Instrument {

	public:
		Instrument( IO::File & );
		~Instrument();

		u8	NewNoteAction;
		u8	DuplicateCheckType;
		u8	DuplicateCheckAction;

		u16	Fadeout;
		u8	PPS;
		u8	PPC;
		u8	GlobalVolume;
		u8	DefaultPan;
		u8	RandomVolume;
		u8	RandomPanning;
		u8	TrackerVersion;
		u8	NumberOfSamples;
		u8	InitialFilterCutoff;
		u8	InitialFilterResonance;

		u8	MidiChannel;
		u8	MidiProgram;
		u16 MidiBank;

		NotemapEntry Notemap[ 120 ];

		Envelope *VolumeEnvelope;
		Envelope *PanningEnvelope;
		Envelope *PitchEnvelope;
	};

	class Module {
		
	public:
		Module( const char *filename );
		~Module();
		
		char Title[26];
		
		u16 PatternHighlight;
		u16 Length;
		u16 InstrumentCount;
		u16 SampleCount;
		u16 PatternCount;
		u16 Cwtv;
		u16 Cmwt;
		int Flags;
		int Special;
		u8 GlobalVolume;
		u8 MixingVolume;
		u8 InitialSpeed;
		u8 InitialTempo;
		u8 Sep;
		u8 PWD;

		u16 MessageLength;
		char *Message;

		int ChannelPan[64];
		int ChannelVolume[64];

		int Orders[256];

		Instrument **Instruments;
		Sample **Samples;
		Pattern **Patterns;
	};
	
	// conversion data handler
	class Bank {
		
	public:
		Bank( const ConversionInput::SoundbankData * );
		~Bank();
		void AddModule( const char *filename );

		std::vector<Module*> modules;
	};
};

#endif
