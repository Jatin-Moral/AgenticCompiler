#pragma once
#include <cstdlib>
#include <chrono>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>

class Runner {
public:
    // Helper to write LLVM IR to disk
    static void save_module(llvm::Module& module, const std::string& filename) {
        std::error_code EC;
        
        llvm::raw_fd_ostream dest(filename, EC);

        if (EC) {
            std::cerr << "Could not open file: " << EC.message() << "\n";
            return;
        }
        module.print(dest, nullptr);
    }

    static long run_and_measure(const std::string& binary_name, int iterations = 5) {
        long total_duration = 0;

        for (int i = 0; i < iterations; ++i) {
            auto start = std::chrono::high_resolution_clock::now();
            
            std::string cmd = "./" + binary_name;
            int ret = std::system(cmd.c_str());
            
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
            
            total_duration += duration;
        }

        return total_duration / iterations;
    }
    
    static bool compile_ir_to_bin(const std::string& ir_file, const std::string& output_bin) {
        std::string cmd = "clang-18 " + ir_file + " -o " + output_bin + " -Wno-override-module"; 
        int ret = std::system(cmd.c_str());
        return (ret == 0);
    }
};
