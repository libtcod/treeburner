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

#include "umbra.hpp"
#include <stdarg.h>
#include <stdio.h>

//resolve errorlevel strings
std::string errLevStr (UmbraErrorLevel errLev) {
	std::string ret;
	switch (errLev) {
		case UMBRA_ERRORLEVEL_NOTICE:
			ret = "NOTICE: "; break;
		case UMBRA_ERRORLEVEL_WARNING:
			ret = "WARNING: "; break;
		case UMBRA_ERRORLEVEL_ERROR:
			ret = "ERROR: "; break;
		case UMBRA_ERRORLEVEL_FATAL_ERROR:
			ret = "FATAL ERROR: "; break;
		default:
			ret = "UNKNOWN ERROR: "; break;
	}
	return ret;
}

//initialise empty error list
TCODList <std::string*> UmbraError::errors;

//append an error message
int UmbraError::add (UmbraErrorLevel errLev, const char * errStr, ...) {
	char err[2048];
	va_list ap;
	va_start(ap,errStr);
	vsprintf(err,errStr,ap);
	va_end(ap);

	std::string * errorMessage = new std::string ();
	errorMessage->assign(errLevStr(errLev) + std::string(err));

	errors.push(errorMessage);
	fprintf(stderr,"%s\n",errorMessage->c_str());

	save();

	return errors.size()-1;
}

//append an error message
int UmbraError::add (UmbraErrorLevel errLev, std::string errStr) {
	std::string * errorMessage = new std::string();
	errorMessage->assign(errLevStr(errLev) + std::string(errStr));

	errors.push(errorMessage);
	fprintf(stderr,"%s\n",errorMessage->c_str());

	save();

	return errors.size()-1;
}


//save the error log
void UmbraError::save () {
	//if there are no errors, return
	if (errors.size() == 0) return;
	//else create the log file
	else {
		//create an empty error log file
		FILE * out = fopen("errorlog.txt","w");

		//print the log header
		fprintf(out,"%s ver. %s (%s) Error Log\n"
					"---===---\n"
					"%d %s registered in the log.\n"
					"\n"
					"\n",
					UMBRA_TITLE,UMBRA_VERSION,UMBRA_STATUS,errors.size(),errors.size() > 1 ? "errors" : "error");

		//print the errors
		int i = 0;
		do {
			std::string * msg = errors.get(i);
			fprintf(out,"%03d. %s\n",i+1,msg->c_str());
		} while (++i < errors.size());

		//close the log file
		fclose(out);
	}
}

const char * UmbraError::getLastMessage () {
	int size = errors.size(); //SIZE MATTERS!
	if (size == 0) return "No errors registered.";
	else return (errors.get(size-1))->c_str();
}

const char * UmbraError::getMessage (int idx) {
	if (idx < 0 || idx >= errors.size()) return "Requested wrong error index.";
	else return (errors.get(idx))->c_str();
}



//confirms the existence of a file. Returns false if there is no such file.
bool UmbraError::fileExists (const char * filename, ...) {
	char f[256];
	va_list ap;
	va_start(ap,filename);
	vsprintf(f,filename,ap);
	va_end(ap);

	bool retVal = false;
	FILE * in = fopen(f,"rb");
	if (in != NULL) {
		retVal = true;
		fclose(in);
	}
	return retVal;
}
