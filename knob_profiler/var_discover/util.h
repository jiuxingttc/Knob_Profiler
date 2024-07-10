#ifndef _KNOB_PROFILER_UTIL_H
#define _KNOB_PROFILER_UTIL_H   

#include <fstream>
#include <cstdarg>
#include <algorithm>
#include <string>
#include <vector>
#include <regex>
#include <cxxabi.h>

#include "llvm/IR/CFG.h"
#include "llvm/IR/IntrinsicInst.h"

using namespace llvm;

namespace knobprofiler {

    std::string cxxabi_demangle(const std::string &mangledName){
        int status;
        char *demangled = abi::__cxa_demangle(mangledName.c_str(), nullptr, nullptr, &status);
        if(status == 0 && demangled){
            std::string result(demangled);
            std::size_t offset = result.find('(');
            if (offset != std::string::npos){ 
                result = result.substr(0, offset);
            }
            free(demangled);
            return result;
        }else{
            return mangledName;
        }
    }

    void KPLog(const char *path, const char *fmt, ...){
        va_list mark;
        char buf[4096] = {0};
        va_start(mark, fmt);
        vsprintf(buf, fmt, mark);
        va_end(mark);
        std::string msg(buf);
        std::ofstream ofstr(path, std::ofstream::out|std::ofstream::app);
        if (!ofstr) 
            return;
        std::copy(msg.begin(), msg.end(), std::ostream_iterator<char>(ofstr));
    }

    #define SCHEMA_FILE "/tmp/knob_profiler/schema.txt"
    #define SRC2BASICBLOCK_FILE "/tmp/knob_profiler/src2basicblock.txt"
    #ifdef SCHEMA_FILE
    #define KPLogSchema(fmt, ...) KPLog(SCHEMA_FILE, fmt, ##__VA_ARGS__ )
    #else
    #define KPLogSchema(fmt, ...) do{} while(0)
    #endif
    #ifdef SRC2BASICBLOCK_FILE
    #define KPSrc2Basicblock(fmt, ...)KPLog(SRC2BASICBLOCK_FILE, fmt, ##__VA_ARGS__)
    #else
    #define KPSrc2Basicblock(fmt, ...) do{}while(0)
    #endif

    static std::string getSimplebbLabel(const BasicBlock *bb){
        const Function *F = bb->getParent();
        if(!bb->getName().empty())
            return F->getName().str()+'#'+bb->getName().str();

        std::string str;
        raw_string_ostream OS(str);
        bb->printAsOperand(OS, false);
        return F->getName().str()+'#'+ OS.str();
    }

    static std::vector<std::string> record_precessors_successors(const BasicBlock *bb){
        std::vector<std::string> sequences;
        for (const BasicBlock *Pred : predecessors(bb)){
            sequences.push_back(getSimplebbLabel(Pred));
        }
        sequences.push_back(getSimplebbLabel(bb));
        for (const BasicBlock *Succ : successors(bb)){
            sequences.push_back(getSimplebbLabel(Succ));
        }
        return sequences;
    }

    static void block2SrcRange(const BasicBlock *bb, int *begin, int *end) {
        for (auto it = bb->begin(),ed = bb->end();it != ed; ++it){
            const Instruction *I = &*it;
            if (isa<DbgInfoIntrinsic>(I))
                continue;
            const DebugLoc &Loc = I->getDebugLoc();
            if (!Loc)
                continue;
            if (Loc.getLine()== 0)
                continue;
            const int lineno = Loc.getLine();
            if(*begin == -1)
                *begin = lineno;
            else if (*begin > lineno)
                *begin = lineno;
            if (*end < lineno)
                *end = lineno;
        }
    }

    std::unordered_map<const Function *, std::vector<int>> visitedFunctions;

    void updateFunctionInfo(const Function *f, const int begin, const int end) {
        std::vector<int> location{begin, end};
        if(visitedFunctions.find(f)== visitedFunctions.end())
            visitedFunctions[f]= location;
        else{
            if (visitedFunctions[f][0]==-1 || begin < visitedFunctions[f][0])
                visitedFunctions[f][0]= begin;
            if (end > visitedFunctions[f][1])
                visitedFunctions[f][1]= end;
        }
    }

    void SaveBlockInfo(const BasicBlock *bb){
        int beginLine = -1,endLine = -1;
        block2SrcRange(bb,&beginLine, &endLine);
        std::vector<std::string> seq = record_precessors_successors(bb);
        KPSrc2Basicblock("tag=%s,begin=%d,end=%d;",getSimplebbLabel(bb).c_str(),beginLine,endLine);
        for (auto elem: seq)
            KPSrc2Basicblock("%s,", elem.c_str());
        KPSrc2Basicblock("\n");

        if(beginLine !=-1 || endLine != -1)
            updateFunctionInfo(bb->getParent(), beginLine, endLine);
    }

    void SaveFunctionInfo(const Function *f){
        if(visitedFunctions.find(f) == visitedFunctions.end())
            return;
        std::string demangledName = cxxabi_demangle(f->getName().str());
        KPSrc2Basicblock("function=%s,begin=%d,end=%d,filename=%s\n",demangledName.c_str(),visitedFunctions[f][0],visitedFunctions[f][1],f->getParent()->getSourceFileName().c_str());
    }

    bool hasSuffix(const std::string& str, const std::string& suffix){
        if (str.length() < suffix.length()) return false; 
        return str.rfind(suffix)==(str.length()- suffix.length());
    }

    void SaveResult(Function *F, const std::unordered_map<Value*, MyMDNode> & result_vars) {
        for(auto const & kv : result_vars){
            MDNode *var = kv.second.node;
            if(!var)
                continue;
            DIVariable *node = dyn_cast<DIVariable>(var);
            if(!node)
                continue;
            std::string type("uintptr");
            if(node->getType()){
                std::string type1 = node->getType()->getName().str();
                std::replace(type1.begin(), type1.end(), ' ', '#');
                if(type1.size() > 0 && type1 != " ")
                    type = type1;
            }
            std::string demangledName =cxxabi_demangle(F->getName().str());
            std::string filename = node->getFilename().str();
            if(!hasSuffix(filename, ".cc"))
                continue;
            KPLogSchema("%s %s %s %d %s %s %s\n",node->getDirectory().str().c_str(),node->getFilename().str().c_str(),demangledName.c_str(),node->getLine(),node->getName().str().c_str(),type.c_str(),kv.second.type.c_str());    
        }
    }

    void SaveGlobalResult(Module& m, const std::unordered_set<GlobalValue*>& globals){
        NamedMDNode *CUNodes = m.getNamedMetadata("llvm.dbg.cu");
        if(!CUNodes)
            return;
        for( unsigned I=0,E=CUNodes->getNumOperands();I!=E; ++I){
            auto *CU = cast<DICompileUnit>(CUNodes->getOperand(I));
            auto *GVs = dyn_cast_or_null<MDTuple>(CU->getRawGlobalVariables());
            if(!GVs)
                continue;
            for(unsigned I=0; I<GVs->getNumOperands();I++){
                auto *GV = dyn_cast_or_null<DIGlobalVariableExpression>(GVs->getOperand(I));
                if(!GV)
                    continue;
                DIGlobalVariable *Var = GV->getVariable();
                for (auto gVal: globals){
                    if (gVal->getGlobalIdentifier()!= Var->getName()) continue;
                    //set the default type, in case Type name is not available
                    std::string type("uintptr");
                    if (Var->getType()){
                        type = Var->getType()->getName().str();
                        std::replace(type.begin(),type.end(),' ','#');
                    }
                    KPLogSchema("%s %s %s %d %s %s %s\n",Var->getDirectory().str().c_str(),Var->getFilename().str().c_str(),"#global",Var->getLine(),Var->getName().str().c_str(),type.size()==0 ? "uintptr" : type.c_str(), "globalvar");
                    break;
                }
            }
        }
    }

    std::string shell2CppRegex(const std::string& shellPattern){
        std::string cppPattern = shellPattern;
        // Escape special characters in the shell pattern
        std::string specialchars = "\\^$*+?.()|{}[]";
        for(char c : specialchars){
            size_t pos = cppPattern.find(c);
            while (pos != std::string::npos){
                cppPattern.replace(pos, 1,"\\"+ std::string(1, c));
                pos = cppPattern.find(c, pos + 2);// Skip the escaped character
            }
        }
        size_t pos = cppPattern.find('*');
        while(pos != std::string::npos){
            cppPattern.replace(pos,1,".*");
            pos = cppPattern.find('*', pos + 2); // Move to the. next occurrence, skipping the '*'
        }
        // '?' in shell is equivalent to '.' in std::regex
        pos = cppPattern.find('?');
        while (pos != std::string::npos){
            cppPattern.replace(pos,1,".");
            pos = cppPattern.find('?', pos + 1);// Move to the next occurrence, skipping the '?
        }
        // Return the converted pattern
        return cppPattern;
    }

    std::string line_strip(const std::string& str){
        const std::string WHITESPACE =" \n\r\t";
        size_t start = str.find_first_not_of(WHITESPACE);
        if (start == std::string::npos) return "";// No non-whitespace characters
        size_t end = str.find_last_not_of(WHITESPACE);
        return str.substr(start, end - start + 1);
    }
} // namespace knobprofiler

#endif