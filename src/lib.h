#pragma once
#include <string>
#include <map>
#include <memory>
#include <vector>
#include <set>

struct Option
{
    bool useWeakSymbol = false;
    bool verbose = false;
};
extern Option g_opt;

class Lib : public std::enable_shared_from_this<Lib>
{
public:
    explicit Lib(std::string const& libname);

    void init();

    std::string name_;
    std::set<std::string> definedSymbols_;
    std::set<std::string> undefinedSymbols_;
    std::set<std::string> weakSymbols_;
};
typedef std::shared_ptr<Lib> LibPtr;

class LibManager
{
public:
    static LibManager& getInstance();

    void addLib(std::string const& libname);

    void addSymbol(std::string const& symbol, LibPtr const& libPtr);

    LibPtr findSymbol(std::string const& symbol);

    LibPtr getLib(std::string const& libname);

    void addDep(std::string const& libname, std::string const& depname);

public:
    void run();
    void dumpDeps();
    void dumpLinkArgs();

private:
    std::map<std::string, LibPtr> libs_;

    std::map<std::string, std::set<LibPtr>> globalSymbolTable_;

    // <被依赖的lib, <<依赖的lib, 依赖符号数>...>>
    std::map<std::string, std::map<std::string, long>> depsTable_;
};
