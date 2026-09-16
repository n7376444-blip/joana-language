#include "joana/joana.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
using namespace joana;
static void print(const std::vector<Diagnostic>&d){for(auto&x:d)std::cerr<<(x.severity==Severity::Error?"error":"warning")<<"["<<x.code<<"] "<<x.message<<" at "<<x.span.line<<":"<<x.span.column<<"\n";}
int main(int argc,char**argv){bool check=false,graph=false;std::string path;for(int i=1;i<argc;i++){std::string a=argv[i];if(a=="--check")check=true;else if(a=="--graph")graph=true;else path=a;}if(path.empty()){std::cerr<<"usage: joana [--check|--graph] file.joana\n";return 2;}std::ifstream f(path);if(!f){std::cerr<<"cannot open "<<path<<"\n";return 2;}std::stringstream b;b<<f.rdbuf();auto r=compile(b.str());print(r.diagnostics);if(!r.graph)return 1;if(graph){std::cout<<dump_graph(*r.graph);return 0;}if(check){std::cout<<"Compilation successful.\nGraph verified.\n";return 0;}std::unordered_map<std::string,Value> in;for(auto&i:r.graph->inputs){std::string x;std::cout<<i.name<<": ";if(!std::getline(std::cin,x))return 1;if(i.type==Type::Int)in[i.name]=std::stoll(x);else if(i.type==Type::Bool)in[i.name]=(x=="true");else in[i.name]=x;}auto v=execute(*r.graph,in);if(!v.ok){std::cerr<<"runtime error: "<<v.error<<"\n";return 1;}std::cout<<value_string(v.value)<<"\n";return 0;}
