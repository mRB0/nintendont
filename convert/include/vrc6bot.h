#ifndef VRC6BOT_H
#define VRC6BOT_H

#include "itloader.h"

namespace VRC6Bot {

	typedef struct { // IBANK

		u8	y;
		u8	duration;
		s16 delta;
	} EnvelopeNode;

	class InstrumentEnvelope { // IBANK

	private:
		u8	Sustain;
		u8	LoopStart;
		u8	LoopEnd;
		u8	Length;

		EnvelopeNode *nodes;

	public:
		InstrumentEnvelope( const ITLoader::Envelope & );
		~InstrumentEnvelope();

		void ExportAttributes( IO::File &file );
		void ExportNodes( IO::File &file );
	};
	
	class Instrument { // IBANK

	private:
		u16	Fadeout;
		u8	SampleIndex;
		u8	GlobalVolume;
		u8	SetPan;
		
		InstrumentEnvelope *venv;
		InstrumentEnvelope *penv;

	public:
		Instrument( const ITLoader::Instrument &source );
		~Instrument();

		void Export( IO::File &file );
	};

	class Sample { // EBANK

	private:
		u16 DataLength;
		u16 Loop;
		u8	*BRRdata;

		double TuningFactor;

	public:

		Sample( const ITLoader::Sample &, const ConversionInput::SampleData * );
		~Sample();

		bool Compare( const Sample& ) const;
		void Export( IO::File & ) const;
		int GetDataLength() const {
			return DataLength;
		}

		double GetTuningFactor() const {
			return TuningFactor;
		}
	};
	
	class SampleHeader { // IBANK
	
	private:
		u8	DefaultVolume;
		u8	GlobalVolume;
		u8	SetPan;
		s16	Pitch_Base;
		u16	SampleIndex;	// ignored for vrc6 samples
		
	public:
		SampleHeader( const ITLoader::Sample &, int, const Sample * );
		u16 GetSampleIndex() const {
			return SampleIndex;
		}
		
		void Export( IO::File &file );
	};

	class Pattern { // EBANK

	private:
		u16	DataLength;
		u8	Rows;
		u8	*Data;

	public:
		Pattern( ITLoader::Pattern & );
		~Pattern();

		void Export( IO::File &file );
	};
	
	class IModule { // IBANK

	private:
		u16	SampleCount;
		u16 InstrumentCount;
		u8 SequenceLength;

		SampleHeader **samples;
		Instrument **instruments;

	public:
		IModule( const ITLoader::Module &source, 
				const ConversionInput::ModuleData &cinput, 
				const u16 *sample_map, 
				const std::vector<Sample*> &sample_tab );

		~IModule();

		void Export( IO::File &file );
		void PrintSampleReport( int prefix, const std::vector<Sample*> &sources ) const;

		int GetSequenceLength() const;
	};
	
	class EModule { // EBANK

	private:
		u8	InitialVolume;
		u8	InitialTempo;
		u8	InitialSpeed;
		u8	InitialChannelVolume[11];
		u8	InitialChannelPanning[11];
		
		s8	EchoVolumeLeft;
		s8	EchoVolumeRight;
		u8	EchoDelay;
		s8	EchoFeedback;
		s8	EchoFIR[8];
		u8	EchoEnableBits;

		u8	*Sequence;

		u8	NumberOfPatterns;
		Pattern **Patterns;

	public:
		EModule( const ITLoader::Module &source, const ConversionInput::ModuleData & );
		~EModule();

		void Export( IO::File &file, int SequenceLength );
	};

	class Bank {

		void ExportI( const char *filename, u16 *ModulePointers );
		void ExportE( const char *filename, u16 *IModulePointers );
		
	public:

		Bank( const ITLoader::Bank &, const ConversionInput::SoundbankData & );
		
		void Export( const char *ibank, const char *ebank );

		void AddModule( const ITLoader::Module &, const ConversionInput::ModuleData & );
		int AddSample( VRC6Bot::Sample *s );
		
		// INTERNAL BANK
		u16	SampleCount;
		std::vector<IModule*> imodules;

		// EXTERNAL BANK
		std::vector<Sample*> samples;
		std::vector<EModule*> emodules;
	};
};

#endif
