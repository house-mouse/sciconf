#include "ebnf_parser.hpp"


int main() {
    auto file(memory_file::New("test1", "numbers = abcdefg;"));

    ebnf_parser parser;
    
    shared_ptr<config_point> parent;
    
    config_point cp(parent, file);
    
    parse_tree pt(shared_ptr<ebnf_object>(0), cp);
    
    int rv = parser.parse_file(pt, "rule");
    
    printf("Result: %d\n", rv);
}
