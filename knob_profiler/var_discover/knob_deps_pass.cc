#include <cstdlib>
#include <queue>
#include <string>
#include <vector>
#include <utility>
#include <regex>
#include <fstream>
#include <iostream>
#include <unordered_set>
#include <unordered_map>

#include "knob_deps_pass.h"
#include "util.h"

using namespace llvm;
namespace knobprofiler {

    bool isclassScope(llvm::DIScope *Scope){
        // Walk up the scope hierarchy until we find a DICompositeType or reach the top
        while (Scope != nullptr){
            // Check if the current scope is a DICompositeType
            if (auto *CompositeType = llvm::dyn_cast<llvm::DICompositeType>(Scope)){
            // Check if the DICompositeType represents a class
                if(CompositeType->getTag()==llvm::dwarf::DW_TAG_class_type){
                    return true;
                }
            }
            // Move up to the parent scope
            Scope = Scope->getScope();
        }
        return false;   
    }

    std::vector<std::string> str_split(const std::string &str, const char sep='|'){
        std::vector<std::string> result;
        size_t st = 0, en = 0;
        while(1){
            en = str.find(sep, st);
            auto s = str.substr(st, en-st);
            if(s.size())
                result.push_back(std::move(s));
            if(en == std::string::npos)
                break;
            st = en+1;
        }
        return result;
    }

    std::string getEnv(const char *var){
        char *val = std::getenv(var);
        if(val == nullptr)
            return "Undefined";
        return val;
    }

    KnobDependencyPass::KnobDependencyPass() : ModulePass(ID) {
        this->taint_seeds.clear();
        parseConfigFile("/root/knob_profiler/test/knob_profiler_config_seed.txt");
    }

    KnobDependencyPass::KnobDependencyPass(const std::string &filename) : ModulePass(ID) {
        this->taint_seeds.insert("conf1");
        this->taint_seeds.insert("conf2");
    }

    void KnobDependencyPass::getAnalysisUsage(AnalysisUsage &AU) const {
        AU.addRequired<ScalarEvolutionWrapperPass>();
        AU.addRequired<LoopInfoWrapperPass>();
        AU.addRequired<CallGraphWrapperPass>();
        AU.setPreservesAll();
    }

    void KnobDependencyPass::parseConfigFile(const std::string &filename){
        std::ifstream file(filename);
        if(file.is_open()){
            std::string line_buffer;
            while(std::getline(file, line_buffer)){
                auto var_name = line_strip(line_buffer);
                if(var_name[0] == '#')
                    continue;
                size_t pos = var_name.find("*");
                if(pos != std::string::npos){
                    this->taint_seeds_regex.push_back(std::regex(shell2CppRegex(var_name)));
                }else{
                    this->taint_seeds.insert(var_name);
                }
            }
            file.close();
        }
    }

    bool KnobDependencyPass::runOnModule(Module &m) {
        outs()<< "#KnobDependencyPass::runOnModule module name: " << m.getName().str() << '\n';
        const std::string env_schema = getEnv("EnvSchema");
        if(env_schema == "Undefined"){
            auto all_file_substrings = str_split(env_schema);
            bool skip = true;
            for(auto &file_substring : all_file_substrings){
                if (m.getName().str().find(file_substring.c_str())!= std::string::npos){
                    skip = false;
                    break;
                }
            }
            if(skip) return false;
        }

        CallGraph& cg = getAnalysis<CallGraphWrapperPass>().getCallGraph();
        std::unordered_map<Value*,MyMDNode> result_vars;
        std::unordered_map<Function *,std::unordered_set<Function *>> dependency_graph;
        std::unordered_set<Function *> visited_functions;
        std::queue<Function *> fq;
        for (scc_iterator<CallGraph*> I = scc_begin(&cg);I != scc_end(&cg); ++I){
            const std::vector<CallGraphNode *> &SCC = *I;
            // pre-process rounds 
            for(CallGraphNode *CGN : SCC){
                std::unordered_map<Value*,MyMDNode> tmp_result_vars;
                Function *F=CGN->getFunction();
                if(visited_functions.find(F)!=visited_functions.end())continue;

                if (F && !F->isDeclaration()){
                    outs()<<"PreProcessing function: " << F->getName()<< "\n";

                    bool member_var_matched = checkOnMVAndEGV(F);
                    if(member_var_matched){
                        outs()<<"Function is a seed function: "<< F->getName()<<'\n';
                        fq.push(F);
                    } else {
                        collectOnLoop(F, tmp_result_vars);
                        //collectOnCond(F,tmp result vars);
                        //collectOnArg(F, tmp_result_vars);
                        collectOnMultiple(F, tmp_result_vars);
                        // check has taint seeds
                        for(auto & kv : tmp_result_vars){
                            DIVariable *tmp_node = dyn_cast<DIVariable>(kv.second.node);
                            auto name_ref = tmp_node->getName();
                            std::string var_name(name_ref.data(), name_ref.size());
                            outs()<< "in preprocessing rounds, we find variable: " << var_name << "\n";
                            if (taint_match(var_name)){
                                outs()<<"Function is a seed function:"<< F->getName()<<'\n';
                                fq.push(F);
                                break;
                            }
                        }
                    }
                    visited_functions.insert(F);
                    // Iterate over the calls from this function using CallGraph (next level only)
                    for(const auto &CallRecord : *cg[F]){
                        Function *Callee= CallRecord.second->getFunction();
                        if(Callee && !Callee->isDeclaration()){
                            if(dependency_graph.find(F)==dependency_graph.end()){
                                dependency_graph.insert(std::make_pair(F, std::unordered_set<Function *>({Callee})));
                            } else {
                                dependency_graph[F].insert(Callee);
                            }
                            //outs()<<" Calls:" << Callee->getName() << “\n";
                            //outs()<< "dependency edge: " << F->getName() << "_" <<. Callee->getName() << "\n";
                        }
                    }
                }
            } // for CallGraphNode in SCC
        } // for SCCs

        visited_functions.clear();
        while (fq.size()> 0){
            Function* func = fq.front();
            outs()<< "processing func: " << func->getName() << "\n";
            fq.pop();
            std::unordered_map<Value*, MyMDNode> tmp_result_vars;
            if (visited_functions.find(func)== visited_functions.end()){
                collectOnLoop(func, tmp_result_vars);
                //collectOnCond(func, tmp_result_vars);
                //collectOnArg(func, tmp_result_vars);
                collectOnMultiple(func, tmp_result_vars);
                if (tmp_result_vars.size()> 0){
                    result_vars.insert(tmp_result_vars.begin(), tmp_result_vars.end());
                    outs() << "collected risky vars: " << tmp_result_vars.size() << " from function: " << func->getName() << "\n";
                    SaveResult(func, tmp_result_vars);
                }
                visited_functions.insert(func);
            }
            if(dependency_graph.find(func)== dependency_graph.end()) continue;
            for (auto dep_f : dependency_graph[func]){
                tmp_result_vars.clear();
                if (visited_functions.find(dep_f)!= visited_functions.end()){
                    outs()<< " de-duplicate fukc" << dep_f->getName()<< "\n";
                    continue;
                }
                collectOnLoop(dep_f, tmp_result_vars);
                //collectOnCond(dep_f, tmp_result_vars);
                //collectOnArg(dep_f, tmp_result_vars);
                collectOnMultiple(dep_f, tmp_result_vars);
                visited_functions.insert(dep_f);
                if (tmp_result_vars.size()> 0){
                    result_vars.insert(tmp_result_vars.begin(), tmp_result_vars.end());
                    outs()<< "collected risky vars: " << tmp_result_vars.size() << " from function: " << dep_f->getName() << "\n";
                    SaveResult(dep_f, tmp_result_vars);
                }
                fq.push(dep_f);
                outs()<< " processing sub func: "<< dep_f->getName()<<"\n";
            }
        }

        SaveGlobalResult(m, this->gloab_vars);
        debug_print(result_vars);
        return false;
    }

    void KnobDependencyPass::processSCC(const std::vector<CallGraphNode *> &SCC, std::unordered_map<Value*, MyMDNode> &vars){
        for(CallGraphNode *CGN : SCC){
            Function *F = CGN->getFunction();
            if(F && !F->isDeclaration()){
                // outs()<<"Processing function: " << F->getName()<< "\n";
                LoopInfo &LI = getAnalysis<LoopInfoWrapperPass>(*F).getLoopInfo();
                if(!LI.empty()){
                    collectOnLoop(F, vars);
                }
                collectOnCond(F, vars);
                collectOnArg(F, vars);
            }
        }
    }

    void KnobDependencyPass::taint_analysis(Function *F){
        LoopInfo &LI = getAnalysis<LoopInfoWrapperPass>(*F).getLoopInfo();
        ScalarEvolution &SE = getAnalysis<ScalarEvolutionWrapperPass>(*F).getSE();
        for(BasicBlock &BB : *F){
            errs()<< "Basic Block: " << BB.getName()<< "\n";
            for(Instruction &I : BB){
                const SCEV *S = SE.getSCEV(&I);
                outs()<< " Instruction: " << I << "\n";
                outs()<<" Scalar Evolution: " << *S << "\n";
            }
            outs()<< "\n";
        }
    }

    void KnobDependencyPass::collectOnLoop(std::unordered_set<Function *> Fs, std::unordered_map<Value *, MyMDNode> &vars) {
        for (auto F: Fs){
            LoopInfo &LI = getAnalysis<LoopInfoWrapperPass>(*F).getLoopInfo();
            if (!LI.empty()){
                this->collectOnLoop(F, vars);
            }
        }
    }

    void KnobDependencyPass::collectOnLoop(Function *F, std::unordered_map<Value *, MyMDNode> &vars) {
        LoopInfo& LI = getAnalysis<LoopInfoWrapperPass>(*F).getLoopInfo();
        ScalarEvolution& SE = getAnalysis<ScalarEvolutionWrapperPass>(*F).getSE();
        std::string var_type = "loop";
        for(Loop *L : LI){
            PHINode *Phi_outer =L->getCanonicalInductionVariable();
            if(Phi_outer){
                extractVarMetaData(dyn_cast<Value>(Phi_outer),F, vars, var_type);
            }

            for(BasicBlock *BB :L->blocks()){
                for(Instruction &I : *BB){
                    for(User::op_iterator O = I.op_begin(); O!= I.op_end(); ++O){
                        Value *o = dyn_cast<Value>(&*O);
                        if(!o||!SE.isSCEVable(o->getType())) continue;
                        const SCEV *scval = SE.getSCEV(o);
                        if(scval && isa<SCEVAddRecExpr>(scval)){
                            extractVarMetaData(o, F, vars, var_type);
                        }
                    }
                    if(BinaryOperator *BO = dyn_cast<BinaryOperator>(&I)){
                        if(BO->getOpcode() == Instruction::Add || BO->getOpcode() == Instruction::Sub|| BO->getOpcode() == Instruction::Mul|| BO->getOpcode() == Instruction::UDiv|| BO->getOpcode() == Instruction::SDiv){
                            Value *Op0 = BO->getOperand(0);
                            Value *Op1 = BO->getOperand(1);
                            if(ConstantInt *C = dyn_cast<ConstantInt>(Op1)){
                                extractVarMetaData(Op0, F, vars, var_type);
                            }
                        }
                    }
                    if(PHINode *Phi = dyn_cast<PHINode>(&I)){
                        // Check if.the PHI node is an induction variable
                        const SCEV *PhiSCEV = SE.getSCEV(Phi);
                        if (SE.isSCEVable(Phi->getType())&& SE.isLoopInvariant(PhiSCEV, L)){
                            // outs()<< "Induction variable found: " << *Phi << "\n"";
                            extractVarMetaData(dyn_cast<Value>(Phi),F, vars, var_type);
                        }
                    }
                } //BB
            }
        } // Loop
    }

    void KnobDependencyPass::collectOnCond(std::unordered_set<Function *> Fs, std::unordered_map<Value *, MyMDNode> &vars){
        for(auto F : Fs){
            this->collectOnCond(F, vars);
        }
    }

    void KnobDependencyPass::collectOnCond(Function *F, std::unordered_map<Value *, MyMDNode> &vars){
        for(auto &BB : *F){
            if(this->recorded_bbs.find(&BB)== this->recorded_bbs.end()){
                SaveBlockInfo(&BB);
                this->recorded_bbs.insert(&BB);
            }
            for(auto &Inst : BB){
                //Branch instruction
                if(BranchInst *BI = dyn_cast<BranchInst>(&Inst)){
                    if(BI->isConditional()&&brNeedCheck(BI)){
                        Value* cond = BI->getCondition();
                        std::string var_type = "cond";
                        if(cond && isa<CmpInst>(cond)){
                            extractVarMetaData(cond,F, vars, var_type);
                        } else {
                            extractVarMetaData(dyn_cast<Value>(BI),F, vars, var_type);
                        }
                    }
                }
            }
        }
        if (this->recorded_fs.find(F)== this->recorded_fs.end()){
            SaveFunctionInfo(F);
            this->recorded_fs.insert(F);
        }
    }

    void KnobDependencyPass::collectOnArg(std::unordered_set<Function *> Fs, std::unordered_map<Value *, MyMDNode> &vars){
        for(auto F : Fs){
            this->collectOnArg(F, vars);
        }
    }

    void KnobDependencyPass::collectOnArg(Function *F, std::unordered_map<Value *, MyMDNode> &vars){
        for(auto &BB : *F){
            for(auto &Inst : BB){
                if(auto *call = dyn_cast<CallBase>(&Inst)){
                    for(auto ArgIt = call->arg_begin(); ArgIt != call->arg_end(); ++ArgIt){
                        Value *Arg = *ArgIt;
                        std::string var_type = "arg";
                        extractVarMetaData(Arg, F, vars, var_type);
                    }
                }
            }
        }
    }

    void KnobDependencyPass::collectOnMultiple(std::unordered_set<Function *> Fs, std::unordered_map<Value *, MyMDNode> &vars){
        for(auto F : Fs){
            LoopInfo &LI = getAnalysis<LoopInfoWrapperPass>(*F).getLoopInfo();
            if(!LI.empty()){
                this->collectOnLoop(F, vars);
            }
            this->collectOnMultiple(F, vars);
        }
    }

    void KnobDependencyPass::collectOnMultiple(Function *F, std::unordered_map<Value *, MyMDNode> &vars){
        for(auto &BB : *F){
            if(this->recorded_bbs.find(&BB)== this->recorded_bbs.end()){
                SaveBlockInfo(&BB);
                this->recorded_bbs.insert(&BB);
            }
            for(auto &Inst : BB){
                if(BranchInst *BI = dyn_cast<BranchInst>(&Inst)){
                    if(BI->isConditional()&& brNeedCheck(BI)){
                        Value *cond = BI->getCondition();
                        std::string var_type = "cond";
                        if(cond && isa<CmpInst>(cond)){
                            extractVarMetaData(cond, F, vars, var_type);
                        } else {
                            extractVarMetaData(dyn_cast<Value>(BI), F, vars, var_type);
                        }
                    }
                }
                if(auto *call = dyn_cast<CallBase>(&Inst)){
                    for(auto ArgIt = call->arg_begin(); ArgIt != call->arg_end(); ++ArgIt){
                        Value *Arg = *ArgIt;
                        std::string var_type = "arg";
                        extractVarMetaData(Arg, F, vars, var_type);
                    }
                }
            } // for Inst
        } // for BB
        if(this->recorded_fs.find(F)== this->recorded_fs.end()){
            SaveFunctionInfo(F);
            this->recorded_fs.insert(F);
        }
    }

    bool KnobDependencyPass::checkOnMVAndEGV(Function *F){
        for(auto &BB : *F){
            for(auto &I : BB){
                if(auto *loadInst = llvm::dyn_cast<llvm::LoadInst>(&I)){
                    llvm::Value *op = loadInst->getPointerOperand();
                    if(auto *globalVar = llvm::dyn_cast<llvm::GlobalVariable>(op)){
                        if(globalVar->hasExternalLinkage()){
                            std::string global_var_name = globalVar->getName().str();
                            if(taint_match(global_var_name)){
                                outs()<< "external global variable matched: " << global_var_name << '\n';
                                return true;
                            }
                        }
                    }
                }
                if(auto *GEP = dyn_cast<GetElementPtrInst>(&I)){
                    if(GEP->getNumOperands()<3) continue;
                    bool valid = false;
                    llvm::Type *BaseType = GEP->getPointerOperandType()->getPointerElementType();
                    if(!BaseType->isStructTy()) continue;
                    if(llvm::MDNode *Metadata = GEP->getMetadata(llvm::LLVMContext::MD_dbg)){
                        if (auto *DILoc = llvm::dyn_cast<llvm::DILocation>(Metadata)){
                            if(DILoc){
                                DIScope *Scope = DILoc->getScope();
                                if(isclassScope(Scope)){
                                    valid = true;
                                }
                            }
                        }
                    }
                    if(!valid) continue;

                    llvm::ConstantInt *fieldIndexOperand = llvm::dyn_cast<llvm::ConstantInt>(GEP->getOperand(2));
                    auto index = fieldIndexOperand->getZExtValue();
                    if (llvm::MDNode *metadata = GEP->getMetadata(llvm::LLVMContext::MD_dbg)){
                        if(auto *dbgLoc = llvm::dyn_cast<llvm::DILocation>(metadata)){
                            llvm::DIScope *scope = dbgLoc->getScope();
                            // Walk up to find the DICompositeType corresponding to the structure
                            while (scope && !llvm::isa<llvm::DICompositeType>(scope)){
                                scope = scope->getScope();
                            }
                            if (auto *structType = llvm::dyn_cast<llvm::DICompositeType>(scope)){
                                // Get the field from the structure by index
                                llvm::DINodeArray elements = structType->getElements();
                                if (index >= elements.size()) continue;
                                llvm::DINode *NodeAtIndex = elements[index];
                                if (auto *field = llvm::dyn_cast<llvm::DIDerivedType>(NodeAtIndex)){
                                    std::string var_name = field->getName().str();
                                    if(taint_match(var_name)){
                                        outs()<< "class member variable matched: " << var_name << '\n';
                                        return true;
                                    }
                                }
                            }
                        }
                    }
                } 
            } // for Inst
        } // for BB
        return false;
    }

    void KnobDependencyPass::getOperand(Value *V, std::unordered_set<Value *> &operands, int depth){
        if(!V || depth >= this->max_depth) return;
        depth++;
        if(Instruction *I = dyn_cast<Instruction>(V)){
            for(int i = 0; i < I->getNumOperands(); ++i){
               getOperand(I->getOperand(i), operands, depth);
            }
        }
        if(GEPOperator *I = dyn_cast<GEPOperator>(V)){
            for(auto Iter = I->idx_begin(); Iter != I->idx_end(); ++Iter){
                if(!dyn_cast<ConstantInt>(&*Iter)){
                    if(Value *V = dyn_cast<Value>(&*Iter)){
                        operands.insert(V);
                    }
                }
            }
        }
        operands.insert(V);
    }

    void KnobDependencyPass::extractVarMetaData(Value *V, Function *F, std::unordered_map<Value *, MyMDNode> &vars, const std::string &var_type){
        if(!V) return;
        bool terminate = false;
        std::unordered_set<Value *> operands;
        getOperand(V, operands, 0);
        for(auto &op : operands){
            if(GlobalValue *GV = dyn_cast<GlobalValue>(&*op)){
                gloab_vars.insert(GV);
            }
            terminate = false;
            if(op){
                for(auto &BB: *F){
                    for(auto &Inst : BB){
                        if(const DbgDeclareInst *DbgDeclare = dyn_cast<DbgDeclareInst>(&Inst)){
                            if(DbgDeclare->getAddress() == op){
                                if(DbgDeclare->getVariable()->getName() == "this") continue;
                                if(vars.find(op)!= vars.end()){
                                    if (vars[op].type.find(var_type)== std::string::npos){
                                        vars[op].type += "|" + var_type;
                                    }
                                }else{
                                    vars[op]=MyMDNode(DbgDeclare->getVariable(), var_type);
                                }
                                terminate = true;
                                break;
                            }
                        }else if(const DbgValueInst *DbgValue = dyn_cast<DbgValueInst>(&Inst)){
                            if(DbgValue->getValue() == op){
                                if(DbgValue->getVariable()->getName() == "this") continue;
                                if(vars.find(op)!= vars.end()){
                                    if(vars[op].type.find(var_type)== std::string::npos){
                                        vars[op].type += "|" + var_type;
                                    }
                                }else{
                                    vars[op]=MyMDNode(DbgValue->getVariable(), var_type);
                                }
                                terminate = true;
                                break;
                            }
                        }
                    }
                    if(terminate) break;
                }
            }
        }
    }

    bool KnobDependencyPass::taint_match(const std::string &var_name){
        if(this->taint_seeds.find(var_name)!= this->taint_seeds.end()){
            return true;
        }
        for(auto &regex : this->taint_seeds_regex){
            if(std::regex_search(var_name, regex)){
                return true;
            }
        }
        return false;
    }

    bool  KnobDependencyPass::brNeedCheck(const BranchInst *BI){
       unsigned int n = BI->getNumSuccessors();
       if(n < 2) return false;
       unsigned int i;
       for(i = 0; i < n; i++){
           BasicBlock *Block = BI->getSuccessor(i);
           if(Block == NULL) continue;
           for(BasicBlock::iterator Iter = Block->begin(); Iter != Block->end(); ++Iter){
               const Instruction *I = &*Iter;
               if(isa<BranchInst>(I)||isa<CallInst>(I)||isa<InvokeInst>(I)){
                   return true;
               }
           }
       }
       return false;
    }

    void KnobDependencyPass::debug_print(const std::unordered_map<Value *, MyMDNode> &vars){
        if(vars.size() == 0){
            return;
        }
        int count = 0;
        for(auto &kv : vars){
           MDNode *var= kv.second.node;
            if(!var){
                    outs()<< "empty variable" << '\n';
                    return;
            }
            DIVariable *node = dyn_cast<DIVariable>(var);
            if(!node){
                outs()<< "non DIVariable" << '\n';
                return;
            }
            outs()<< "IR Inst:" << *kv.first <<"|";
            outs()<< node->getDirectory()<<"|";
            outs()<< node->getFilename() <<"|";
            outs()<< node->getLine()<<"|";
            outs()<< node->getName()<<"|";
            std::string type("uintptr");
            if (node->getType()){
                std::string type1 = node->getType()->getName().str();
                std::replace(type1.begin(), type1.end(),' ','|');
                if (type1.size()>0 && type1 !=" ") type = type1;
            }
            outs()<< type << "|";
            outs()<< kv.second.type << '\n';
        }
        for(auto &gv : this->gloab_vars){
            outs()<< "found Global Variable: " << gv->getName() << '\n';
        }
    }

    void KnobDependencyPass::taint_filter(std::unordered_map<Value*, MyMDNode> &vars){}

    char KnobDependencyPass::ID = 0;
    static RegisterPass<KnobDependencyPass> X("knobDependencyPass", "Knob Dependency Pass", false, false);
    static void registerknobDependencyPass(const PassManagerBuilder &, legacy::PassManagerBase &PM){
        PM.add(new CallGraphWrapperPass);
        PM.add(new LoopInfoWrapperPass);
        PM.add(new ScalarEvolutionWrapperPass);
        PM.add(new KnobDependencyPass());
    }

    static RegisterStandardPasses Y(PassManagerBuilder::EP_ModuleOptimizerEarly, registerknobDependencyPass);
    static RegisterStandardPasses Z(PassManagerBuilder::EP_EnabledOnOptLevel0, registerknobDependencyPass);

        
} // namespace knobprofiler