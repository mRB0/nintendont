#include "itloader.h"
#include "io.h"

namespace ITLoader {

	template<typename T> static void deletePtrVector( std::vector<T*> &vecs ) {
		
		for(typename std::vector<T*>::iterator iter = vecs.begin(), ending = vecs.end();
			iter != ending; 
			iter++ ) {
		
			if( *iter )
				delete (*iter);
		}
		
		vecs.clear();
	}

	Bank::Bank( const ConversionInput::SoundbankData *source ) {
		for( u32 i = 0; i < source->modules.size(); i++ ) {
			AddModule( source->modules[i]->filename.c_str() );
		}
	}

	Bank::~Bank() {
		deletePtrVector<Module>( modules );
	}

	void Bank::AddModule( const char *filename ) {

		modules.push_back( new Module( filename ) );
	}

	Module::Module( const char *filename ) {
		IO::File file( filename, IO::MODE_READ );
		
		if( file.Read8() != 'I' ) return;
		if( file.Read8() != 'M' ) return;
		if( file.Read8() != 'P' ) return;
		if( file.Read8() != 'M' ) return;

		for( int i = 0; i < 26; i++ )
			Title[i] = file.Read8();

		PatternHighlight = file.Read16();

		Length = file.Read16();
		InstrumentCount = file.Read16();
		SampleCount = file.Read16();
		PatternCount = file.Read16();
		Cwtv = file.Read16();
		Cmwt = file.Read16();
		Flags = file.Read16();
		Special = file.Read16();
		GlobalVolume = file.Read8();
		MixingVolume = file.Read8();
		InitialSpeed = file.Read8();
		InitialTempo = file.Read8();

		Sep = file.Read8();
		PWD = file.Read8();
		MessageLength = file.Read16();
		
		u32 MessageOffset = file.Read32();
		
		file.Skip( 4 ); // reserved
		
		for( int i = 0; i < 64; i++ )
			ChannelPan[i] = file.Read8();

		for( int i = 0; i < 64; i++ )
			ChannelVolume[i] = file.Read8();
		
		bool foundend=false;
		int ActualLength=Length;
		for( int i = 0; i < 256; i++ ) {
			Orders[i] = i < Length ? file.Read8() : 255;
			if( Orders[i] == 255 && !foundend ) {
				foundend=true;
				ActualLength = i+1;
			}
		}
		
		Length = ActualLength;

		Instruments = new Instrument*[InstrumentCount];
		Samples = new Sample*[SampleCount];
		Patterns = new Pattern*[PatternCount];

		u32 *InstrTable = new u32[InstrumentCount];
		u32 *SampleTable = new u32[SampleCount];
		u32 *PatternTable = new u32[SampleCount];
		
		for( int i = 0; i < InstrumentCount; i++ )
			InstrTable[i] = file.Read32();
		for( int i = 0; i < SampleCount; i++ )
			SampleTable[i] = file.Read32();
		for( int i = 0; i < PatternCount; i++ )
			PatternTable[i] = file.Read32();

		for( int i = 0; i < InstrumentCount; i++ ) {
			file.Seek( InstrTable[i] );
			Instruments[i] = new Instrument( file );
		}

		for( int i = 0; i < SampleCount; i++ ) {
			file.Seek( SampleTable[i] );
			Samples[i] = new Sample( file );
		}

		for( int i = 0; i < PatternCount; i++ ) {
			file.Seek( PatternTable[i] );
			Patterns[i] = new Pattern( file );
		}

		if( MessageLength ) {
			Message = new char[MessageLength];
			file.Seek( MessageOffset );
			for( int i = 0; i < MessageLength; i++ )
				Message[i] = file.Read8();
		} else {
			Message = 0;
		}

		delete[] InstrTable;
		delete[] SampleTable;
		delete[] PatternTable;
	}

	Module::~Module() {
		for( int i = 0; i < InstrumentCount; i++ )
			delete Instruments[i];
		delete[] Instruments;
		for( int i = 0; i < SampleCount; i++ )
			delete Samples[i];
		delete[] Samples;
		for( int i = 0; i < PatternCount; i++ )
			delete Patterns[i];
		delete[] Patterns;
		if( Message ) {
			delete[] Message;
		}
	}
	
	Instrument::Instrument( IO::File &file ) {
		file.Skip(
			4		// 'IMPI'
			+12		// dos filename
			+1		// 00h
		);
		NewNoteAction = file.Read8();
		DuplicateCheckType = file.Read8();
		DuplicateCheckAction = file.Read8();
		Fadeout = file.Read16();
		PPS = file.Read8();
		PPC = file.Read8();
		GlobalVolume = file.Read8();
		DefaultPan = file.Read8();
		RandomVolume = file.Read8();
		RandomPanning = file.Read8();
		TrackerVersion = file.Read16();
		NumberOfSamples = file.Read8();

		file.Skip(
			26		// instrument name
		);

		InitialFilterCutoff = file.Read8();
		InitialFilterResonance = file.Read8();

		MidiChannel = file.Read8();
		MidiProgram = file.Read8();
		MidiBank = file.Read16();

		file.Read8(); // reserved
		
		for( int i = 0; i < 120; i++ ) {
			Notemap[i].Note = file.Read8();
			Notemap[i].Sample = file.Read8();
		}
		
		VolumeEnvelope = new Envelope( file );
		PanningEnvelope = new Envelope( file );
		PitchEnvelope = new Envelope( file );
	}

	Instrument::~Instrument() {
		delete VolumeEnvelope;
		delete PanningEnvelope;
		delete PitchEnvelope;
	}

	Envelope::Envelope( IO::File &file ) {
		u8 FLG = file.Read8();
		Enabled = !!(FLG & 1);
		Loop = !!(FLG & 2);
		Sustain = !!(FLG & 4);
		IsFilter = !!(FLG & 128);

		Length = file.Read8();
		
		LoopStart = file.Read8();
		LoopEnd = file.Read8();
		
		SustainStart = file.Read8();
		SustainEnd = file.Read8();

		for( int i = 0; i < 25; i++ ) {
			Nodes[i].y = file.Read8();
			Nodes[i].x = file.Read16();
		}
	}

	Sample::Sample( IO::File &file ) {
		file.Skip(
			4		// IMPS
			+12		// dos filename
			+1		// 00h
		);
		GlobalVolume = file.Read8();
		u8 Flags = file.Read8();
		
		HasSample = !!(Flags & 1);
		Bits16 = !!(Flags & 2);
		Stereo = !!(Flags & 4);
		Compressed = !!(Flags & 8);
		Loop = !!(Flags & 16);
		Sustain = !!(Flags & 32);
		BidiLoop = !!(Flags & 64);
		BidiSustain = !!(Flags & 128);
		
		DefaultVolume = file.Read8();
		file.Skip( 26 ); // sample name
		
		Convert = file.Read8();
		DefaultPanning = file.Read8();
		
		Length = file.Read32();
		LoopStart = file.Read32();
		LoopEnd = file.Read32();
		C5Speed = file.Read32();
		SustainStart = file.Read32();
		SustainEnd = file.Read32();

		u32 SamplePointer = file.Read32();

		VibratoSpeed = file.Read8();
		VibratoDepth = file.Read8();
		VibratoRate = file.Read8();
		VibratoForm = file.Read8();

		file.Seek( SamplePointer );
		LoadData( file );
	}

	Sample::~Sample() {
		if( Bits16 ) {
			delete[] Data16;
		} else {
			delete[] Data8;
		}
	}
	
	void Sample::LoadData( IO::File &file ) {
		if( !Compressed ) {
			if( Convert & 1 ) {
				// signed samples
				if( Bits16 ) {
					Data16 = new s16[ Length ];
					for( int i = 0; i < Length; i++ ) {
						Data16[i] = file.Read16();
					}
				} else {
					Data8 = new s8[ Length ];
					for( int i = 0; i < Length; i++ ) {
						Data8[i] = file.Read8();
					}
				}
			} else {
				// unsigned samples
				if( Bits16 ) {
					Data16 = new s16[ Length ];
					for( int i = 0; i < Length; i++ ) {
						Data16[i] = file.Read16() - 32768;
					}
				} else {
					Data8 = new s8[ Length ];
					for( int i = 0; i < Length; i++ ) {
						Data8[i] = file.Read8() - 128;
					}
				}
			}
		} else {
			// TODO : accept compressed samples.
		}
	}

	Pattern::Pattern( IO::File &file ) {
		DataLength = file.Read16();
		Rows = file.Read16();
		file.Skip( 4 ); // reserved
		Data = new u8[ DataLength ];
		for( int i = 0; i < DataLength; i++ ) {
			Data[i] = file.Read8();
		}
	}

	Pattern::~Pattern() {
		if( Data ) {
			delete[] Data;
		}
	}

};
