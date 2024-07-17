#include <cstdlib>
#include <fstream>
#include <iostream>
#include <queue>
#include <regex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "knob_deps_pass.h"
#include "util.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;
namespace knobprofiler {

/**
    Determines whether the given llvm::DIScope is in class scope.
    @param Scope The llvm::DIScope to check.
    @return True if the llvm::DIScope is in class scope, false otherwise.
*/
bool isClassScope(llvm::DIScope *Scope) {
  // Walk up the scope hierarchy until we find a DICompositeType or reach the
  // top
  while (Scope != nullptr) {
    // Check if the current scope is a DICompositeType
    if (auto *CompositeType = llvm::dyn_cast<llvm::DICompositeType>(Scope)) {
      // Check if the DICompositeType represents a class
      if (CompositeType->getTag() == llvm::dwarf::DW_TAG_class_type) {
        return true;
      }
    }
    // Move up to the parent scope
    Scope = Scope->getScope();
  }
  return false;
}

/**
 * Splits a string into a vector of substrings based on a delimiter.
 * @param str The input string to split.
 * @param sep The delimiter character (default is '|').
 * @return A vector of substrings.
 */
std::vector<std::string> str_split(const std::string &str,
                                   const char sep = '|') {
  std::vector<std::string> result;
  size_t st = 0, en = 0;
  while (1) {
    en = str.find(sep, st);
    auto s = str.substr(st, en - st);
    if (s.size())
      result.push_back(std::move(s));
    if (en == std::string::npos)
      break;
    st = en + 1;
  }
  return result;
}

/**
 * Retrieves the value of the specified environment variable.
 *
 * @param var The name of the environment variable.
 * @return The value of the environment variable if it exists, otherwise returns
 * "Undefined".
 */
std::string getEnv(const char *var) {
  char *val = std::getenv(var);
  if (val == nullptr)
    return "Undefined";
  return val;
}

/**
 * @brief Constructs a KnobDependencyPass object.
 *
 * This constructor initializes a KnobDependencyPass object by clearing the
 * taint_seeds vector and parsing a configuration file to populate the
 * taint_seeds vector.
 *
 * @param None
 * @return None
 */
KnobDependencyPass::KnobDependencyPass() : ModulePass(ID) {
  this->taint_seeds.clear();
  parseConfigFile(
      "/home/LLVM_MYSQL/knob_profiler/test/knob_profiler_config_seed.txt");
}

/**
 * @brief Constructs a KnobDependencyPass object.
 *
 * This constructor initializes a KnobDependencyPass object with the specified
 * filename. It also inserts two taint seeds, "conf1" and "conf2", into the
 * taint_seeds set.
 *
 * @param filename The filename associated with the KnobDependencyPass object.
 */
KnobDependencyPass::KnobDependencyPass(const std::string &filename)
    : ModulePass(ID) {
  this->taint_seeds.insert("conf1");
  this->taint_seeds.insert("conf2");
}

/**
 * This function is used to specify the analysis passes that are required by the
 * KnobDependencyPass. It adds the required passes to the AnalysisUsage object.
 *
 * @param AU The AnalysisUsage object to which the required passes are added.
 */
void KnobDependencyPass::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<ScalarEvolutionWrapperPass>();
  AU.addRequired<LoopInfoWrapperPass>();
  AU.addRequired<CallGraphWrapperPass>();
  AU.setPreservesAll();
}

/**
 * Parses a configuration file and extracts knob dependencies.
 *
 * @param filename The path to the configuration file.
 */
void KnobDependencyPass::parseConfigFile(const std::string &filename) {
  std::ifstream file(filename);
  if (file.is_open()) {
    std::string line_buffer;
    while (std::getline(file, line_buffer)) {
      auto var_name = line_strip(line_buffer);
      if (var_name[0] == '#')
        continue;
      size_t pos = var_name.find("*");
      if (pos != std::string::npos) {
        this->taint_seeds_regex.push_back(std::regex(shell2CppRegex(var_name)));
      } else {
        this->taint_seeds.insert(var_name);
      }
    }
    file.close();
  }
}

/**
 * Runs the KnobDependencyPass on the given module.
 *
 * @param m The module to run the pass on.
 * @return Returns false to indicate that the pass did not modify the module.
 */
bool KnobDependencyPass::runOnModule(Module &m) {
  outs() << "#KnobDependencyPass::runOnModule module name: "
         << m.getName().str() << '\n';
  const std::string env_schema = getEnv("EnvSchema");
  outs() << "EnvSchema: " << env_schema << '\n';
  if (env_schema == "Undefined") {
    auto all_file_substrings = str_split(env_schema);
    bool skip = true;
    for (auto &file_substring : all_file_substrings) {
      if (m.getName().str().find(file_substring.c_str()) != std::string::npos) {
        skip = false;
        break;
      }
    }
    if (skip)
      return false;
  }

  CallGraph &cg = getAnalysis<CallGraphWrapperPass>().getCallGraph();
  std::unordered_map<Value *, MyMDNode> result_vars;
  std::unordered_map<Function *, std::unordered_set<Function *>>
      dependency_graph;
  std::unordered_set<Function *> visited_functions;
  std::queue<Function *> fq;
  std::unordered_map<Value *, std::vector<Instruction *>> varUsagePoints;

  for (scc_iterator<CallGraph *> I = scc_begin(&cg); I != scc_end(&cg); ++I) {
    const std::vector<CallGraphNode *> &SCC = *I;
    // pre-process rounds
    for (CallGraphNode *CGN : SCC) {
      std::unordered_map<Value *, MyMDNode> tmp_result_vars;
      std::unordered_map<Value *, std::unordered_set<BasicBlock *>> varToBB;
      Function *F = CGN->getFunction();
      if (visited_functions.find(F) != visited_functions.end())
        continue;

      if (F && !F->isDeclaration()) {
        outs() << "PreProcessing function: " << F->getName() << "\n";
        bool member_var_matched = checkOnMVAndEGV(F);
        if (member_var_matched) {
          outs() << "Function is a seed function: " << F->getName() << '\n';
          fq.push(F);
        } else {
          // collectOnLoop(F, tmp_result_vars);
          // // collectOnCond(F,tmp result vars);
          // // collectOnArg(F, tmp_result_vars);
          // collectOnMultiple(F, tmp_result_vars);
          collectOnLoop(F, tmp_result_vars, varToBB);
          collectOnMultiple(F, tmp_result_vars, varToBB);
          // check has taint seeds
          for (auto &kv : tmp_result_vars) {
            DIVariable *tmp_node = dyn_cast<DIVariable>(kv.second.node);
            auto name_ref = tmp_node->getName();
            std::string var_name(name_ref.data(), name_ref.size());
            outs() << "in preprocessing rounds, we find variable: " << var_name
                   << "\n";
            if (taint_match(var_name)) {
              outs() << "Function is a seed function:" << F->getName() << '\n';
              fq.push(F);
              break;
            }
          }
        }
        visited_functions.insert(F);
        // Iterate over the calls from this function using CallGraph (next level
        // only)
        for (const auto &CallRecord : *cg[F]) {
          Function *Callee = CallRecord.second->getFunction();
          if (Callee && !Callee->isDeclaration()) {
            if (dependency_graph.find(F) == dependency_graph.end()) {
              dependency_graph.insert(
                  std::make_pair(F, std::unordered_set<Function *>({Callee})));
            } else {
              dependency_graph[F].insert(Callee);
            }
            // outs()<<" Calls:" << Callee->getName() << “\n";
            // outs() << "dependency edge: " << F->getName() << "_"
            //        << Callee->getName() << "\n";
          }
        }
      }
    } // for CallGraphNode in SCC
  }   // for SCCs

  visited_functions.clear();
  while (fq.size() > 0) {
    Function *func = fq.front();
    outs() << "processing func: " << func->getName() << "\n";
    fq.pop();
    std::unordered_map<Value *, MyMDNode> tmp_result_vars;
    std::unordered_map<Value *, std::unordered_set<BasicBlock *>> varToBB;
    if (visited_functions.find(func) == visited_functions.end()) {
      // collectOnLoop(func, tmp_result_vars);
      // // collectOnCond(func, tmp_result_vars);
      // // collectOnArg(func, tmp_result_vars);
      // collectOnMultiple(func, tmp_result_vars);
      collectOnLoop(func, tmp_result_vars, varToBB);
      collectOnMultiple(func, tmp_result_vars, varToBB);
      if (tmp_result_vars.size() > 0) {
        result_vars.insert(tmp_result_vars.begin(), tmp_result_vars.end());
        outs() << "collected risky vars: " << tmp_result_vars.size()
               << " from function: " << func->getName() << "\n";
        // SaveResult(func, tmp_result_vars);
        SaveResult(func, tmp_result_vars, varToBB);
        // ############################
        collectVarUsagePoints(tmp_result_vars, func, varUsagePoints);
        // 构建并输出所有变量的执行路径
        std::unordered_map<Value *, std::vector<std::string>>
            varExecutionPaths = buildVarExecutionPath(
                tmp_result_vars, varUsagePoints, dependency_graph);
        // 输出所有变量的执行路径
        for (const auto &varPathPair : varExecutionPaths) {
          Value *var = varPathPair.first;
          const std::vector<std::string> &executionPath = varPathPair.second;
          if (DIVariable *node =
                  dyn_cast<DIVariable>(tmp_result_vars[var].node)) {
            outs() << "Execution Path for variable: "
                   << node->getName().str().c_str() << "\n";
          } else {
            outs() << "Execution Path for variable: " << var << "\n";
          }
          for (const auto &step : executionPath) {
            outs() << step << "\n";
          }
        }
        SaveExecutionPath(func, tmp_result_vars, varExecutionPaths);
        // ############################
        outs() << "varToBB size: " << varToBB.size() << "\n";
        for (const auto &pair : varToBB) {
          Value *var = pair.first;
          const auto &bbs = pair.second;
          if (tmp_result_vars.find(var) != tmp_result_vars.end()) {
            DIVariable *node = dyn_cast<DIVariable>(tmp_result_vars[var].node);
            outs() << "Variable: " << node->getName().str().c_str()
                   << " is found in blocks: ";
            for (const auto *BB : bbs) {
              outs() << getSimplebbLabel(BB).c_str() << " ";
            }
            outs() << "\n";
          } else {
            continue;
          }
        }
        // ############################
      }
      visited_functions.insert(func);
    }
    if (dependency_graph.find(func) == dependency_graph.end())
      continue;
    for (auto dep_f : dependency_graph[func]) {
      tmp_result_vars.clear();
      if (visited_functions.find(dep_f) != visited_functions.end()) {
        outs() << " de-duplicate fukc" << dep_f->getName() << "\n";
        continue;
      }
      // collectOnLoop(dep_f, tmp_result_vars);
      // // collectOnCond(dep_f, tmp_result_vars);
      // // collectOnArg(dep_f, tmp_result_vars);
      // collectOnMultiple(dep_f, tmp_result_vars);
      collectOnLoop(dep_f, tmp_result_vars, varToBB);
      collectOnMultiple(dep_f, tmp_result_vars, varToBB);
      visited_functions.insert(dep_f);
      if (tmp_result_vars.size() > 0) {
        result_vars.insert(tmp_result_vars.begin(), tmp_result_vars.end());
        outs() << "collected risky vars: " << tmp_result_vars.size()
               << " from function: " << dep_f->getName() << "\n";
        // SaveResult(dep_f, tmp_result_vars);
        SaveResult(dep_f, tmp_result_vars, varToBB);
        // ############################
        collectVarUsagePoints(tmp_result_vars, dep_f, varUsagePoints);
        // 构建并输出所有变量的执行路径
        std::unordered_map<Value *, std::vector<std::string>>
            varExecutionPaths = buildVarExecutionPath(
                tmp_result_vars, varUsagePoints, dependency_graph);

        // 输出所有变量的执行路径
        for (const auto &varPathPair : varExecutionPaths) {
          Value *var = varPathPair.first;
          const std::vector<std::string> &executionPath = varPathPair.second;
          if (DIVariable *node =
                  dyn_cast<DIVariable>(tmp_result_vars[var].node)) {
            outs() << "Execution Path for variable: "
                   << node->getName().str().c_str() << "\n";
          } else {
            outs() << "Execution Path for variable: " << var << "\n";
          }
          for (const auto &step : executionPath) {
            outs() << step << "\n";
          }
        }
        SaveExecutionPath(dep_f, tmp_result_vars, varExecutionPaths);
        // ############################
        for (const auto &pair : varToBB) {
          Value *var = pair.first;
          const auto &bbs = pair.second;
          if (tmp_result_vars.find(var) != tmp_result_vars.end()) {
            DIVariable *node = dyn_cast<DIVariable>(tmp_result_vars[var].node);
            outs() << "Variable: " << node->getName().str().c_str()
                   << " is found in blocks: ";
            for (const auto *BB : bbs) {
              outs() << getSimplebbLabel(BB).c_str() << " ";
            }
            outs() << "\n";
          } else {
            continue;
          }
        }
        // ############################
      }
      fq.push(dep_f);
      outs() << " processing sub func: " << dep_f->getName() << "\n";
    }
  }
  SaveGlobalResult(m, this->gloab_vars);
  debug_print(result_vars);
  return false;
}

/**
 * Process the Strongly Connected Component (SCC) of call graph nodes.
 *
 * This function iterates over each call graph node in the SCC and performs
 * various operations on the associated function, such as collecting
 * information on loops, conditions, and arguments.
 *
 * @param SCC The vector of call graph nodes in the SCC.
 * @param vars The unordered map of variables and their associated metadata
 * nodes.
 */
void KnobDependencyPass::processSCC(
    const std::vector<CallGraphNode *> &SCC,
    std::unordered_map<Value *, MyMDNode> &vars) {
  for (CallGraphNode *CGN : SCC) {
    Function *F = CGN->getFunction();
    if (F && !F->isDeclaration()) {
      // outs()<<"Processing function: " << F->getName()<< "\n";
      LoopInfo &LI = getAnalysis<LoopInfoWrapperPass>(*F).getLoopInfo();
      if (!LI.empty()) {
        collectOnLoop(F, vars);
      }
      collectOnCond(F, vars);
      collectOnArg(F, vars);
    }
  }
}

/**
 * Performs taint analysis on the given function.
 * This function iterates over the basic blocks and instructions of the
 * function, printing the name of each basic block and the scalar evolution of
 * each instruction.
 *
 * @param F The function to perform taint analysis on.
 */
void KnobDependencyPass::taint_analysis(Function *F) {
  LoopInfo &LI = getAnalysis<LoopInfoWrapperPass>(*F).getLoopInfo();
  ScalarEvolution &SE = getAnalysis<ScalarEvolutionWrapperPass>(*F).getSE();
  for (BasicBlock &BB : *F) {
    errs() << "Basic Block: " << BB.getName() << "\n";
    for (Instruction &I : BB) {
      const SCEV *S = SE.getSCEV(&I);
      outs() << " Instruction: " << I << "\n";
      outs() << " Scalar Evolution: " << *S << "\n";
    }
    outs() << "\n";
  }
}

// ######################################################################

void KnobDependencyPass::collectVarUsagePoints(
    std::unordered_map<Value *, MyMDNode> &vars, Function *F,
    std::unordered_map<Value *, std::vector<Instruction *>> &varUsagePoints) {
  for (auto &BB : *F) {
    for (auto &I : BB) {
      for (auto &var : vars) {
        if (I.mayReadOrWriteMemory() || isa<CallBase>(&I)) {
          for (unsigned op = 0; op < I.getNumOperands(); ++op) {
            if (I.getOperand(op) == var.first) {
              varUsagePoints[var.first].push_back(&I);
              break;
            }
          }
        }
      }
    }
  }
  outs() << "varUsagePoints size: " << varUsagePoints.size() << "\n";
}

std::unordered_map<Value *, std::vector<std::string>>
KnobDependencyPass::buildVarExecutionPath(
    const std::unordered_map<Value *, MyMDNode> &result_vars,
    const std::unordered_map<Value *, std::vector<Instruction *>>
        &varUsagePoints,
    const std::unordered_map<Function *, std::unordered_set<Function *>>
        &dependencyGraph) {

  std::unordered_map<Value *, std::vector<std::string>> varExecutionPaths;

  for (const auto &var_pair : result_vars) {
    Value *var = var_pair.first;
    // MDNode *m = var_pair.second.node;
    // if (!m) {
    //   continue;
    // }
    // DIVariable *node = dyn_cast<DIVariable>(m);
    // if (!node) {
    //   continue;
    // }
    std::vector<std::string> executionPath;
    std::unordered_set<std::string> uniqueFunctions;

    // 确保 varUsagePoints 包含 var
    if (varUsagePoints.find(var) == varUsagePoints.end()) {
      varExecutionPaths[var] = executionPath;
      continue;
    }
    // 获取变量的所有使用点
    const std::vector<Instruction *> &usagePoints = varUsagePoints.at(var);

    for (Instruction *instr : usagePoints) {
      Function *parentFunc = instr->getFunction();
      if (parentFunc) {
        std::string funcName = parentFunc->getName().str();
        if (uniqueFunctions.insert(funcName).second) {
          executionPath.push_back(funcName);
        }
      }
    }

    // 继续处理依赖的函数
    for (const auto &kv : dependencyGraph) {
      Function *caller = kv.first;
      const std::unordered_set<Function *> &callees = kv.second;
      if (uniqueFunctions.find(caller->getName().str()) !=
          uniqueFunctions.end()) {
        for (Function *callee : callees) {
          std::string calleeName = callee->getName().str();
          if (uniqueFunctions.insert(calleeName).second) {
            executionPath.push_back(calleeName);
          }
        }
      }
    }

    varExecutionPaths[var] = executionPath;
  }

  return varExecutionPaths;
}

void KnobDependencyPass::collectOnLoop(
    Function *F, std::unordered_map<Value *, MyMDNode> &vars,
    std::unordered_map<Value *, std::unordered_set<BasicBlock *>> &varToBB) {
  LoopInfo &LI = getAnalysis<LoopInfoWrapperPass>(*F).getLoopInfo();
  ScalarEvolution &SE = getAnalysis<ScalarEvolutionWrapperPass>(*F).getSE();
  std::string var_type = "loop";

  for (Loop *L : LI) {
    PHINode *Phi_outer = L->getCanonicalInductionVariable();
    if (Phi_outer) {
      extractVarMetaData(dyn_cast<Value>(Phi_outer), F, vars, var_type,
                         varToBB);
    }

    for (BasicBlock *BB : L->blocks()) {
      for (Instruction &I : *BB) {
        for (User::op_iterator O = I.op_begin(); O != I.op_end(); ++O) {
          Value *o = dyn_cast<Value>(&*O);
          if (!o || !SE.isSCEVable(o->getType()))
            continue;
          const SCEV *scval = SE.getSCEV(o);
          if (scval && isa<SCEVAddRecExpr>(scval)) {
            extractVarMetaData(o, F, vars, var_type, varToBB);
          }
        }
        if (BinaryOperator *BO = dyn_cast<BinaryOperator>(&I)) {
          if (BO->getOpcode() == Instruction::Add ||
              BO->getOpcode() == Instruction::Sub ||
              BO->getOpcode() == Instruction::Mul ||
              BO->getOpcode() == Instruction::UDiv ||
              BO->getOpcode() == Instruction::SDiv) {
            Value *Op0 = BO->getOperand(0);
            Value *Op1 = BO->getOperand(1);
            if (ConstantInt *C = dyn_cast<ConstantInt>(Op1)) {
              extractVarMetaData(Op0, F, vars, var_type, varToBB);
            }
          }
        }
        if (PHINode *Phi = dyn_cast<PHINode>(&I)) {
          const SCEV *PhiSCEV = SE.getSCEV(Phi);
          if (SE.isSCEVable(Phi->getType()) && SE.isLoopInvariant(PhiSCEV, L)) {
            extractVarMetaData(dyn_cast<Value>(Phi), F, vars, var_type,
                               varToBB);
          }
        }
      } // BB
    }
  } // Loop

  // 输出 varToBB 的内容
  // outs() << "vars size 234:" << vars.size() << "\n";
  // outs() << "varToBB size 234:" << varToBB.size() << "\n";
  // for (const auto &pair : varToBB) {
  //   Value *var = pair.first;
  //   const auto &bbs = pair.second;
  //   DIVariable *node = dyn_cast<DIVariable>(vars[var].node);
  //   outs() << "Variable: " << node->getName().str().c_str()
  //          << " is found in blocks: ";
  //   for (const auto *BB : bbs) {
  //     outs() << getSimplebbLabel(BB).c_str() << " ";
  //   }
  //   outs() << "\n";
  // }
}

void KnobDependencyPass::collectOnMultiple(
    Function *F, std::unordered_map<Value *, MyMDNode> &vars,
    std::unordered_map<Value *, std::unordered_set<BasicBlock *>> &varToBB) {
  for (auto &BB : *F) {
    if (this->recorded_bbs.find(&BB) == this->recorded_bbs.end()) {
      SaveBlockInfo(&BB);
      this->recorded_bbs.insert(&BB);
    }
    for (auto &Inst : BB) {
      if (BranchInst *BI = dyn_cast<BranchInst>(&Inst)) {
        if (BI->isConditional() && brNeedCheck(BI)) {
          Value *cond = BI->getCondition();
          std::string var_type = "cond";
          if (cond && isa<CmpInst>(cond)) {
            extractVarMetaData(cond, F, vars, var_type, varToBB);
          } else {
            extractVarMetaData(dyn_cast<Value>(BI), F, vars, var_type, varToBB);
          }
        }
      }
      if (auto *call = dyn_cast<CallBase>(&Inst)) {
        for (auto ArgIt = call->arg_begin(); ArgIt != call->arg_end();
             ++ArgIt) {
          Value *Arg = *ArgIt;
          std::string var_type = "arg";
          extractVarMetaData(Arg, F, vars, var_type, varToBB);
        }
      }
    } // for Inst
  }   // for BB
  if (this->recorded_fs.find(F) == this->recorded_fs.end()) {
    SaveFunctionInfo(F);
    this->recorded_fs.insert(F);
  }
}
void KnobDependencyPass::extractVarMetaData(
    Value *V, Function *F, std::unordered_map<Value *, MyMDNode> &vars,
    const std::string &var_type,
    std::unordered_map<Value *, std::unordered_set<BasicBlock *>> &varToBB) {
  if (!V)
    return;
  bool terminate = false;
  std::unordered_set<Value *> operands;
  getOperand(V, operands, 0);
  for (auto &op : operands) {
    if (GlobalValue *GV = dyn_cast<GlobalValue>(&*op)) {
      gloab_vars.insert(GV);
    }
    terminate = false;
    if (op) {
      for (auto &BB : *F) {
        for (auto &Inst : BB) {
          if (const DbgDeclareInst *DbgDeclare =
                  dyn_cast<DbgDeclareInst>(&Inst)) {
            if (DbgDeclare->getAddress() == op) {
              if (DbgDeclare->getVariable()->getName() == "this")
                continue;
              if (vars.find(op) != vars.end()) {
                if (vars[op].type.find(var_type) == std::string::npos) {
                  vars[op].type += "|" + var_type;
                }
              } else {
                vars[op] = MyMDNode(DbgDeclare->getVariable(), var_type);
              }
              varToBB[op].insert(&BB); // 记录基本块信息
              terminate = true;
              break;
            }
          } else if (const DbgValueInst *DbgValue =
                         dyn_cast<DbgValueInst>(&Inst)) {
            if (DbgValue->getValue() == op) {
              if (DbgValue->getVariable()->getName() == "this")
                continue;
              if (vars.find(op) != vars.end()) {
                if (vars[op].type.find(var_type) == std::string::npos) {
                  vars[op].type += "|" + var_type;
                }
              } else {
                vars[op] = MyMDNode(DbgValue->getVariable(), var_type);
              }
              varToBB[op].insert(&BB); // 记录基本块信息
              terminate = true;
              break;
            }
          }
        }
        if (terminate)
          break;
      }
    }
  }
}

// ######################################################################

/**
 * Collects variables within loops for a given set of functions.
 *
 * @param Fs The set of functions to collect variables from.
 * @param vars The map to store the collected variables.
 */
void KnobDependencyPass::collectOnLoop(
    std::unordered_set<Function *> Fs,
    std::unordered_map<Value *, MyMDNode> &vars) {
  for (auto F : Fs) {
    LoopInfo &LI = getAnalysis<LoopInfoWrapperPass>(*F).getLoopInfo();
    if (!LI.empty()) {
      this->collectOnLoop(F, vars);
    }
  }
}

/**
 * Collects variable metadata for a given function on loop level.
 *
 * @param F The function for which to collect variable metadata.
 * @param vars The map to store the collected variable metadata.
 */
void KnobDependencyPass::collectOnLoop(
    Function *F, std::unordered_map<Value *, MyMDNode> &vars) {
  LoopInfo &LI = getAnalysis<LoopInfoWrapperPass>(*F).getLoopInfo();
  ScalarEvolution &SE = getAnalysis<ScalarEvolutionWrapperPass>(*F).getSE();
  std::string var_type = "loop";
  for (Loop *L : LI) {
    PHINode *Phi_outer = L->getCanonicalInductionVariable();
    if (Phi_outer) {
      extractVarMetaData(dyn_cast<Value>(Phi_outer), F, vars, var_type);
    }

    for (BasicBlock *BB : L->blocks()) {
      for (Instruction &I : *BB) {
        for (User::op_iterator O = I.op_begin(); O != I.op_end(); ++O) {
          Value *o = dyn_cast<Value>(&*O);
          if (!o || !SE.isSCEVable(o->getType()))
            continue;
          const SCEV *scval = SE.getSCEV(o);
          if (scval && isa<SCEVAddRecExpr>(scval)) {
            extractVarMetaData(o, F, vars, var_type);
          }
        }
        if (BinaryOperator *BO = dyn_cast<BinaryOperator>(&I)) {
          if (BO->getOpcode() == Instruction::Add ||
              BO->getOpcode() == Instruction::Sub ||
              BO->getOpcode() == Instruction::Mul ||
              BO->getOpcode() == Instruction::UDiv ||
              BO->getOpcode() == Instruction::SDiv) {
            Value *Op0 = BO->getOperand(0);
            Value *Op1 = BO->getOperand(1);
            if (ConstantInt *C = dyn_cast<ConstantInt>(Op1)) {
              extractVarMetaData(Op0, F, vars, var_type);
            }
          }
        }
        if (PHINode *Phi = dyn_cast<PHINode>(&I)) {
          // Check if.the PHI node is an induction variable
          const SCEV *PhiSCEV = SE.getSCEV(Phi);
          if (SE.isSCEVable(Phi->getType()) && SE.isLoopInvariant(PhiSCEV, L)) {
            // outs()<< "Induction variable found: " << *Phi << "\n"";
            extractVarMetaData(dyn_cast<Value>(Phi), F, vars, var_type);
          }
        }
      } // BB
    }
  } // Loop
}

/**
 * Collects variables based on a given condition for a set of functions.
 *
 * @param Fs The set of functions to collect variables from.
 * @param vars The map to store the collected variables.
 */
void KnobDependencyPass::collectOnCond(
    std::unordered_set<Function *> Fs,
    std::unordered_map<Value *, MyMDNode> &vars) {
  for (auto F : Fs) {
    this->collectOnCond(F, vars);
  }
}

/**
 * Collects information about conditional branches in the given function.
 * This function records the basic blocks and functions that have been
 * processed. For each conditional branch instruction, it extracts variable
 * metadata and stores it in the provided map.
 *
 * @param F The function to analyze.
 * @param vars The map to store the variable metadata.
 */
void KnobDependencyPass::collectOnCond(
    Function *F, std::unordered_map<Value *, MyMDNode> &vars) {
  for (auto &BB : *F) {
    if (this->recorded_bbs.find(&BB) == this->recorded_bbs.end()) {
      SaveBlockInfo(&BB);
      this->recorded_bbs.insert(&BB);
    }
    for (auto &Inst : BB) {
      // Branch instruction
      if (BranchInst *BI = dyn_cast<BranchInst>(&Inst)) {
        if (BI->isConditional() && brNeedCheck(BI)) {
          Value *cond = BI->getCondition();
          std::string var_type = "cond";
          if (cond && isa<CmpInst>(cond)) {
            extractVarMetaData(cond, F, vars, var_type);
          } else {
            extractVarMetaData(dyn_cast<Value>(BI), F, vars, var_type);
          }
        }
      }
    }
  }
  if (this->recorded_fs.find(F) == this->recorded_fs.end()) {
    SaveFunctionInfo(F);
    this->recorded_fs.insert(F);
  }
}

/**
 * Collects variables on the arguments of the given functions.
 *
 * @param Fs The set of functions to collect variables from.
 * @param vars The map to store the collected variables.
 */
void KnobDependencyPass::collectOnArg(
    std::unordered_set<Function *> Fs,
    std::unordered_map<Value *, MyMDNode> &vars) {
  for (auto F : Fs) {
    this->collectOnArg(F, vars);
  }
}

/**
 * Collects the variables passed as arguments to function calls within a given
 * function.
 *
 * @param F The function to analyze.
 * @param vars The map to store the collected variables and their metadata.
 */
void KnobDependencyPass::collectOnArg(
    Function *F, std::unordered_map<Value *, MyMDNode> &vars) {
  for (auto &BB : *F) {
    for (auto &Inst : BB) {
      if (auto *call = dyn_cast<CallBase>(&Inst)) {
        for (auto ArgIt = call->arg_begin(); ArgIt != call->arg_end();
             ++ArgIt) {
          Value *Arg = *ArgIt;
          std::string var_type = "arg";
          extractVarMetaData(Arg, F, vars, var_type);
        }
      }
    }
  }
}

/**
 * Collects knob dependencies on multiple functions.
 *
 * This function collects knob dependencies on multiple functions by iterating
 * over a set of functions and collecting knob dependencies on each function.
 * It checks if the function has any loops using LoopInfo and calls the
 * collectOnLoop function to collect knob dependencies on loops. Finally, it
 * recursively calls itself to collect knob dependencies on multiple
 * functions.
 *
 * @param Fs The set of functions to collect knob dependencies on.
 * @param vars The map of variables and their metadata nodes to collect knob
 * dependencies on.
 */
void KnobDependencyPass::collectOnMultiple(
    std::unordered_set<Function *> Fs,
    std::unordered_map<Value *, MyMDNode> &vars) {
  for (auto F : Fs) {
    LoopInfo &LI = getAnalysis<LoopInfoWrapperPass>(*F).getLoopInfo();
    if (!LI.empty()) {
      this->collectOnLoop(F, vars);
    }
    this->collectOnMultiple(F, vars);
  }
}

/**
 * Collects variable dependencies within a function.
 *
 * This function iterates over the basic blocks of the given function and
 * collects variable dependencies. It records the basic blocks and function
 * information if they have not been recorded before. It extracts variable
 * metadata for conditional branches and function call arguments.
 *
 * @param F The function to collect variable dependencies from.
 * @param vars A map to store the collected variable metadata.
 */
void KnobDependencyPass::collectOnMultiple(
    Function *F, std::unordered_map<Value *, MyMDNode> &vars) {
  for (auto &BB : *F) {
    if (this->recorded_bbs.find(&BB) == this->recorded_bbs.end()) {
      SaveBlockInfo(&BB);
      this->recorded_bbs.insert(&BB);
    }
    for (auto &Inst : BB) {
      if (BranchInst *BI = dyn_cast<BranchInst>(&Inst)) {
        if (BI->isConditional() && brNeedCheck(BI)) {
          Value *cond = BI->getCondition();
          std::string var_type = "cond";
          if (cond && isa<CmpInst>(cond)) {
            extractVarMetaData(cond, F, vars, var_type);
          } else {
            extractVarMetaData(dyn_cast<Value>(BI), F, vars, var_type);
          }
        }
      }
      if (auto *call = dyn_cast<CallBase>(&Inst)) {
        for (auto ArgIt = call->arg_begin(); ArgIt != call->arg_end();
             ++ArgIt) {
          Value *Arg = *ArgIt;
          std::string var_type = "arg";
          extractVarMetaData(Arg, F, vars, var_type);
        }
      }
    } // for Inst
  }   // for BB
  if (this->recorded_fs.find(F) == this->recorded_fs.end()) {
    SaveFunctionInfo(F);
    this->recorded_fs.insert(F);
  }
}

/**
 * Checks if there are any matches for external global variables or class
 * member variables in the given function.
 *
 * @param F The function to be checked.
 * @return True if a match is found, False otherwise.
 */
bool KnobDependencyPass::checkOnMVAndEGV(Function *F) {
  for (auto &BB : *F) {
    for (auto &I : BB) {
      if (auto *loadInst = llvm::dyn_cast<llvm::LoadInst>(&I)) {
        llvm::Value *op = loadInst->getPointerOperand();
        if (auto *globalVar = llvm::dyn_cast<llvm::GlobalVariable>(op)) {
          if (globalVar->hasExternalLinkage()) {
            std::string global_var_name = globalVar->getName().str();
            if (taint_match(global_var_name)) {
              outs() << "external global variable matched: " << global_var_name
                     << '\n';
              return true;
            }
          }
        }
      }
      if (auto *GEP = dyn_cast<GetElementPtrInst>(&I)) {
        if (GEP->getNumOperands() < 3)
          continue;
        bool valid = false;
        llvm::Type *BaseType =
            GEP->getPointerOperandType()->getPointerElementType();
        if (!BaseType->isStructTy())
          continue;
        if (llvm::MDNode *Metadata =
                GEP->getMetadata(llvm::LLVMContext::MD_dbg)) {
          if (auto *DILoc = llvm::dyn_cast<llvm::DILocation>(Metadata)) {
            if (DILoc) {
              DIScope *Scope = DILoc->getScope();
              if (isClassScope(Scope)) {
                valid = true;
              }
            }
          }
        }
        if (!valid)
          continue;

        llvm::ConstantInt *fieldIndexOperand =
            llvm::dyn_cast<llvm::ConstantInt>(GEP->getOperand(2));
        auto index = fieldIndexOperand->getZExtValue();
        if (llvm::MDNode *metadata =
                GEP->getMetadata(llvm::LLVMContext::MD_dbg)) {
          if (auto *dbgLoc = llvm::dyn_cast<llvm::DILocation>(metadata)) {
            llvm::DIScope *scope = dbgLoc->getScope();
            // Walk up to find the DICompositeType corresponding to the
            // structure
            while (scope && !llvm::isa<llvm::DICompositeType>(scope)) {
              scope = scope->getScope();
            }
            if (auto *structType =
                    llvm::dyn_cast<llvm::DICompositeType>(scope)) {
              // Get the field from the structure by index
              llvm::DINodeArray elements = structType->getElements();
              if (index >= elements.size())
                continue;
              llvm::DINode *NodeAtIndex = elements[index];
              if (auto *field =
                      llvm::dyn_cast<llvm::DIDerivedType>(NodeAtIndex)) {
                std::string var_name = field->getName().str();
                if (taint_match(var_name)) {
                  outs() << "class member variable matched: " << var_name
                         << '\n';
                  return true;
                }
              }
            }
          }
        }
      }
    } // for Inst
  }   // for BB
  return false;
}

/**
 * Recursively collects the operands of a given value.
 *
 * @param V The value to collect operands from.
 * @param operands The set to store the collected operands.
 * @param depth The current depth of recursion.
 */
void KnobDependencyPass::getOperand(Value *V,
                                    std::unordered_set<Value *> &operands,
                                    int depth) {
  if (!V || depth >= this->max_depth)
    return;
  depth++;
  if (Instruction *I = dyn_cast<Instruction>(V)) {
    for (int i = 0; i < I->getNumOperands(); ++i) {
      getOperand(I->getOperand(i), operands, depth);
    }
  }
  if (GEPOperator *I = dyn_cast<GEPOperator>(V)) {
    for (auto Iter = I->idx_begin(); Iter != I->idx_end(); ++Iter) {
      if (!dyn_cast<ConstantInt>(&*Iter)) {
        if (Value *V = dyn_cast<Value>(&*Iter)) {
          operands.insert(V);
        }
      }
    }
  }
  operands.insert(V);
}

/**
 * Extracts variable metadata for a given value in a function.
 *
 * This function extracts variable metadata for a given value in a function.
 * It collects the operands of the value and iterates over the basic blocks
 * of the function to find debug declare and debug value instructions that
 * match the operands. If a match is found, it checks if the variable name is
 * "this" and skips it. If the variable is already present in the vars map,
 * it appends the var_type to the existing type. Otherwise, it adds a new
 * entry to the vars map with the variable and its metadata.
 *
 * @param V The value to extract variable metadata for.
 * @param F The function to extract variable metadata from.
 * @param vars The map to store the extracted variable metadata.
 * @param var_type The type of the variable.
 */
void KnobDependencyPass::extractVarMetaData(
    Value *V, Function *F, std::unordered_map<Value *, MyMDNode> &vars,
    const std::string &var_type) {
  if (!V)
    return;
  bool terminate = false;
  std::unordered_set<Value *> operands;
  getOperand(V, operands, 0);
  for (auto &op : operands) {
    if (GlobalValue *GV = dyn_cast<GlobalValue>(&*op)) {
      gloab_vars.insert(GV);
    }
    terminate = false;
    if (op) {
      for (auto &BB : *F) {
        for (auto &Inst : BB) {
          if (const DbgDeclareInst *DbgDeclare =
                  dyn_cast<DbgDeclareInst>(&Inst)) {
            if (DbgDeclare->getAddress() == op) {
              if (DbgDeclare->getVariable()->getName() == "this")
                continue;
              if (vars.find(op) != vars.end()) {
                if (vars[op].type.find(var_type) == std::string::npos) {
                  vars[op].type += "|" + var_type;
                }
              } else {
                vars[op] = MyMDNode(DbgDeclare->getVariable(), var_type);
              }
              terminate = true;
              break;
            }
          } else if (const DbgValueInst *DbgValue =
                         dyn_cast<DbgValueInst>(&Inst)) {
            if (DbgValue->getValue() == op) {
              if (DbgValue->getVariable()->getName() == "this")
                continue;
              if (vars.find(op) != vars.end()) {
                if (vars[op].type.find(var_type) == std::string::npos) {
                  vars[op].type += "|" + var_type;
                }
              } else {
                vars[op] = MyMDNode(DbgValue->getVariable(), var_type);
              }
              terminate = true;
              break;
            }
          }
        }
        if (terminate)
          break;
      }
    }
  }
}

/**
 * Checks if a variable matches any of the taint seeds.
 *
 * @param var_name The name of the variable to check.
 * @return True if the variable matches any of the taint seeds, false
 * otherwise.
 */
bool KnobDependencyPass::taint_match(const std::string &var_name) {
  if (this->taint_seeds.find(var_name) != this->taint_seeds.end()) {
    return true;
  }
  for (auto &regex : this->taint_seeds_regex) {
    if (std::regex_search(var_name, regex)) {
      return true;
    }
  }
  return false;
}

/**
 * @brief Checks if a branch instruction needs to be checked for dependencies.
 *
 * This function determines whether a branch instruction needs to be checked
 * for dependencies. It checks if the branch instruction has more than one
 * successor and if any of the successors contain a branch, call, or invoke
 * instruction.
 *
 * @param BI The branch instruction to be checked.
 * @return True if the branch instruction needs to be checked for
 * dependencies, false otherwise.
 */
bool KnobDependencyPass::brNeedCheck(const BranchInst *BI) {
  unsigned int n = BI->getNumSuccessors();
  if (n < 2)
    return false;
  unsigned int i;
  for (i = 0; i < n; i++) {
    BasicBlock *Block = BI->getSuccessor(i);
    if (Block == NULL)
      continue;
    for (BasicBlock::iterator Iter = Block->begin(); Iter != Block->end();
         ++Iter) {
      const Instruction *I = &*Iter;
      if (isa<BranchInst>(I) || isa<CallInst>(I) || isa<InvokeInst>(I)) {
        return true;
      }
    }
  }
  return false;
}

/**
 * Debug print function for KnobDependencyPass.
 * Prints information about variables and global variables.
 *
 * @param vars The unordered map of variables.
 */
void KnobDependencyPass::debug_print(
    const std::unordered_map<Value *, MyMDNode> &vars) {
  if (vars.size() == 0) {
    return;
  }
  int count = 0;
  for (auto &kv : vars) {
    MDNode *var = kv.second.node;
    if (!var) {
      outs() << "empty variable" << '\n';
      return;
    }
    DIVariable *node = dyn_cast<DIVariable>(var);
    if (!node) {
      outs() << "non DIVariable" << '\n';
      return;
    }
    outs() << "IR Inst:" << *kv.first << "|";
    outs() << node->getDirectory() << "|";
    outs() << node->getFilename() << "|";
    outs() << node->getLine() << "|";
    outs() << node->getName() << "|";
    std::string type("uintptr");
    if (node->getType()) {
      std::string type1 = node->getType()->getName().str();
      std::replace(type1.begin(), type1.end(), ' ', '|');
      if (type1.size() > 0 && type1 != " ")
        type = type1;
    }
    outs() << type << "|";
    outs() << kv.second.type << '\n';
  }
  for (auto &gv : this->gloab_vars) {
    outs() << "found Global Variable: " << gv->getName() << '\n';
  }
}

void KnobDependencyPass::taint_filter(
    std::unordered_map<Value *, MyMDNode> &vars) {}

char KnobDependencyPass::ID = 0;
static RegisterPass<KnobDependencyPass> X("knobDependencyPass",
                                          "Knob Dependency Pass", false, false);
static void registerknobDependencyPass(const PassManagerBuilder &,
                                       legacy::PassManagerBase &PM) {
  PM.add(new CallGraphWrapperPass);
  PM.add(new LoopInfoWrapperPass);
  PM.add(new ScalarEvolutionWrapperPass);
  PM.add(new KnobDependencyPass());
}

static RegisterStandardPasses Y(PassManagerBuilder::EP_ModuleOptimizerEarly,
                                registerknobDependencyPass);
static RegisterStandardPasses Z(PassManagerBuilder::EP_EnabledOnOptLevel0,
                                registerknobDependencyPass);

} // namespace knobprofiler