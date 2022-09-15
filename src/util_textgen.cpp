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
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include "main.hpp"

TCODList<Rule *> TextGenerator::globalRules;
TCODList<TextGenFunc *> TextGenerator::globalFuncs;

const char * NumberToLetterFunc::execute(const char *params) {
	static const char *num0_20[20] = {
		"zero","one","two","three","four","five","six","seven","eight","nine",
		"ten", "eleven","twelve","thirteen","fourteen","fifteen","sixteen","seventeen","eighteen","nineteen"
	};
	int num = atoi(params);
	if ( num >= 0 && num <= 19 ) {
		return num0_20[num];
	}
	// TODO
	return params;
}

const char * RandomIntFunc::execute(const char *params) {
	static char buf[32];
	int min,max;
	sscanf(params,"%d,%d",&min,&max);
	int i=riRng->getInt(min,max);
	sprintf(buf,"%d",i);
	return buf;
}


static inline bool startsWith(const char *s, const char *sub) {
	return strncmp(s,sub,strlen(sub)) == 0;
}
static inline bool isNumber(int c) {
	return c >= '0' && c <= '9';
}

TextGenerator::TextGenerator(const char *txgFile, TCODRandom *textGenRng) : init(false) {
	filename=txgFile;
	if ( textGenRng == NULL ) this->textGenRng = TCODRandom::getInstance();
	else this->textGenRng=textGenRng;
}

void TextGenerator::parseFile() {
	TCODParser parser;
	TCODParserStruct *textgen = parser.newStructure("textgen");
	TCODParserStruct *rule = parser.newStructure("rule");
	textgen->addStructure(rule);
	rule->addProperty("value",TCOD_TYPE_STRING,false);
	rule->addListProperty("values",TCOD_TYPE_STRING,false);
	parser.run(filename,this);
	init=true;
}

bool TextGenerator::parserNewStruct(TCODParser *parser,const TCODParserStruct *str,const char *name) {
	if ( strcmp(str->getName(),"textgen") == 0 ) {
		TextGen *textgen=new TextGen();
		textgen->name = strdup(name);
		generators.push(textgen);
	} else {
		Rule *rule=new Rule();
		rule->name=strdup(name);
		generators.peek()->rules.push(rule);
	}
	return true;
}

bool TextGenerator::parserFlag(TCODParser *parser,const char *name) {
	return true;
}

bool TextGenerator::parserProperty(TCODParser *parser,const char *name, TCOD_value_type_t type, TCOD_value_t value) {
	if ( strcmp(name,"value") == 0 ) {
		generators.peek()->rules.peek()->values.push(strdup(value.s));
	} else {
		TCODList<const char *>values=value.list;
		generators.peek()->rules.peek()->values.addAll(values);
	}
	return true;
}

bool TextGenerator::parserEndStruct(TCODParser *parser,const TCODParserStruct *str, const char *name) {
	return true;
}

void TextGenerator::error(const char *msg) {
	fprintf(stderr,"%s",msg);
	std::abort();
}

char * TextGenerator::goatSoup2(const char *generator, const char *source,char *buf) {
	TextGen *gen=NULL;
	for ( TextGen **it=generators.begin(); it != generators.end(); it++ ) {
		if ( strcmp((*it)->name,generator) == 0 ) {
			gen = *it;
			break;
		}
	}
	if ( gen == NULL )  {
		fprintf (stderr,"Unknown text generator %s in file %s\n",generator,filename);
		std::abort();
	}
	for(;;)
	{	int c=*(source++);
		if(c=='\0')	break;
		if ( c < 0 ) c+=256;
		if(*source != '{' || (c != '$' && c != '?')) {
			if ( goatfirst ) {
				(*buf++)=toupper(c);
				goatfirst=false;
				*buf=0;
			} else {
				(*buf++)=c;
				*buf=0;
			}
		} else {
			source++;
			if ( c == '?' && textGenRng->getInt(0,1) == 0 ) {
				// ignore the var
				source = strchr(source,'}');
				source++;
				continue;
			}
			Rule *rule=NULL;
			char *end=(char *)strchr(source,'}');
			*end=0;
			char *funcptr=(char *)strchr(source,'(');
			bool wasfunc=false;
			// look for a function
			if ( funcptr && funcptr < end ) {
				*funcptr=0;
				TextGenFunc *func=NULL;
				// a local func first
				for (TextGenFunc **fit=localFuncs.begin(); fit != localFuncs.end(); fit++) {
					if ( strcmp((*fit)->name,source) == 0 ) {
						func=*fit;
						break;
					}
				}
				// if not found, look in global funcs
				if (! func ) {
					for (TextGenFunc **fit=globalFuncs.begin(); fit != globalFuncs.end(); fit++) {
						if ( strcmp((*fit)->name,source) == 0 ) {
							func=*fit;
							break;
						}
					}
				}
				*funcptr='(';
				if (func ) {
					char *funcend=strchr(funcptr,')');
					if ( funcend && funcend < end ) {
						*funcend=0;
						source=funcptr+1;
						const char *funcresult = func->func->execute(source);
						strcpy(buf,funcresult);
						buf += strlen(funcresult);
						wasfunc=true;
						*funcend=')';
						source=end+1;
					}
				}
			}
			if (! wasfunc) {
				// look in the generator rules
				for (Rule **rit=gen->rules.begin(); rit != gen->rules.end(); rit++) {
					if ( strcmp((*rit)->name,source) == 0 ) {
						rule=*rit;
						break;
					}
				}
				// not found. look in global rules
				if (! rule )
				for (Rule **rit=globalRules.begin(); rit != globalRules.end(); rit++) {
					if ( strcmp((*rit)->name,source) == 0 ) {
						rule=*rit;
						break;
					}
				}

				*end='}';
				if ( ! rule || rule->values.size() == 0 ) {
					// unknown or empty rule. copy
					do {
						(*buf++)=*source++;
					} while ( *source != '}');
					source++;
					*buf=0;
				} else {
					int rnd = textGenRng->getInt(0,rule->values.size()-1);
					buf = goatSoup2(generator,rule->values.get(rnd),buf);
					source = end+1;
				}
			}
		}	/* endelse */
	}	/* endwhile */
	return buf;
}

void TextGenerator::addGlobalValue(const char *varName, const char *varValue, ...) {
	static char tmp[1024];
	va_list ap;
	va_start (ap, varValue);
	vsprintf(tmp,varValue,ap);
	va_end(ap);
	Rule *rule=NULL;
	for (Rule **it=globalRules.begin(); it != globalRules.end(); it++ ) {
		if ( strcmp((*it)->name,varName) == 0 ) {
			rule=*it;
			break;
		}
	}
	if (! rule ) {
		rule = new Rule();
		rule->name=strdup(varName);
		globalRules.push(rule);
	}
	rule->values.push(strdup(tmp));
}

void TextGenerator::setGlobalFunction(const char *funcName, ITextGeneratorFunc *func) {
	for (TextGenFunc **it=globalFuncs.begin(); it != globalFuncs.end(); it++) {
		if ( strcmp((*it)->name,funcName) == 0 ) {
			// the function already exists. update it
			(*it)->func=func;
			return;
		}
	}
	// create a new func
	TextGenFunc *tgf = new TextGenFunc();
	tgf->name = strdup(funcName);
	tgf->func=func;
	globalFuncs.push(tgf);
}

void TextGenerator::setLocalFunction(const char *funcName, ITextGeneratorFunc *func) {
	for (TextGenFunc **it=localFuncs.begin(); it != localFuncs.end(); it++) {
		if ( strcmp((*it)->name,funcName) == 0 ) {
			// the function already exists. update it
			(*it)->func=func;
			return;
		}
	}
	// create a new func
	TextGenFunc *tgf = new TextGenFunc();
	tgf->name = strdup(funcName);
	tgf->func=func;
	localFuncs.push(tgf);
}

void TextGenerator::deleteGlobalValue(const char *varName) {
	for (Rule **it=globalRules.begin(); it != globalRules.end(); it++ ) {
		if ( strcmp((*it)->name,varName) == 0 ) {
			Rule *rule=*it;
			for ( const char **sit=rule->values.begin(); sit != rule->values.end(); sit++) {
				free((void*)*sit);
			}
			free((void *)rule->name);
			delete rule;
			globalRules.remove(it);
			return;
		}
	}
}

const char *TextGenerator::generateBuf(const char *generator, char *buf, const char *source, ...) {
	static char tmp[1024];
	va_list ap;
	va_start (ap, source);
	vsprintf(tmp,source,ap);
	va_end(ap);
	if (! init) parseFile();
	goatfirst=true;
	goatSoup2(generator,tmp,buf);
	return buf;
}

const char *TextGenerator::generate(const char *generator, const char *source, ...) {
	static char buf[1024];
	static char tmp[1024];
	va_list ap;
	va_start (ap, source);
	vsprintf(tmp,source,ap);
	va_end(ap);
	goatfirst=true;
	if (! init) parseFile();
	goatSoup2(generator,tmp,buf);
	return buf;
}
