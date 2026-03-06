#pragma once
#include <llvm/IR/Module.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/InstIterator.h>
#include <vector>
#include <string>
#include <iostream>

class SecurityScanner {
public:
    struct Risk {
        std::string functionName;
        std::string riskType;
        std::string suggestion;
    };

    static std::vector<Risk> scan(llvm::Module& module) {
        std::vector<Risk> risks;

        for (auto& F : module) {
            for (auto& I : llvm::instructions(F)) {
                
                if (auto* callInst = llvm::dyn_cast<llvm::CallInst>(&I)) {
                    llvm::Function* calledFunc = callInst->getCalledFunction();
                    
                    if (!calledFunc || !calledFunc->hasName()) continue;

                    std::string funcName = calledFunc->getName().str();

                    if (funcName == "strcpy") {
                        risks.push_back({funcName, "Buffer Overflow", "Use 'strncpy' or 'strlcpy' instead."});
                    }
                    else if (funcName == "gets") {
                        risks.push_back({funcName, "Unbounded Write", "NEVER use gets. Use 'fgets'."});
                    }
                    else if (funcName == "system") {
                        risks.push_back({funcName, "Command Injection", "Sanitize inputs before passing to system()."});
                    }
                    else if (funcName == "sprintf") {
                        risks.push_back({funcName, "Buffer Overflow", "Use 'snprintf' to limit buffer size."});
                    }
                }
            }
        }
        return risks;
    }
};
