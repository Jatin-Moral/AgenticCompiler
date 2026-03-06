#pragma once
#include <llvm/IR/Module.h>
#include <llvm/IR/Instructions.h>
#include <nlohmann/json.hpp>

class CodeAnalyzer {
public:
    static nlohmann::json analyze(llvm::Module& module) {
        int instruction_count = 0;
        int branch_count = 0;
        int load_count = 0;
        int store_count = 0;
        int function_count = 0;

        for (auto& F : module) {
            if (F.isDeclaration()) continue;
            function_count++;

            for (auto& BB : F) {
                for (auto& I : BB) {
                    instruction_count++;

                    if (llvm::isa<llvm::BranchInst>(I) || llvm::isa<llvm::SwitchInst>(I)) {
                        branch_count++;
                    }
                    if (llvm::isa<llvm::LoadInst>(I)) {
                        load_count++;
                    }
                    if (llvm::isa<llvm::StoreInst>(I)) {
                        store_count++;
                    }
                }
            }
        }

        nlohmann::json metrics;
        metrics["instructions"] = instruction_count;
        metrics["branches"] = branch_count;
        metrics["memory_ops"] = load_count + store_count;
        metrics["functions"] = function_count;
        
        return metrics;
    }
};
