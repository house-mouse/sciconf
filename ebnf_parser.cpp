#include "ebnf_parser.hpp"

// This hard codes a boot-strap parser based on the
// wikipedia definition of EBNF
// It's quite painful.  Clearly some syntatic sugar
// and helper functions would be needed if we ever
// intended to use this interface for anything real.
// (We don't)

void add_ascii_range(shared_ptr<ebnf_group> target,
                     char first, char last) {
    
    for (char c=first; c<=last; c++) {
        string utf8;
        utf8 += c;
        target->add(ebnf_string::New(utf8));
    }
}

void add_ascii_elements(shared_ptr<ebnf_group> target,
                     const char *elements) {
    
    for (const char *c=elements; *c; c++) {
        string utf8;
        utf8 += *c;
        target->add(ebnf_string::New(utf8));
    }
}

ebnf_parser::ebnf_parser() {
    // Letters
    
    auto letter = ebnf_alternation::New();
    
    add_ascii_range(letter, 'a', 'z');
    add_ascii_range(letter, 'A', 'Z');
    add("letter", letter);

    // digits
    
    auto digit = ebnf_alternation::New();
    
    add_ascii_range(digit, '0', '9');

    add("digit", digit);
    
    // symbol
    
    auto symbol = ebnf_alternation::New();
    
    add_ascii_elements(symbol, "[" "]" "{" "}" "(" ")" "<" ">"
                        "'"  "\"" "=" "|" "." "," ";" );
    
    add("symbol", symbol);
    
    // character
    
    auto character = ebnf_alternation::New();
    *character << letter << digit << symbol << ebnf_string::New("_");
    
    add("character", character);

    // whitespace
    
    auto whitespace_character = ebnf_alternation::New();
    
    add_ascii_elements(whitespace_character, "\n" "\r" "\t" " " );
    
    add("whitespace_character", whitespace_character);

    // whitespace

    auto whitespace = ebnf_repetition::New(whitespace_character);
    
    add("whitespace", whitespace);

    // identifier
    
    auto identifier = ebnf_concatenation::New();
    
    auto identifier_alternation = ebnf_alternation::New();
    *identifier_alternation << letter << digit << ebnf_string::New("_");
    
    *identifier << letter << ebnf_repetition::New(identifier_alternation);
    
    add("identifier", identifier);

    // single_quote_terminal

    auto single_quote_terminal = ebnf_concatenation::New();
    
    *single_quote_terminal << ebnf_string::New("'")
                           << ebnf_exception::New(ebnf_string::New("'"))
                           << ebnf_string::New("'");
    
    add("single_quote_terminal", single_quote_terminal);

    // double_quote_terminal
    
    auto double_quote_terminal = ebnf_concatenation::New();

    *double_quote_terminal << ebnf_string::New("\"")
                           << ebnf_exception::New(ebnf_string::New("\""))
                           << ebnf_string::New("\"");
    
    add("double_quote_terminal", double_quote_terminal);

    
    // terminal
    // This is a lame terminal with no escapes, but .. whatever..
    
    auto terminal = ebnf_alternation::New();
    *terminal << single_quote_terminal << double_quote_terminal;
    
    add("terminal", terminal);

    // lhs
    
    auto lhs = ebnf_alternation::New();
    *lhs << identifier;
    add("lhs", lhs);
    
    
    // rhs
    // well, this is fun... it's recursive so it has to late bind...
    
    auto rhs = ebnf_alternation::New();

    
    // optional
    
    auto optional = ebnf_concatenation::New();
    
    *optional << ebnf_string::New("[")
              << rhs
              << ebnf_string::New("]");
    
    add("optional", optional);
    
    
    // repetition
    
    auto repetition = ebnf_concatenation::New();
    
    *repetition << ebnf_string::New("{")
                << rhs
                << ebnf_string::New("}");
    
    add("repetition", repetition);

    // group
    
    auto group = ebnf_concatenation::New();
    
    *group << ebnf_string::New("(")
           << rhs
           << ebnf_string::New(")");
    
    add("group", group);

    // alternation
    
    auto alternation = ebnf_concatenation::New();
    
    *alternation << rhs
                 << ebnf_string::New("|")
                 << rhs;
    
    add("alternation", alternation);

    
    auto concatenation = ebnf_concatenation::New();
    
    *concatenation << rhs
                   << ebnf_string::New(",")
                   << rhs;
    
    add("concatenation", concatenation);
    
    *rhs << identifier << terminal << optional << repetition << group << alternation << concatenation;
    
    add("rhs", rhs);

    // rule
    
    auto rule = ebnf_concatenation::New();
    *rule << lhs << whitespace << ebnf_string::New("=") << whitespace << rhs << whitespace << ebnf_string::New(";");
    add("rule", rule);
    
    
}
