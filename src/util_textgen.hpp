/*
* Copyright (c) 2009 Jice
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * The name of Jice may not be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY Jice ``AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL Jice BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

struct Rule {
	const char *name;
	TCODList<const char *> values;
};
struct TextGen {
	const char *name;
	TCODList<Rule *>rules;
};

class ITextGeneratorFunc {
public :
	virtual const char * execute(const char *params) = 0;
};

struct TextGenFunc {
	const char *name;
	ITextGeneratorFunc *func;
};

class TextGenerator : public ITCODParserListener {
public :
	TextGenerator(const char *txgFile, TCODRandom *rng = NULL);
	// if not called, will be called at first generate call
	//might be used if we need to access generators data without calling generate first
	void parseFile();
	// generate the text in a user supplied buffer
	const char *generateBuf(const char *generatorName, char *buf, const char *source, ...);
	// generate text in a static buffer. subsequent calls override the buffer content !
	const char *generate(const char *generatorName, const char *source, ...);
	// runtime definition of a rule. runtime rule can be referenced in the config file.
	// if called more than once with the same varName, generate a random value from the list
	static void addGlobalValue(const char *varName, const char *varValue, ...);
	// remove a runtime rule
	static void deleteGlobalValue(const char *varName);
	// runtime definition of a function
	static void setGlobalFunction(const char *funcName, ITextGeneratorFunc *func);
	void setLocalFunction(const char *funcName, ITextGeneratorFunc *func);

	// file parsing callbacks
	bool parserNewStruct(TCODParser *parser,const TCODParserStruct *str,const char *name);
	bool parserFlag(TCODParser *parser,const char *name);
	bool parserProperty(TCODParser *parser,const char *name, TCOD_value_type_t type, TCOD_value_t value);
	bool parserEndStruct(TCODParser *parser,const TCODParserStruct *str, const char *name);
	void error(const char *msg);
	TCODList<TextGen *> generators;
	static TCODList<Rule *> globalRules;
	static TCODList<TextGenFunc *> globalFuncs;
	TCODList<TextGenFunc *> localFuncs;
private :
	char * goatSoup2(const char *generator, const char *source,char *buf);
	// capitalize first char of the generated text ?
	bool goatfirst;
	// config filename
	const char *filename;
	// filename has been parsed ?
	bool init;
	TCODRandom *textGenRng;

};

// functions for the text generator
// RANDOM_INT(min,max)
class RandomIntFunc : public ITextGeneratorFunc {
public :
	RandomIntFunc(TCODRandom *riRng) : riRng(riRng) {}
	const char * execute(const char *params);
protected :
	TCODRandom *riRng;
};

// RANDOM_NAME()
class RandomNameFunc : public ITextGeneratorFunc {
public :
	RandomNameFunc(TCODRandom *rnRng) : rnRng(rnRng) {}
	const char * execute(const char *params) {
		return NameGenerator::generateRandomName(rnRng);
	}
protected :
	TCODRandom *rnRng;
};

// NUMBER_TO_LETTER(num)
class NumberToLetterFunc : public ITextGeneratorFunc {
public :
	const char * execute(const char *params);
};

