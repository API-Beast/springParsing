//  Copyright (C) 2015 Manuel Riecke <api.beast@gmail.com>
//  Licensed under the terms of the WTFPL.
//
//  TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION
//  0. You just DO WHAT THE FUCK YOU WANT TO.

#include "Parser.h"

#include <cassert>
#include <vector>
#include "trex.h"

ParserRule* Parser::define(const std::string& name, const ParserRule& rule)
{
	ParserRule* copy = new ParserRule(rule);
	Rules[name] = copy;
	copy->Label = name;
	return copy;
}

ParserRule* Parser::get(const std::string& name)
{
	auto it = Rules.find(name);
	if(it == Rules.end())
		return nullptr;
	return it->second;
}

ParserData Parser::parseString(const std::string& ruleName, const std::string& str)
{
	ParserData data;
	data.RuleSet = this;
	data.Input   = str;
	data.Pos     = 0;
	
	ParserRule* rule = this->get(ruleName);
	if(rule)
		rule->apply(&data);
	
	return data;
}

bool ParserRule::apply(ParserData* p)
{
	if(p->Error)
		return false;
	
	if(!ResolvedReference && Reference.size())
		ResolvedReference = p->RuleSet->get(Reference);
	
	int prevPos = p->Pos;
	if(p->RuleSet->SkipWhitespace == true)
	{
		while(p->Input[p->Pos] == ' ')
			p->Pos++;
	}

	bool result = false;
	
	p->beginNode(this->Label);
	
	if(p->ElementStack.size()>64)
	{
		p->ErrorMessage  = "Syntax Error";
		p->Error = true;
		return false;
	}
	
	if(ResolvedReference)
		result = ResolvedReference->apply(p);
	else if(Function)
		result = Function(p);

	if(result == true && p->Error == false)
	{
		p->completeNode();
		return true;
	}
	else
	{
		p->discardNode();
		p->Pos = prevPos;
		return false;
	}
}



ParserRule::ParserRule(){}

ParserRule::ParserRule(ParserRuleFunctor fun, const std::string& name) : Function(fun), Label(name){}
ParserRule::ParserRule(const std::string& str) : Reference(str){}

ParserRule ParserRules::InRange(Codepoint a, Codepoint b)
{
	auto funky = [a, b](ParserData* p) mutable -> bool
	{
		Codepoint c = UTF8::DecodeAt(p->Input, p->Pos);
		if(c >= a && c <= b)
		{
			p->Pos++;
			return true;
		}
		return false;
	};
	return ParserRule(funky);
}

ParserRule ParserRules::OneOrMore(ParserRule a)
{
	auto funky = [a](ParserData* p) mutable -> bool
	{
		static auto rule = a;
		int runs = 0;
		while(rule.apply(p)) runs++;
		return runs > 0;
	};
	return ParserRule(funky);
}

ParserRule ParserRules::ZeroOrMore(ParserRule a)
{
	auto funky = [a](ParserData* p) mutable -> bool
	{
		while(a.apply(p));
		return true;
	};
	return ParserRule(funky);
}

ParserRule ParserRules::Optional(ParserRule a)
{
	auto funky = [a](ParserData* p) mutable -> bool
	{
		a.apply(p); // Ignore result.
		return true;
	};
	return ParserRule(funky);
}

ParserRule ParserRules::Or(ParserRule a, ParserRule b)
{
	auto funky = [a, b](ParserData* p) mutable -> bool
	{
		
		if(!a.apply(p))
			return b.apply(p);
		return true;
	};
	return ParserRule(funky);
}

ParserRule ParserRules::Sentence(std::initializer_list< ParserRule > rules)
{
	std::vector<ParserRule> rulez(rules);
	auto funky = [rulez](ParserData* p) mutable -> bool
	{
		// ParserData rollback = *p;
		for(ParserRule rule : rulez)
		{
			if(rule.apply(p))
				continue;
			else
			{
				// (*p) = rollback;
				return false;
			}
		}
		return true;
	};
	return ParserRule(funky);
}

ParserRule ParserRules::String(const std::string& str)
{
	auto funky = [str](ParserData* p) -> bool
	{
		int i = 0;
		while(true)
		{
			// When comparing strings UTF8 is not important.
			if(p->Input[p->Pos+i] != str[i])
			{
				return false;
			}
			i++;
			if(i == str.length())
			{
				p->Pos += i;
				return true;
			}
		}
	};
	return ParserRule(funky, "string");
}

ParserRule ParserRules::RegEx(const std::string& str)
{
	auto funky = [str](ParserData* p) -> bool
	{
		const char* error = nullptr;
		TRex* exp=trex_compile(str.c_str(), &error);
		if(error)
			return false;
		
		const char* newPos = 0;
		bool result = trex_matchcontinous(exp, p->cur(), p->end(), &newPos);
		
		trex_free(exp);
		
		if(!result)
			return result;
		
		p->setPos(newPos);
		return true;
	};
	return ParserRule(funky, "regex");
}
	
ParserRule ParserRules::Rule(const std::string& name)
{
	ParserRule result;
	result.Reference = name;
	return result;
}

void ParserData::beginNode(const std::string& type)
{
	SyntaxTree* element = new SyntaxTree;
	element->Type = type;
	ElementStack.push(element);
	PositionStack.push(Pos);
}

void SyntaxTree::insertChild(SyntaxTree* child)
{
	// Insert into the family.
	if(Child == nullptr)
		Child = child;
	else
	{
		SyntaxTree* sibling = Child;
		while(sibling->Sibling)
			sibling = sibling->Sibling;
		sibling->Sibling = child;
	}
}

void ParserData::completeNode()
{
	SyntaxTree* element = nullptr;
	SyntaxTree* parent  = nullptr;
	element = ElementStack.top();
	ElementStack.pop();
	
	int startPosition = PositionStack.top();
	PositionStack.pop();
	element->Value = Input.substr(startPosition, Pos-startPosition);
	
	if(!ElementStack.empty())
		parent = ElementStack.top();
	else
		parent = AST;
	
	if(!parent)
	{
		AST = element;
		return;
	}
	
	// Blank elements aren't inserted into the syntax tree.
	if(element->Type.empty())
	{
		// Instead we merge them with the parent.
		// This inserts the whole chain of children, not just the first one.
		if(element->Child)
			parent->insertChild(element->Child);
		delete element;
		return;
	}
	else
		parent->insertChild(element);
}

void ParserData::discardNode()
{
	delete ElementStack.top();
	ElementStack.pop();
	Pos = PositionStack.top();
	PositionStack.pop();
}
