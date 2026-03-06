#pragma once
#include <llvm/IR/Module.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/StandardInstrumentations.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Analysis/LoopAnalysisManager.h>
#include <llvm/Analysis/CGSCCPassManager.h>
#include <vector>
#include <string>
#include <numeric>

class CompilerEngine {
public:
    llvm::LoopAnalysisManager LAM;
    llvm::FunctionAnalysisManager FAM;
    llvm::CGSCCAnalysisManager CGAM;
    llvm::ModuleAnalysisManager MAM;
    llvm::PassBuilder PB;

    CompilerEngine() {
        PB.registerModuleAnalyses(MAM);
        PB.registerCGSCCAnalyses(CGAM);
        PB.registerFunctionAnalyses(FAM);
        PB.registerLoopAnalyses(LAM);
        PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);
    }

    void optimize_module(llvm::Module& module, const std::vector<std::string>& pass_names) {
        // 2. Convert vector ["mem2reg", "dce"] -> string "mem2reg,dce"
        std::string pipeline_str;
        for (size_t i = 0; i < pass_names.size(); ++i) {
            pipeline_str += pass_names[i];
            if (i < pass_names.size() - 1) pipeline_str += ",";
        }

        if (pipeline_str.empty()) return;

        // 3. Create the pass pipeline
        llvm::ModulePassManager MPM;
        if (auto err = PB.parsePassPipeline(MPM, pipeline_str)) {
            llvm::errs() << "Error parsing pipeline: " << pipeline_str << "\n";
            consumeError(std::move(err)); 
            return;
        }

        //the optimizations!
        MPM.run(module, MAM);
    }
};
