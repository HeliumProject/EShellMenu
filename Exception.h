#pragma once

#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>

#include <string>
#include <exception>

namespace Launcher
{
	//
	// Constants
	//

	const size_t ERROR_STRING_BUF_SIZE = 768; 

	//
	// Basic Exception
	//  Try to only throw in "error" cases. Examples:
	//   * A file format API trying to open a file that doesn't exist (the client api should check if it exists if it was graceful execution)
	//   * A disc drive or resource is out of space and cannot be written to
	//   * A network port is taken and cannot be bound
	//

	class Exception
	{
	protected:
		mutable std::string m_Message;

	protected:
		Exception()
		{

		}

	public:
		Exception( const char *msgFormat, ... )
		{
			va_list msgArgs;
			va_start( msgArgs, msgFormat );
			SetMessage( msgFormat, msgArgs );
			va_end( msgArgs );
		}

		//
		// These accessors are thow that re-throw blocks can amend the exception message
		//

		std::string& Get()
		{
			return m_Message;
		}

		const std::string& Get() const
		{
			return m_Message;
		}

		void Set(const std::string& message)
		{
			m_Message = message;
		}

		//
		// This allow operation with std::exception case statements
		//

		virtual const char* What() const
		{
			return m_Message.c_str();
		}

	protected:
		void SetMessage( const char* msgFormat, ... )
		{
			va_list msgArgs;
			va_start( msgArgs, msgFormat );
			SetMessage( msgFormat, msgArgs );
			va_end( msgArgs );
		}

		void SetMessage( const char* msgFormat, va_list msgArgs )
		{
			char msgBuffer[ERROR_STRING_BUF_SIZE];

			_vsnprintf( msgBuffer, sizeof(msgBuffer) / sizeof( char ), msgFormat, msgArgs );
			msgBuffer[sizeof(msgBuffer) / sizeof(msgBuffer[0]) - 1] = 0; 

			m_Message = msgBuffer;
		}
	};
}