#include "lib.h"
#include <stdio.h>
#include <string.h>

Option g_opt;

std::vector<std::string> runCommand(std::string const& cmd)
{
    FILE * input = popen(cmd.c_str(), "r");
    if (!input) {
        fprintf(stderr, "runCommand %s error:%s\n", cmd.c_str(), strerror(errno));
        return {};
    }

    std::vector<std::string> lines;
    std::string s;
    for (;;) {
        int c = fgetc(input);
        if (c == EOF)
            break;

        if ((char)c == '\n') {
            lines.push_back(s);
            s.clear();
        } else {
            s += (char)c;
        }
    }
    if (!s.empty())
        lines.push_back(s);

    pclose(input);

    if (g_opt.verbose) {
        printf("runCommand %s returns:\n", cmd.c_str());
        for (auto & s : lines) {
            printf("%s\n", s.c_str());
        }
        printf("---------------\n");
    }
    return lines;
}

Lib::Lib(std::string const& libname)
    : name_(libname)
{
}
void Lib::init()
{
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "nm %s 2>/dev/null | grep \" [Tt] \" | awk 'BEGIN{FS=\" [Tt] \";}{print $2}'", name_.c_str());
    auto lines = runCommand(cmd);
    for (auto symbol : lines) {
        definedSymbols_.insert(symbol);
        LibManager::getInstance().addSymbol(symbol, shared_from_this());
    }

    snprintf(cmd, sizeof(cmd), "nm %s 2>/dev/null | grep \" [Ww] \" | awk 'BEGIN{FS=\" [Ww] \";}{print $2}'", name_.c_str());
    lines = runCommand(cmd);
    for (auto symbol : lines) {
        weakSymbols_.insert(symbol);

        if (g_opt.useWeakSymbol) {
            LibManager::getInstance().addSymbol(symbol, shared_from_this());
        }
    }

    snprintf(cmd, sizeof(cmd), "nm %s 2>/dev/null | grep \" [Uu] \" | awk 'BEGIN{FS=\" [Uu] \";}{print $2}'", name_.c_str());
    lines = runCommand(cmd);
    for (auto symbol : lines) {
        if (definedSymbols_.count(symbol))
            continue;

        if (weakSymbols_.count(symbol))
            continue;

        undefinedSymbols_.insert(symbol);
    }
}
LibManager & LibManager::getInstance()
{
    static LibManager obj;
    return obj;
}
void LibManager::addLib(const std::string & libname)
{
    if (libs_.count(libname)) {
        fprintf(stderr, "lib conflit: %s\n", libname.c_str());
        return ;
    }

    LibPtr libPtr = std::make_shared<Lib>(libname);
    libPtr->init();
    libs_[libname] = libPtr;
    depsTable_[libname];
}
void LibManager::addSymbol(std::string const& symbol, LibPtr const& libPtr)
{
    globalSymbolTable_[symbol].insert(libPtr);
}
LibPtr LibManager::findSymbol(std::string const& symbol)
{
    auto iter = globalSymbolTable_.find(symbol);
    return (iter != globalSymbolTable_.end() && !iter->second.empty()) ? *iter->second.begin() : LibPtr();
}
LibPtr LibManager::getLib(std::string const& libname)
{
    auto iter = libs_.find(libname);
    return iter == libs_.end() ? LibPtr() : iter->second;
}
void LibManager::addDep(std::string const& libname, std::string const& depname)
{
    depsTable_[depname][libname]++;
}
void LibManager::run()
{
    for (auto & kv : libs_) {
        std::string const& libname = kv.first;
        LibPtr const& libPtr = kv.second;

        for (auto & undefinedSymbol : libPtr->undefinedSymbols_) {
            LibPtr libPtr = findSymbol(undefinedSymbol);
            if (!libPtr) continue;

            addDep(libname, libPtr->name_);
        }
    }
}
void LibManager::dumpDeps()
{
    printf("------------ Deps -------------\n");
    for (auto & kv : depsTable_) {
        printf("%s <- [", kv.first.c_str());
        for (auto & libkv : kv.second) {
            printf("%s(%ld), ", libkv.first.c_str(), libkv.second);
        }
        printf("]\n");
    }
    printf("-------------------------------\n");
}
void LibManager::dumpLinkArgs()
{
    auto depsTable = depsTable_;

    printf("------------ Link Args -------------\n");
    std::set<std::string> outSet;
    for (auto & kv : libs_)
        outSet.insert(kv.first);

retry:
    for (auto & lib : outSet) {
        if (!depsTable.count(lib) || depsTable[lib].empty()) {
            printf("%s ", lib.c_str());
            outSet.erase(lib);

            for (auto & kv : depsTable) {
                kv.second.erase(lib);
            }
            goto retry;
        }
    }

    if (!outSet.empty()) {
        printf("-Wl,--whole-archive ");
        for (auto & lib : outSet) {
            printf("%s ", lib.c_str());
        }
        printf("-Wl,--no-whole-archive");
    }
    printf("\n");

    printf("-------------------------------------n");
}

