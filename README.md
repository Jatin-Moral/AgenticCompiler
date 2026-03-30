# Agentic AI-Driven Compiler Pipeline

## Overview
This project implements a self-tuning C++ compiler pipeline where an AI agent (Google Gemini) autonomously manages compiler optimization phases based on the source code's structure and performance feedback. It parses the C code into LLVM Intermediate Representation (IR), extracts structural metrics and security vulnerabilities (Code DNA), and uses these to query the AI for the best optimization sequence.

## Features
- **Code DNA Extraction**: Analyzes LLVM IR to count basic blocks, instructions, memory operations, and branches. It also detects vulnerabilities like `strcpy`, `gets`, or `sprintf`.
- **AI-Driven Optimization**: Autonomously requests optimization passes from Gemini 1.5 based on the extracted Code DNA and historical execution time.
- **Feedback Loop**: Compiles the IR to an executable, runs it to benchmark the execution time, and feeds performance metrics back to the AI for iterative improvement.
- **Resilience**: Falls back to default optimization passes (`-O2`) if the AI API is unreachable or fails to return passes.

## Prerequisites
- **LLVM 18** (`llvm-18`, `clang-18`)
- **CMake** (v3.16+) and **Ninja**
- **nlohmann/json**
- **libcurl**

## Setup & Build
1. Create a `build` directory and run CMake:
   ```bash
   mkdir build && cd build
   cmake .. -GNinja
   ninja
   ```

2. Export your Google AI Studio API key:
   ```bash
   export GOOGLE_API_KEY="your-gemini-api-key"
   ```

## Usage
The main executable `agentic_compiler` orchestrates the pipeline.

```bash
./build/agentic_compiler <path/to/source_file.c> [--iterations <N>]
```
- `<path/to/source_file.c>`: The C program you want to optimize.
- `--iterations <N>`: (Optional) The number of feedback loop iterations. Defaults to 3.

### Example
A test file containing a buffer overflow vulnerability and a heavy mathematics loop is provided in `tests/vulnerable_loop.c`.

```bash
./build/agentic_compiler tests/vulnerable_loop.c --iterations 5
```

## Project Structure
- `src/main.cpp`: The command-line orchestrator.
- `src/Pipeline.cpp`: Manages the LLVM IR manipulation, executable compilation, and benchmarking loop.
- `src/DNAExtractor.cpp`: Extracts Code DNA (structure statistics and vulnerabilities) from LLVM Modules.
- `src/AIClient.cpp`: Communicates with Google AI Studio using `libcurl` and parses recommended passes.
- `tests/`: Contains test C programs to verify the compiler loop.
