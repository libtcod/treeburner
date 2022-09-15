/*
* Umbra
* Copyright (c) 2009, 2010 Mingos, Jice
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * The names of Mingos or Jice may not be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY MINGOS & JICE ``AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL MINGOS OR JICE BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <string>

enum UmbraErrorLevel {
	UMBRA_ERRORLEVEL_NOTICE,
	UMBRA_ERRORLEVEL_WARNING,
	UMBRA_ERRORLEVEL_ERROR,
	UMBRA_ERRORLEVEL_FATAL_ERROR
};

class UmbraError {
	private:
		static TCODList <std::string*> errors; //list of all errors as C++ strings
		/**
		 * Saves the error log to an external file for further examination
		 */
		static void save ();

	public:
		/**
		 * Adds a new error to the error log.
		 * @param errLev The error level of the logged error, e.g. UMBRA_ERRORLEVEL_ERROR
		 * @param errStr The error message, as a C string. Uses printf-like formatting.
		 * @return The index number of the error in the current log.
		 */
		static int add (UmbraErrorLevel errLev, const char * errStr, ...); //adds an error to the list
		/**
		 * Adds a new error to the error log.
		 * @param errLev The error level of the logged error, e.g. UMBRA_ERRORLEVEL_ERROR
		 * @param errStr The error message, as a C++ string.
		 * @return The index number of the error in the current log.
		 */
		static int add (UmbraErrorLevel errLev, std::string errStr); //adds an error to the list
		/**
		 * Checks whether a file exists in the filesystem.
		 * @param filename The filename to be checked, as a C string. Uses printf-like formatting
		 * @return true if the file exists, false otherwise
		 */
		static bool fileExists (const char * filename, ...); //confirms the existence of a file
		/**
		 * Retrieves the last logged error message.
		 * @return a C string containing the error message. If there are no errors, "No errors registered" will be returned.
		 */
		static const char * getLastMessage (); //retrieves the last message
		/**
		 * Retrieves a specific logged error message.
		 * @param idx The index number of the logged message
		 * @return a C string containing the error message. If the requested index doesn't exist, "Requested wrong error index" will be returned.
		 */
		static const char * getMessage (int idx); //retrieve message with index number idx
		/**
		 * Retrieves the total number of errors in the error log.
		 * @return The number of errors in the error log
		 */
		static inline int getNbErrors () { return errors.size(); }
};
