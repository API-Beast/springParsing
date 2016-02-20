//  Copyright (C) 2015 Manuel Riecke <api.beast@gmail.com>
//  Licensed under the terms of the WTFPL.
//
//  TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION
//  0. You just DO WHAT THE FUCK YOU WANT TO.

#pragma once

#include "UTF8.h"
#include <string>
#include <stack>
#include <map>
#include <functional>

struct Parser;
struct ParserRule;
struct SyntaxTree;

struct SyntaxTree
{
	SyntaxTree* Child = nullptr;
	SyntaxTree* Sibling = nullptr;
	
	std::string Type;
	std::string Value;
	
	void insertChild(SyntaxTree* child);
};

struct ParserData
{
	std::string Input;
	int Pos;
	
	SyntaxTree* AST = nullptr;
	Parser* RuleSet;
	
	const char* begin(){ return Input.data(); };
	const char* cur()  { return begin()+Pos;};
	const char* end()  { return begin()+Input.size(); };
	void setPos(const char* newPos){ Pos = newPos-begin(); };
	
	void beginNode(const std::string& type);
	void completeNode();
	void discardNode();
	
	struct ElementData
	{
		std::string Type;
		int StartPos;
	};
	
	std::stack<SyntaxTree*> ElementStack;
	std::stack<int>         PositionStack;
	std::string             ErrorMessage;
	bool                    Error = false;
};

struct Parser
{
	ParserRule* define(const std::string& name, const ParserRule& rule);
	ParserData  parseString(const std::string& ruleName, const std::string& str);
	ParserRule* get(const std::string& name);
	
	std::map<std::string, ParserRule*> Rules;
	bool SkipWhitespace = true;;
};

using ParserRuleFunctor = std::function<bool(ParserData*)>;

struct ParserRule
{
	//ParserRule(std::initializer_list<ParserRule>);
	ParserRule(const std::string&);
	ParserRule(ParserRuleFunctor fun, const std::string& name = "");
	ParserRule();
	
	bool apply(ParserData* p);
	
	std::string Label;
	ParserRuleFunctor Function;
	std::string Reference = "";
	ParserRule* ResolvedReference = nullptr;
};

namespace ParserRules
{
	ParserRule Sentence(std::initializer_list<ParserRule>);
	ParserRule Or(ParserRule a, ParserRule b);
	ParserRule Rule(const std::string& name);
	ParserRule String(const std::string& str);
	ParserRule OneOrMore(ParserRule a);
	ParserRule ZeroOrMore(ParserRule a);
	ParserRule Optional(ParserRule a);
	ParserRule InRange(Codepoint a, Codepoint b);
	ParserRule RegEx(const std::string& str);
}
