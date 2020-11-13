#include "ebnf.hpp"

//
// config_point
//

config_point::config_point(shared_ptr<config_point> _parent,
                       shared_ptr<config_file>  file,
                       unsigned int _character_offset,
                       unsigned int _line_number,
                       unsigned int _line_offset) {
    parent=_parent;
    reset(file,
          _character_offset,
          _line_number,
          _line_offset);
}

void config_point::reset(shared_ptr<config_file>  _file,
           unsigned int _character_offset,
           unsigned int _line_number,
           unsigned int _line_offset) {
    file         = _file;
    byte_offset  = _character_offset;
    line_number  = _line_number;
    line_offset  = _line_offset;
}

void config_point::advance(int bytes) {
    byte_offset += bytes;
    line_offset += line_offset;
}

void config_point::cr() {
    line_offset      = 0;
    byte_offset++;
    line_number++;
}

bool config_point::match(const string &utf8_string,
                         config_point &position_after) const {
    return file->match(*this, utf8_string, position_after);
}

//
// memory_file
//

memory_file::memory_file(string &__name, string &_data):_name(__name), data(_data) {}

shared_ptr<memory_file> memory_file::New(string name, string data) {
    return shared_ptr<memory_file>(new memory_file(name, data));
}

string memory_file::name() {
    return _name;
}


bool memory_file::match(const config_point &where,
                        const string &utf8_string,
                        config_point &position_after) {
    if (!utf8_string.length()) {
        return false; // error ?
    }
    
    // TODO: make this utf8 awesome... stubs for utf8rewind
    
    if ((where.byte_offset + utf8_string.size()) > data.length()) {
        return false;
    }
    
    if (data.compare(where.byte_offset, utf8_string.length(), utf8_string) == 0) {
        position_after = where;
        for (int i=0; i<utf8_string.length(); i++) {
            if (data.at(position_after.byte_offset) == '\n') {
                position_after.cr();
            } else {
                position_after.advance();
            }
        }
        return true;
    }
    return false;
}

//
// parse_tree
//

parse_tree::parse_tree(shared_ptr<ebnf_object> _owner,
                       const config_point &_end):owner(_owner), end(_end) {}

void parse_tree::add_child(shared_ptr<ebnf_object> _owner,
                       const config_point &_end) {
    // TODO... why doesn't emplace exist/work anymore?
    parse_tree t(_owner, _end);
    children.push_back(t);
}

// ebnf_string

ebnf_string::ebnf_string(const string &_value):value(_value) {
}

shared_ptr<ebnf_string> ebnf_string::New(const string &value) {
    return shared_ptr<ebnf_string>(new ebnf_string(value));
}

bool ebnf_string::match(const config_point &where,
                        config_point &position_after) {
    return where.match(value, position_after);
}

const string ebnf_string::description() {
    return "string";
}

bool ebnf_string::parse(parse_tree &tree) {
    if (tree.end.match(value, tree.end)) {
        tree.add_child(shared_from_this(), tree.end);
        return true;
    }
    return false;
}

// ebnf_group

void ebnf_group::add(shared_ptr<ebnf_object> item) {
    objects.push_back(item);
}

const string ebnf_group::description() {
    return "group";
}

ebnf_group& operator<<(ebnf_group& group, shared_ptr<ebnf_object> item) {
    group.add(item);
    return group;
}


// ebnf_alternation

ebnf_alternation::ebnf_alternation() {
}

shared_ptr<ebnf_alternation> ebnf_alternation::New() {
    return shared_ptr<ebnf_alternation>(new ebnf_alternation());
}

const string ebnf_alternation::description() {
    return "alternation";
}

bool ebnf_alternation::match(const config_point &where,
                          config_point &position_after) {
    for (auto &i:objects) {
        if (i->match(where, position_after)) {
            return true;
        }
    }
    
    return false;
}

bool ebnf_alternation::parse(parse_tree &tree) {
    
    // Add ourself...
    tree.add_child(shared_from_this(), tree.end);
    
    parse_tree &our_tree(tree.children.back());
    
    for (auto &i:objects) {
        if (i->parse(our_tree)) {
            our_tree.end = our_tree.children.back().end;
            return true;
        }
    }
    
    // revert pushing us on... we didn't match.
    tree.children.pop_back();
    return false;
}

// ebnf_concatenation

ebnf_concatenation::ebnf_concatenation() {
}

shared_ptr<ebnf_concatenation> ebnf_concatenation::New() {
    return shared_ptr<ebnf_concatenation>(new ebnf_concatenation());
}

const string ebnf_concatenation::description() {
    return "concatenation";
}

bool ebnf_concatenation::match(const config_point &where,
                             config_point &position_after) {
    config_point start(where);
    for (auto &i:objects) {
        if (!i->match(start, start)) {
            return false;
        }
    }
    position_after=start;
    return true;
}

bool ebnf_concatenation::parse(parse_tree &tree) {
    
    // Add ourself...
    
    tree.add_child(shared_from_this(), tree.end);
    
    parse_tree &our_tree(tree.children.back());
    
    for (auto &i:objects) {
        
        if (!i->parse(our_tree)) {
            // revert pushing us on... we didn't match.
            tree.children.pop_back();
            return false;
        }
        our_tree.end = our_tree.children.back().end;
    }
    
    if (!our_tree.children.empty()) {
        our_tree.end = our_tree.children.back().end;
    }

    return true;
}

// ebnf_exception

ebnf_exception::ebnf_exception() {
}

shared_ptr<ebnf_exception> ebnf_exception::New() {
    return shared_ptr<ebnf_exception>(new ebnf_exception());
}

shared_ptr<ebnf_exception> ebnf_exception::New(shared_ptr<ebnf_object> except) {
    auto rv = New();
    rv->except_this = except;
    return rv;
}

shared_ptr<ebnf_exception> ebnf_exception::New(shared_ptr<ebnf_object> everything,
                                               shared_ptr<ebnf_object> except) {
    auto rv = New(except);
    rv->everything_here = everything;
    return rv;
}

const string ebnf_exception::description() {
    return "exception";
    
}

bool ebnf_exception::match(const config_point &where,
                           config_point &position_after) {
    
    if ((!everything_here) || (everything_here->match(where, position_after))) {
        config_point throw_away(where);
        if (!except_this->match(where, throw_away)) {
            return true;
        }
    }
    return false;
}

// This logic may be borked...
// if it's "ana" and not "an" does it match or not?

bool ebnf_exception::parse(parse_tree &tree) {

    // punt for now...
    return false;
}

// ebnf_repetition

ebnf_repetition::ebnf_repetition() {
}

shared_ptr<ebnf_repetition> ebnf_repetition::New() {
    return shared_ptr<ebnf_repetition>(new ebnf_repetition());
}

shared_ptr<ebnf_repetition> ebnf_repetition::New(shared_ptr<ebnf_object> repeated, unsigned int count) {
    auto rv = New();
    rv->repeated = repeated;
    rv->count=count;
    return rv;
}

const string ebnf_repetition::description() {
    return "repetition";
}

bool ebnf_repetition::parse(parse_tree &tree) {

    // Add ourself...
    tree.add_child(shared_from_this(), tree.end);
    
    parse_tree &our_tree(tree.children.back());
    
    int count=0;
    while (repeated->parse(our_tree)) {
        our_tree.end = our_tree.children.back().end;
        count++;
    }
    
    return true; // matching 0 times is valid...
}

bool ebnf_repetition::match(const config_point &where,
                           config_point &position_after) {
    // TODO
    return false;
}

// ebnf_grammar

ebnf_grammar::ebnf_grammar() {
}

shared_ptr<ebnf_grammar> ebnf_grammar::New() {
    return shared_ptr<ebnf_grammar>(new ebnf_grammar());
}

void ebnf_grammar::add(string key, shared_ptr<ebnf_object> rhs) {
    rhs->key=key;

    key_rhs.insert(pair<string, shared_ptr<ebnf_object> >(key, rhs));
}


int ebnf_grammar::parse_file(parse_tree &parse_tree,
                             string key) {
    auto pair = key_rhs.find(key);
    if (pair==key_rhs.end()) {
        return -1; // no such key!
    }
    
    
    if (pair->second->parse(parse_tree)) {
        return 0;
    }
    
    return -2;
}
