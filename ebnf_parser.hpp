#ifndef __EBNF_PARSER_HPP__
#define __EBNF_PARSER_HPP__
#include "ebnf.hpp"

/*
 The goal here is to create a simple EBNF parser.  The rules
 for the parser are taken almost directly from wikipedia, though
 I had to add whitespace and some rules around grouping:
 
 letter = "A" | "B" | "C" | "D" | "E" | "F" | "G"
        | "H" | "I" | "J" | "K" | "L" | "M" | "N"
        | "O" | "P" | "Q" | "R" | "S" | "T" | "U"
        | "V" | "W" | "X" | "Y" | "Z" | "a" | "b"
        | "c" | "d" | "e" | "f" | "g" | "h" | "i"
        | "j" | "k" | "l" | "m" | "n" | "o" | "p"
        | "q" | "r" | "s" | "t" | "u" | "v" | "w"
        | "x" | "y" | "z" ;
 
 digit = "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9" ;
 symbol = "[" | "]" | "{" | "}" | "(" | ")" | "<" | ">"
 | "'" | '"' | "=" | "|" | "." | "," | ";" ;
 character = letter | digit | symbol | "_" ;
 
 whitespace = "\n" | "\r" | "\t" | " "
 
 identifier = letter , { letter | digit | "_" } ;
 terminal = "'" , character , { character } , "'"
          | '"' , character , { character } , '"' ;
 
 lhs = identifier ;
 rhs = identifier
    | terminal
    | "[" , rhs , "]"
    | "{" , rhs , "}"
    | "(" , rhs , ")"
    | rhs , "|" , rhs
    | rhs , "," , rhs ;
 
 rule = lhs , "=" , rhs , ";" ;
 grammar = { rule } ;
 
 
 There's no attempt to be fast or efficient... just correct...
 It's not expected that this parser will ever be used to parse anything particularly big... it's a bootstrap for other parsers.
 
 */

class ebnf_parser:public ebnf_grammar {
public:
    ebnf_parser();
    
};

#endif // __EBNF_PARSER_HPP__
