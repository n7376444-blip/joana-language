#include "joana/joana.hpp"
#include <cassert>
#include <iostream>
using namespace joana;
static void ok(std::string s){auto r=compile(s);assert(r.graph.has_value());assert(r.graph->status==GraphStatus::Verified);}
static void bad(std::string s,std::string code){auto r=compile(s);assert(!r.graph);bool found=false;for(auto&d:r.diagnostics)if(d.code==code)found=true;assert(found);}
int main(){ok("flow main() -> String { emit \"Hello, World!\"; }");ok("operation greet(name: String) -> String = \"Hello, \" + name; flow main(name: String) -> String { let message = greet(name); emit message; }");ok("operation add(a: Int,b: Int) -> Int = a+b; flow main() -> Int { let x=add(2,3); emit x; }");bad("flow main() -> String { emit missing; }","UNDEFINED_VALUE");bad("flow main(x: Int) -> String { let y=unknown(x); emit y; }","UNDEFINED_OPERATION");bad("operation g(x: String) -> String = x; flow main(x: Int) -> String { let y=g(x); emit y; }","TYPE_MISMATCH");bad("operation n() -> Int = 1; flow main() -> String { let x=n(); emit x; }","OUTPUT_TYPE_MISMATCH");std::cout<<"joana_tests: all tests passed\n";}
