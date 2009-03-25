#include "vrc6bot.h"
#include "io.h"

namespace VRC6Bot {

	/**********************************************************************************************
	 *
	 * Bank
	 *
	 **********************************************************************************************/

	Bank::Bank( const ITLoader::Bank &bank, const ConversionInput::SoundbankData &cinput ) {
		
		for( int i = 0, n = bank.modules.size(); i < n; i++ ) {
			AddModule( *(bank.modules[i]), *cinput.modules[i] );
		}
	}

	void Bank::AddModule( const ITLoader::Module &mod, const ConversionInput::ModuleData &cinput ) {
		emodules.push_back( new EModule( mod, cinput ) );

		// load samples...
		u16 sample_map[256];

		for( int i = 0; i < mod.SampleCount; i++ ) {
			ConversionInput::SampleData *cisd = 0;
			for( u32 j = 0; j < cinput.samples.size(); j++ ) {
				if( cinput.samples[j]->index == i ) {
					cisd = cinput.samples[j];
					break;
				}
			}
			Sample *s = new Sample( *(mod.Samples[i]), cisd );
			sample_map[i] = AddSample( s );
		}

		imodules.push_back( new IModule( mod, cinput, sample_map, samples ) );
	}

	int Bank::AddSample( VRC6Bot::Sample *s ) {
		for( u32 i = 0; i < samples.size(); i++ ) {
			if( samples[i]->Compare( *s ) ) {
				delete s;
				return i;
			}
		}
		SampleCount++;
		samples.push_back( s );
		return samples.size() - 1;
	}

	/**********************************************************************************************
	 *
	 * EModule
	 *
	 **********************************************************************************************/

	EModule::EModule( const ITLoader::Module &source, 
					  const ConversionInput::ModuleData &cinput ) {
		InitialVolume = source.GlobalVolume;
		InitialTempo = source.InitialTempo;
		InitialSpeed = source.InitialSpeed;
		
		int i;
		for( i = 0; i < 11; i++ )
			InitialChannelVolume[i] = source.ChannelVolume[i];
		
		for( i = 0; i < 11; i++ )
			InitialChannelPanning[i] = source.ChannelPan[i];
		
		EchoVolumeLeft = cinput.EVL;
		EchoVolumeRight = cinput.EVR;
		EchoDelay = cinput.EDL;
		EchoFeedback = cinput.EFB;
		
		for( int i = 0; i < 8; i++ ) {
			EchoFIR[i] = cinput.COEF[i];
		}

		EchoEnableBits = cinput.EON;
		
		Sequence = new u8[ source.Length ];
		for( int i = 0; i < source.Length; i++ )
			Sequence[i] = source.Orders[i];

		NumberOfPatterns = (u8)source.PatternCount;
		Patterns = new Pattern*[NumberOfPatterns];

		for( int i = 0; i < NumberOfPatterns; i++ ) {
			Patterns[i] = new Pattern( *(source.Patterns[i]) );
		}
	}
	
	EModule::~EModule() {
		if( Sequence ) {
			delete Sequence;
		}
		for( int i = 0; i < NumberOfPatterns; i++ )
			delete Patterns[i];
		delete[] Patterns;
	}

	/**********************************************************************************************
	 *
	 * Pattern
	 *
	 **********************************************************************************************/

	Pattern::Pattern( ITLoader::Pattern &source ) {
		Rows = (u8)(source.Rows-1);
		DataLength = source.DataLength;
		Data = new u8[ DataLength ];

		for( int i = 0; i < DataLength; i++ )
			Data[i] = source.Data[i];
	}
	
	Pattern::~Pattern() {
		if( Data )
			delete[] Data;
	}
	
	bool Sample::Compare( const Sample &test ) const {
		if( DataLength != test.DataLength )
			return false;
		if( Loop == test.Loop )
			return false;
		for( int i = 0; i < DataLength; i++ ) {
			if( BRRdata[i] != test.BRRdata[i] ) {
				return false;
			}
		}
		return true;
	}

	/**********************************************************************************************
	 *
	 * IModule
	 *
	 **********************************************************************************************/

	IModule::IModule(	const ITLoader::Module &source, 
						const ConversionInput::ModuleData &cinput, 
						const u16 *sample_map, 
						const std::vector<Sample*> &sample_tab ) {

		SampleCount = source.SampleCount;
		InstrumentCount = source.InstrumentCount;
		SequenceLength = (u8)source.Length;

		samples = new SampleHeader*[SampleCount];
		instruments = new Instrument*[InstrumentCount];

		for( int i = 0; i < SampleCount; i++ ) {
			samples[i] = new SampleHeader( *source.Samples[i], sample_map[i], (sample_tab[i]) );
		}

		for( int i = 0; i < InstrumentCount; i++ ) {
			instruments[i] = new Instrument( *source.Instruments[i] );
		}
	}

	IModule::~IModule() {
		for( int i = 0; i < SampleCount; i++ ) {
			delete samples[i];
		}
		delete[] samples;
		for( int i = 0; i < InstrumentCount; i++ ) {
			delete instruments[i];
		}
		delete[] instruments;
	}

	/**********************************************************************************************
	 *
	 * SampleHeader
	 *
	 **********************************************************************************************/
	
	SampleHeader::SampleHeader( const ITLoader::Sample &source, int sample_index, const Sample *ex ) {
		DefaultVolume = source.DefaultVolume;
		GlobalVolume = source.GlobalVolume;
		SetPan = source.DefaultPanning;
		
		// TODO:relnote, finetune

		SampleIndex = sample_index;
	}

	/**********************************************************************************************
	 *
	 * Instrument
	 *
	 **********************************************************************************************/

	Instrument::Instrument( const ITLoader::Instrument &source ) {
		Fadeout = source.Fadeout;
		SampleIndex = source.Notemap[60].Sample;
		GlobalVolume = source.GlobalVolume;
		SetPan = source.DefaultPan;

		venv = new InstrumentEnvelope( *source.VolumeEnvelope );
		penv = new InstrumentEnvelope( *source.PanningEnvelope );
	}

	Instrument::~Instrument() {
		delete venv;
		delete penv;
	}

	/**********************************************************************************************
	 *
	 * InstrumentEnvelope
	 *
	 **********************************************************************************************/
	
	InstrumentEnvelope::InstrumentEnvelope( const ITLoader::Envelope &source ) {
		Sustain = source.Sustain ? source.SustainStart : 0xFF;
		LoopStart = source.Loop ? source.LoopStart : 0xFF;
		LoopEnd = source.Loop ? source.LoopEnd : 0xFF;
		Length = source.Length;
		
		nodes = new EnvelopeNode[ Length ];
		
		for( int i = 0; i < Length; i++ ) {
			nodes[i].y = source.Nodes[i].y;
			if( i != Length-1 ) {
				int duration = source.Nodes[i+1].x - source.Nodes[i].x;
				nodes[i].delta = ((source.Nodes[i+1].y - source.Nodes[i].y) * 256 + duration/2) / duration;
			} else {
				nodes[i].delta = 0;
			}
		}
	}
	
	InstrumentEnvelope::~InstrumentEnvelope() {
		delete[] nodes;
	}

	/**********************************************************************************************
	 *
	 * Export...
	 *
	 **********************************************************************************************/
	
	void Bank::Export( const char *ibank, const char *ebank ) {
		IO::File file( ibank, IO::MODE_WRITE );
		
	}
};
