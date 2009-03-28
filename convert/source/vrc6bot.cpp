#include "vrc6bot.h"
#include "io.h"
#include "math.h"

namespace VRC6Bot {

	/**********************************************************************************************
	 *
	 * Bank
	 *
	 **********************************************************************************************/

	Bank::Bank( const ITLoader::Bank &bank, const ConversionInput::SoundbankData &cinput ) {
		SampleCount = 0;
		for( int i = 0, n = bank.modules.size(); i < n; i++ ) {
			AddModule( *(bank.modules[i]), *cinput.modules[i] );
		}
	}

	void Bank::AddModule( const ITLoader::Module &mod, const ConversionInput::ModuleData &cinput ) {
		emodules.push_back( new EModule( mod, cinput ) );

		// load samples...
		u16 sample_map[256];

		for( int i = 9; i < mod.SampleCount; i++ ) {
			ConversionInput::SampleData *cisd = 0;
			for( u32 j = 0; j < cinput.samples.size(); j++ ) {
				if( cinput.samples[j]->index == (i+1) ) {
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

	void EModule::Export( IO::File &file, int SequenceLength ) {
		file.Write8( InitialVolume );
		file.Write8( InitialTempo );
		file.Write8( InitialSpeed );
		
		for( int i = 0; i < 11; i++ )
			file.Write8( InitialChannelVolume[i] );

		for( int i = 0; i < 11; i++ )
			file.Write8( InitialChannelPanning[i] );

		file.Write8( EchoVolumeLeft );
		file.Write8( EchoVolumeRight );
		file.Write8( EchoDelay );
		file.Write8( EchoFeedback );
		
		for( int i = 0; i < 8; i++ )
			file.Write8( EchoFIR[i] );

		file.Write8( EchoEnableBits );
		file.Write8( NumberOfPatterns );

		for( int i = 0; i < SequenceLength; i++ )
			file.Write8( Sequence[i] );

		for( int i = 0; i < NumberOfPatterns; i++ ) {
			Patterns[i]->Export( file );
		}
	}

	/**********************************************************************************************
	 *
	 * Pattern
	 *
	 **********************************************************************************************/
	
	Pattern::Pattern( ITLoader::Pattern &source ) {
		Rows = (u8)(source.Rows-1);
		DataLength = source.DataLength;

		// our data will be <rows> bytes larger
		Data = new u8[ DataLength + source.Rows ];

		int row;
		u8* read = source.Data;
		u8* write = Data;

		u8	row_buffer[256];
		int	rowsize;

		u8	spc_hints;

		u8	p_maskvar[8];

		for( row = 0; row < source.Rows-1; row++ ) {
			rowsize = 0;
			spc_hints = 0;
			u8 chvar = *read++;
			row_buffer[rowsize++] = chvar;
			
			if( chvar == 0 ) {
				*write++ = spc_hints;
				for( int i = 0; i < rowsize; i++ )
					*write++ = row_buffer[i];
				continue;
			}
			
			int channel = (chvar - 1) & 63;
			u8 maskvar;
			if( chvar & 128 ) {
				maskvar = *read++;
				maskvar |= maskvar<<4;
				p_maskvar[channel] = maskvar;
				row_buffer[rowsize++] = maskvar;
			} else {
				maskvar = p_maskvar[channel];
			}

			if( maskvar & 1 ) {		// note
				row_buffer[rowsize++] = *read++;
			}
			if( maskvar & 2 ) {		// instr
				row_buffer[rowsize++] = *read++;
			}	
			if( maskvar & 4 ) {		// vcmd
				row_buffer[rowsize++] = *read++;
			}

			u8 cmd,param;
			if( maskvar & 8 ) {		// cmd+param

				row_buffer[rowsize++] = cmd = *read++;
				row_buffer[rowsize++] = param = *read++;
			}

			if( (channel > 2) && (maskvar & 16) ) {
				spc_hints |= 1<<(channel-3);
				if( maskvar & 8 ) {
					if( cmd == 7 ) { // glissando
						spc_hints &= ~(1<<(channel-3));
					} else if( cmd == 19 ) { // Sxx
						if( (param & 0xF0) == 0xD0 ) { // note delay
							spc_hints &= ~(1<<(channel-3));
						}
					}
				}
			}
		}

	}
	
	Pattern::~Pattern() {
		if( Data )
			delete[] Data;
	}
	
	void Pattern::Export( IO::File &file ) {
		file.Write16( DataLength );
		file.Write8( Rows );
		file.Write8( 0 );
		for( int i = 0; i < DataLength; i++ )
			file.Write8( Data[ i ] );
	}
	
	/**********************************************************************************************
	 *
	 * Sample
	 *
	 **********************************************************************************************/

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

	void Sample::Export( IO::File &file ) const {
		file.Write16( DataLength );
		file.Write16( Loop );
		for( int i = 0; i < DataLength; i++ )
			file.Write8( BRRdata[i] );
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
			samples[i] = new SampleHeader( *(source.Samples[i]), sample_map[i], i < 9 ? 0 : (sample_tab[i-9]) );
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

	void IModule::Export( IO::File &file ) {

		u32 Origin = file.Tell();
		file.Write16( SampleCount );
		file.Write16( InstrumentCount );
		file.Write8( SequenceLength );

		file.Write8( 0xFF );

		u32 Tables = file.Tell();
		
		file.Skip( 2 * SampleCount );
		file.Skip( 2 * InstrumentCount );

		u16 InstrumentPointers[256];
		u16 SamplePointers[256];
		
		for( int i = 0; i < InstrumentCount; i++ ) {
			InstrumentPointers[i] = file.Tell() - Origin;
			instruments[i]->Export( file );
		}

		for( int i = 0; i < SampleCount; i++ ) {
			SamplePointers[i] = file.Tell() - Origin;
			samples[i]->Export( file );
		}

		u32 End = file.Tell();
		file.Seek( Tables );

		for( int i = 0; i < InstrumentCount; i++ )
			file.Write16( InstrumentPointers[i] );
		
		for( int i = 0; i < SampleCount; i++ )
			file.Write16( SamplePointers[i] );

		file.Seek( End );
	}

	int IModule::GetSequenceLength() const {
		return SequenceLength;
	}

	/**********************************************************************************************
	 *
	 * SampleHeader
	 *
	 **********************************************************************************************/
	
	SampleHeader::SampleHeader( const ITLoader::Sample &source, int sample_index, const Sample *ex ) {
		DefaultVolume = source.DefaultVolume;
		GlobalVolume = source.GlobalVolume;
		SetPan = source.DefaultPanning ^ 128;
		
		double a = ((double)source.C5Speed * (ex ? ex->GetTuningFactor() : 1.0));
		Pitch_Base = (int)floor(log(a / 8363.0) / log(2.0) * 768.0 + 0.5);
		
		SampleIndex = sample_index;
	}

	void SampleHeader::Export( IO::File &file ) {
		file.Write8( DefaultVolume );
		file.Write8( GlobalVolume );
		file.Write16( Pitch_Base );
		file.Write16( SampleIndex );
		file.Write8( SetPan );
	}

	/**********************************************************************************************
	 *
	 * Instrument
	 *
	 **********************************************************************************************/

	Instrument::Instrument( const ITLoader::Instrument &source ) {
		Fadeout = source.Fadeout;
		SampleIndex = source.Notemap[60].Sample - 1;
		GlobalVolume = source.GlobalVolume;
		SetPan = source.DefaultPan;

		venv = new InstrumentEnvelope( *source.VolumeEnvelope );
		penv = new InstrumentEnvelope( *source.PanningEnvelope );
	}

	Instrument::~Instrument() {
		delete venv;
		delete penv;
	}

	void Instrument::Export( IO::File &file ) {
		file.Write16( Fadeout );
		file.Write8( SampleIndex );
		file.Write8( 0 );
		file.Write8( GlobalVolume );
		file.Write8( SetPan );
		venv->ExportAttributes( file );
		penv->ExportAttributes( file );
		venv->ExportNodes( file );
		penv->ExportNodes( file );
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

	void InstrumentEnvelope::ExportAttributes( IO::File &file ) {
		file.Write8( Length );
		file.Write8( Sustain );
		file.Write8( LoopStart );
		file.Write8( LoopEnd );
	}

	void InstrumentEnvelope::ExportNodes( IO::File &file ) {
		
		for( int i = 0; i < Length; i++ ) {
			file.Write8( nodes[i].y );
			file.Write16( nodes[i].delta );
		}
	}

	/**********************************************************************************************
	 *
	 * Export...
	 *
	 **********************************************************************************************/

	void Bank::ExportI( const char *filename, u16 *ModulePointers ) {
		IO::File file( filename, IO::MODE_WRITE );
		
		file.Write16( SampleCount );
		//file.Skip( 2*256 + 2 * SampleCount ); // skip pointers
		
		//u16 ModulePointers[256];

		// write modules
		for( u32 i = 0; i < imodules.size(); i++ ) {
			ModulePointers[i] = file.Tell();
			imodules[i]->Export( file );
		}

//		file.Seek( 0x0002 );
//		for( u32 i = 0; i < 256; i++ ) {
//			file.Write16( ModulePointers[i] );
//		}
	}

	void Bank::ExportE( const char *filename, u16 *IModulePointers ) {
		IO::File file( filename, IO::MODE_WRITE );

		for( u32 i = 0; i < 256; i++ ) {
			file.Write16( IModulePointers[i] );
		}
		
		file.Skip( 2*256 + 2*SampleCount );
		
		u16 ModulePointers[256];
		for( u32 i = 0; i < emodules.size(); i++ ) {
			file.WriteAlign( 64 );
			ModulePointers[i] = file.Tell() / 64;
			emodules[i]->Export( file, imodules[i]->GetSequenceLength() );
		}
		
		u16 *SamplePointers = new u16[SampleCount];
		for( u32 i = 0; i < SampleCount; i++ ) {
			file.WriteAlign( 64 );
			SamplePointers[i] = file.Tell() / 64;
			samples[i]->Export( file );
		}

		file.Seek( 0x200 );

		for( u32 i = 0; i < 256; i++ ) {
			file.Write16( ModulePointers[i] );
		}

		for( u32 i = 0; i < SampleCount; i++ ) {
			file.Write16( SamplePointers[i] );
		}

		delete[] SamplePointers;
	}
	
	void Bank::Export( const char *ibank, const char *ebank ) {

		u16 IModPointers[256];
		if( ibank ) {
			ExportI( ibank, IModPointers );
		}
		if( ebank ) {
			ExportE( ebank, IModPointers );
		}
	}
};
