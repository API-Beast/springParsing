/* 
 * DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
 * TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION
 *
 * 0. You just DO WHAT THE FUCK YOU WANT TO.
 */

#include "../springParsing/Parser.h"
#include <iostream>

namespace
{
	void printTree(SyntaxTree* node, std::string intend="")
	{
		std::cout << intend << node->Type << ":" << '"' << node->Value << '"' << std::endl;
		if(node->Child)   printTree(node->Child,   intend+"  ");
		if(node->Sibling) printTree(node->Sibling, intend);
	};
}

int main(int argc, char** argv)
{
  Parser p;
  // Mini-Expressions approach
  /*p.define("expression", {"[#product]", Parser::Or("+", "-"), "[#product]"});
  p.define("product",    {"[#value]",   Parser::Or("*", "/"), "[#value]"});
  p.define("value",      {"[0-9]+"},
		              {"[#expression]"});*/

  // Combinator approach
  {
    using namespace ParserRules;
    p.define("expression", Sentence({Rule("product"), Rule("add-op"), Rule("product")}));
		p.define("add-op",     Or(String("+"), String("-")));
		p.define("mult-op",    Or(String("*"), String("/")));
    p.define("product",    Or(Sentence({Rule("value"), Rule("mult-op"), Rule("value")}), Rule("value")));
    p.define("value",      Or(OneOrMore(Rule("digit")), Rule("expression")));
		p.define("digit",      InRange('0', '9'));
  }
  
  p.SkipWhitespace = true;
  ParserData result = p.parseString("expression", "12+85*2-12/4");
	
  if(result.ErrorMessage.size())
		std::cout << "Parser-Error - " << result.ErrorMessage << " at " << std::string(result.cur(), 4);
	else
		printTree(result.AST);
	
	return 0;
}