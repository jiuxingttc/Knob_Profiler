#ifndef _KNOB_DEP_PASS_H
#define _KNOB_DEP_PASS_H

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <regex>

#include "llvm/Pass.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Operator.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/GraphTraits.h"
#include "llvm/ADT/iterator.h"
#include "llvm/ADT/SCCIterator.h"

#include "llvm/Analysis/CallGraph.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"

#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Demangle/Demangle.h"
#include "llvm/AsmParser/LLToken.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace knobprofiler {

    struct MyMDNode {
        MyMDNode() {}
        MyMDNode(MDNode *n, const std::string& t) : node(n),type(t) {}
        MDNode *node = nullptr;
        std::string type;
    };

    struct KnobDependencyPass : public ModulePass {
        static char ID;
        const int max_depth = 8;  // Maximum recursion depth
        std::unordered_set<BasicBlock*> recorded_bbs; // Record the basic blocks that have been processed
        std::unordered_set<Function*> recorded_fs; // Record the functions that have been processed
        std::unordered_set<std::string> taint_seeds; // Record the taint seeds
        std::vector<std::regex> taint_seeds_regex;  // Record the taint seeds in regex
        std::unordered_set<GlobalValue*> gloab_vars; // Record the global variables
        std::unordered_map<Function*, std::unordered_map<Value*,MyMDNode> > function_vars_cache; // Record the variables in functions
        KnobDependencyPass(); 
        KnobDependencyPass(const std::string &); 
        void parseConfigFile(const std::string &);
        void getAnalysisUsage(AnalysisUsage &) const override;
        bool runOnModule(Module &) override;
        void processSCC(const std::vector<CallGraphNode*> &,std::unordered_map<Value*,MyMDNode> &);
        void taint_analysis(Function *);
        void collectOnLoop(std::unordered_set<Function*>, std::unordered_map<Value*,MyMDNode> &);
        void collectOnLoop(Function *, std::unordered_map<Value*,MyMDNode> &);
        void collectOnCond(std::unordered_set<Function*>, std::unordered_map<Value*,MyMDNode> &);
        void collectOnCond(Function *, std::unordered_map<Value*,MyMDNode> &);
        void collectOnArg(std::unordered_set<Function*>, std::unordered_map<Value*,MyMDNode> &);
        void collectOnArg(Function *, std::unordered_map<Value*,MyMDNode> &);
        void collectOnMultiple(std::unordered_set<Function*>, std::unordered_map<Value*,MyMDNode> &);
        void collectOnMultiple(Function *, std::unordered_map<Value*,MyMDNode> &);
        bool checkOnMVAndEGV(Function *);
        bool taint_match(const std::string &);
        void getOperand(Value *, std::unordered_set<Value*> &, int);
        void extractVarMetaData(Value *, Function *, std::unordered_map<Value *, MyMDNode> &, const std::string&);
        void debug_print(const std::unordered_map<Value *,MyMDNode> &);
        bool brNeedCheck(const BranchInst *);
        void taint_filter(std::unordered_map<Value*, MyMDNode> &);
    };
}
#endif