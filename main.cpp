#include "CompilerEngine.h"
#include "CodeAnalyzer.h"
#include "Runner.h"
#include "LLMClient.h"
#include "SecurityScanner.h"
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/SourceMgr.h>
#include <iostream>
#include <filesystem>
#include <limits>

namespace fs = std::filesystem;

// --- CONFIGURATION ---
const std::string MODEL = "gemini-2.5-flash"; 
const int MAX_ITERATIONS = 3;

// Compile C -> IR Helper
bool compileCToIR(const std::string& inputFile, const std::string& outputFile) {
    std::string command = "clang-18 -S -emit-llvm " + inputFile + " -o " + outputFile;
    return (std::system(command.c_str()) == 0);
}

// AI Response Parser
std::vector<std::string> parse_ai_strategy(std::string ai_response) {
    std::vector<std::string> raw_passes;
    try {
        size_t json_start = ai_response.find("[");
        size_t json_end = ai_response.rfind("]");
        if (json_start != std::string::npos && json_end != std::string::npos) {
            ai_response = ai_response.substr(json_start, json_end - json_start + 1);
        }
        auto j = json::parse(ai_response);
        for (const auto& item : j) raw_passes.push_back(item);
    } catch (...) {
        return {"mem2reg", "instcombine", "simplifycfg"}; // Fallback
    }

    std::vector<std::string> safe_pipeline;
    for (std::string p : raw_passes) {
        if (p == "mem2reg" || p == "instcombine" || p == "simplifycfg" || 
            p == "dce" || p == "gvn" || p == "sroa" || p == "early-cse" || p == "adce") {
            safe_pipeline.push_back(p);
        } 
        else if (p == "licm") {
            safe_pipeline.push_back("loop-mssa(licm)");
        } 
        else if (p == "loop-unroll") {
            safe_pipeline.push_back("loop(loop-unroll-full)");
        } 
        else {
            std::cout << "\033[1;33m[Safety Filter] Stripping dangerous/unknown pass: " << p << "\033[0m\n";
        }
    }

    if (safe_pipeline.empty()) return {"mem2reg", "instcombine"};
    return safe_pipeline;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: ./agentic-cc <input.c>\n";
        return 1;
    }

    // 1. API Setup
    const char* env_key = std::getenv("GEMINI_API_KEY");
    if (!env_key) {
        std::cerr << "Error: GEMINI_API_KEY not set.\n"; 
        return 1;
    }
    LLMClient ai(MODEL, env_key);

    std::string inputFile = argv[1];
    std::string baseIrFile = "base.ll";
    std::string currentIrFile = "current.ll";
    std::string binaryFile = "output_bin";

    // 2. Initial Compilation (C -> IR)
    if (!compileCToIR(inputFile, baseIrFile)) return 1;

    llvm::LLVMContext context;
    llvm::SMDiagnostic error;
    auto baseModule = llvm::parseIRFile(baseIrFile, error, context);
    if (!baseModule) return 1;

    // SECURITY SENTINEL ---
    std::cout << "\n[Sentinel] Scanning for vulnerabilities...\n";
    auto risks = SecurityScanner::scan(*baseModule);
    std::string security_context = "";
    
    if (!risks.empty()) {
        std::cout << "\033[1;31m[Sentinel] ALERT: Found " << risks.size() << " risks!\033[0m\n";
        security_context = "CRITICAL SECURITY WARNING: The code contains these risks: ";
        for (const auto& r : risks) {
            std::cout << "  - " << r.riskType << " in " << r.functionName << ": " << r.suggestion << "\n";
            security_context += r.functionName + " (" + r.riskType + "); ";
        }
        security_context += "You MUST prioritize safety passes (like stack-protection) if available.";
    } else {
        std::cout << "[Sentinel] Code looks clean.\n";
    }

    //THE SELF-TUNING LOOP ---
    long best_time = std::numeric_limits<long>::max();
    std::vector<std::string> best_strategy;
    
    for (int i = 1; i <= MAX_ITERATIONS; i++) {
        std::cout << "\n---------------- ITERATION " << i << " ----------------\n";
        
        auto currentModule = llvm::parseIRFile(baseIrFile, error, context);

        // 1. Analyze
        auto metrics = CodeAnalyzer::analyze(*currentModule);
        
        std::string system_prompt = 
            "You are an LLVM Compiler Expert. Output ONLY a JSON array of strings. "
            "CHOOSE ONLY from this exact list: [\"mem2reg\", \"instcombine\", \"simplifycfg\", \"dce\", \"gvn\", \"sroa\", \"early-cse\", \"licm\", \"loop-unroll\"]. "
            "Do not add any other passes. Do not add explanations.";
        
        std::string user_prompt = "Code Metrics: " + metrics.dump() + ". \n";
        
        if (!security_context.empty()) {
            user_prompt += "\n" + security_context;
        }

        if (i > 1) {
            user_prompt += "\nPrevious run took " + std::to_string(best_time) + "ms. "
                           "Try to generate a DIFFERENT strategy to beat this time. "
                           "Be aggressive with optimizations.";
        }

        // 3. Consult AI
        std::cout << "[Agent] asking AI...\n";
        std::string strategy_json = ai.query(system_prompt, user_prompt);
        std::vector<std::string> passes = parse_ai_strategy(strategy_json);
        
        std::cout << "[Agent] Strategy: " << strategy_json << "\n";

        // 4. Optimize
        CompilerEngine engine;
        engine.optimize_module(*currentModule, passes);

        // 5. Measure
        Runner::save_module(*currentModule, currentIrFile);
        if (Runner::compile_ir_to_bin(currentIrFile, binaryFile)) {
            long current_time = Runner::run_and_measure(binaryFile, 5);
            std::cout << "-> Execution Time: " << current_time << "ms\n";

            // 6. Compare & Save Best
            if (current_time < best_time) {
                std::cout << "\033[1;32m[New Record] This is the fastest run so far!\033[0m\n";
                best_time = current_time;
                best_strategy = passes;
                fs::copy_file(binaryFile, "final_optimized_bin", fs::copy_options::overwrite_existing);
            } else {
                std::cout << "[Result] Slower than best record (" << best_time << "ms).\n";
            }
        }
    }

    std::cout << "\n==============================================\n";
    std::cout << "Optimization Complete.\n";
    std::cout << "Best Time: " << best_time << "ms\n";
    std::cout << "Best Strategy: ";
    for(const auto& s : best_strategy) std::cout << s << " ";
    std::cout << "\nBinary saved to: ./final_optimized_bin\n";

    return 0;
}
