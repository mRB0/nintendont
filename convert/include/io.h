// a class to read a binary file with stdio

#ifndef __IO_H
#define __IO_H

#include <stdio.h>
#include "basetypes.h"

namespace IO {

	typedef enum {
		MODE_READ,
		MODE_WRITE
	} FileAccessMode;

	class File {

	private:
		FILE	*file;
		bool	isOpen;
		FileAccessMode mode;

	public:
		File();
		File( const char *, FileAccessMode );
		~File();
		bool Open( const char *, FileAccessMode );
		void Close();
		
		u8 Read8();
		u16 Read16();
		u32 Read32();

		void Write8( u8 );
		void Write16( u16 );
		void Write32( u32 );
		
		void Skip( s32 amount );
		u32 Tell();
		void Seek( u32 offset );

		bool IsOpen();

	};

	bool FileExists();

};

#endif
