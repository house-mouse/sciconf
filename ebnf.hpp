/*
 * A set of EBNF description classes
 */

#ifndef __EBNF_HPP__
#define __EBNF_HPP__


#include <string>
#include <vector>
#include <map>
#include <memory> // for shared_ptr

using namespace std;

class config_file;

struct config_point {
public:
    shared_ptr<config_point> parent; // chain of how we got here
    shared_ptr<config_file>  file;   // reference to the file this is about
    unsigned int byte_offset;        // Offset in the file for the point
    unsigned int line_number;        // number of \n's preceding this point in the file
    unsigned int line_offset;        // characters from the last \n
    
    config_point(shared_ptr<config_point> _parent,
               shared_ptr<config_file>  _file,
               unsigned int _character_offset=0,
               unsigned int _line_number=0,
               unsigned int _line_offset=0);
    void reset(shared_ptr<config_file>  _file,
               unsigned int _character_offset=0,
               unsigned int _line_number=0,
               unsigned int _line_offset=0);
    void advance(int characters=1);
    void cr();
    
    // convenience pass-through function
    virtual bool match(const string &utf8_string,
                       config_point &position_after) const;
    
};

// Wrapper to get at the config info...
// This lets us use a file or a blob of memory...

class config_file {
public:
    virtual string name() = 0;                 // return a filename or reference to this object
    /*
    virtual void seek(config_point &where);   // set file position to the point
    virtual char peek_next_chars();           // return the next character
 
    virtual int  advance(int size=1);         // move forward in the file this many characters
     */
    
    // This may need refactoring, but the idea is to provide a single
    // point for compare functions that can be utf-8 (or any other
    // encoding) sane.  UTF-8 can encode characters in multiple ways,
    // so it's not enough to do a binary compare and call that good
    // all of the time.  All the complexity goes in this function.
    // Since the number of bytes needed to encode the two strings
    // may not be the same, position_after is required to tell us
    // how many bytes of the source file were actually consumed.
    
    // It must not change position_after unless it matches
    // (No match does not move position_after)
    
    virtual bool match(const config_point &where,
                       const string &utf8_string,
                       config_point &position_after) = 0;
};

//
// Simplest implementation of an in-memory config_file
//
class memory_file:public config_file {
    string _name;
    string data;
    memory_file(string &name, string &data);
public:
    static shared_ptr<memory_file> New(string name, string data);
    
    
    virtual bool match(const config_point &where,
                       const string &utf8_string,
                       config_point &position_after);
    virtual string name(); // return a filename or reference to this object
};

class ebnf_object;

struct parse_tree {
    shared_ptr<ebnf_object> owner; // ebnf_object that matched
    config_point end;              // end point of match
    vector<parse_tree> children;   // children that matched
    
    parse_tree();
    parse_tree(shared_ptr<ebnf_object> owner,
               const config_point &end);
    void add_child(shared_ptr<ebnf_object> owner, const config_point &end);
};

// Base class of most EBNF things...
class ebnf_object:public enable_shared_from_this<ebnf_object> {
public:
    string key; // ebnf 'identifier'

    virtual const string description() { return "object"; }
    virtual bool match(const config_point &where,
                       config_point &position_after)=0;
    virtual bool parse(parse_tree &tree)=0;
};

// Even "characters" are treated like "stings" because
// this allows us to use utf-8 "characters" easily...

class ebnf_string:public ebnf_object {
    string value;
    
    ebnf_string(const string &value);
public:
    virtual ~ebnf_string() {};

    static shared_ptr<ebnf_string> New(const string &value="");

    virtual const string description();
    
    virtual bool match(const config_point &where,
                       config_point &position_after);
    
    virtual bool parse(parse_tree &tree);
};

class ebnf_group:public ebnf_object {
protected:
    vector<shared_ptr<ebnf_object> > objects;
public:
    virtual ~ebnf_group() {};

    virtual const string description();
    virtual void add(shared_ptr<ebnf_object> item);
    
};

ebnf_group& operator<<(ebnf_group& group, shared_ptr<ebnf_object> item);


// a | b ...
// Logical "or"

class ebnf_alternation:public ebnf_group {
    ebnf_alternation();
public:
    virtual ~ebnf_alternation() {};

    static shared_ptr<ebnf_alternation> New();

    virtual const string description();
    
    virtual bool match(const config_point &where,
                       config_point &position_after);
    
    virtual bool parse(parse_tree &tree);

};

// a, b ...
// logical and (in order!)
class ebnf_concatenation:public ebnf_group {
    ebnf_concatenation();
public:
    virtual ~ebnf_concatenation() {};

    static shared_ptr<ebnf_concatenation> New();

    virtual const string description();
    
    virtual bool match(const config_point &where,
                       config_point &position_after);
    bool parse(parse_tree &tree);

};

// everything in set a but not in set b
// (a - b)
class ebnf_exception:public ebnf_object {
    ebnf_exception();
    shared_ptr<ebnf_object> everything_here;
    shared_ptr<ebnf_object> except_this;

public:
    virtual ~ebnf_exception() {};
    
    static shared_ptr<ebnf_exception> New();
    static shared_ptr<ebnf_exception> New(shared_ptr<ebnf_object> everything,
                                          shared_ptr<ebnf_object> except);
    static shared_ptr<ebnf_exception> New(shared_ptr<ebnf_object> except);
    virtual const string description();
    virtual bool match(const config_point &where,
                       config_point &position_after);
    bool parse(parse_tree &tree);

};

// { ... }

class ebnf_repetition:public ebnf_object {
    ebnf_repetition();
    shared_ptr<ebnf_object> repeated;
    unsigned int count;
public:
    virtual ~ebnf_repetition() {};

    static shared_ptr<ebnf_repetition> New();
    static shared_ptr<ebnf_repetition> New(shared_ptr<ebnf_object> repeated, unsigned int count=0);

    virtual const string description();

    virtual bool match(const config_point &where,
                       config_point &position_after);
    
    bool parse(parse_tree &tree);

};

// That's the end of the "fundamentals"...


class ebnf_rule:public ebnf_group {
    shared_ptr<ebnf_object> lhs, rhs;
    ebnf_rule();
public:
    virtual ~ebnf_rule() {};

    static shared_ptr<ebnf_rule> New();
    static shared_ptr<ebnf_rule> New(shared_ptr<ebnf_object> lhs,
                                     shared_ptr<ebnf_object> rhs);
    virtual const string description();
    
    bool parse(parse_tree &tree);

};

class ebnf_grammar {
    map<string, shared_ptr<ebnf_object> > key_rhs;
public:
    virtual ~ebnf_grammar() {};

    ebnf_grammar(); // don't use this directly!

    static shared_ptr<ebnf_grammar> New();

    void add(string key, shared_ptr<ebnf_object> rhs);
    
    int parse_file(parse_tree &parse_tree, string key);
};

#endif // __EBNF_HPP__
