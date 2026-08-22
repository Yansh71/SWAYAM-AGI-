#ifndef SWAYAM_COGNITIVE_FORGE_HPP
#define SWAYAM_COGNITIVE_FORGE_HPP

// SWAYAM Phase 5+6: CognitiveForge with Predictive Execution Layer
#include "core.hpp"
#include "Moltbook.h"
#include "HeuristicAnalyzer.hpp" 
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <filesystem>
#include <cstdint>
#include <cctype>
#include <chrono>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

namespace Swayam {

struct ForgeResult {
    bool analyzed = false;
    bool pre_quarantined = false;
    bool compiled = false;
    bool executed = false;
    bool quarantined = false;
    std::vector<HeuristicAnalyzer::Finding> findings;
    std::string binary_path;
    std::string source_path;
    std::string compile_log;
};

class CognitiveForge {
public:
    static ForgeResult spawn(AtomicGuard& guard,
                             const std::string& module_name,
                             const std::string& cpp_source) {
        ForgeResult result;

        if (module_name.empty()) {
            throw std::invalid_argument("[FORGE FATAL] Module name cannot be empty.");
        }
        for (char c : module_name) {
            if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_') {
                throw std::invalid_argument("[FORGE FATAL] Invalid module name.");
            }
        }

        const std::string filename = module_name + ".h";
        const std::string bin_dir = "/tmp/swayam_forge";
        const std::string bin_path = bin_dir + "/" + module_name;
        const std::string driver_path = bin_dir + "/" + module_name + "_driver.cpp";
        const std::string log_path = bin_dir + "/" + module_name + "_compile.log";
        const std::string source_path = "src/generated/" + filename;

        std::filesystem::create_directories(bin_dir);
        result.binary_path = bin_path;
        result.source_path = source_path;

        // Step 1: HEURISTIC ANALYSIS (Phase 6)
        result.analyzed = true;
        auto analysis = HeuristicAnalyzer::analyze(cpp_source);
        result.findings = analysis.findings;
        HeuristicAnalyzer::report(analysis, module_name);

        if (analysis.pre_quarantine) {
            uint64_t hash = structural_hash(cpp_source);
            write_prequarantine(hash, module_name);
            result.pre_quarantined = true;
            std::cerr << "[FORGE] Pre-quarantined " << module_name 
                      << " (hash=" << hash << "). Compilation blocked.\n";
            return result;
        }

        if (analysis.warning_count > 0) {
            std::cout << "[FORGE] " << analysis.warning_count 
                      << " warning(s) on " << module_name 
                      << ". Proceeding to compile.\n";
        }

        // Step 2: Write source through Moltbook
        try {
            SwayamAGI::Core::Moltbook::write_mutation(filename, cpp_source);
            std::cout << "[FORGE] Source written: " << source_path << "\n";
        } catch (const std::exception& e) {
            throw std::runtime_error(std::string("[FORGE FATAL] Moltbook rejected write: ") + e.what());
        }

        // Step 3: Compile via fork()+execvp
        {
            const std::string abs_src = std::filesystem::absolute(source_path).string();
            std::ofstream driver(driver_path, std::ios::trunc);
            if (!driver) {
                throw std::runtime_error("[FORGE FATAL] Cannot write driver to " + driver_path);
            }
            driver << "#include \"" << abs_src << "\"\n"
                   << "int main() { return 0; }\n";
        }

        result.compiled = compile(driver_path, bin_path, log_path);
        {
            std::ifstream lf(log_path);
            std::ostringstream ls;
            ls << lf.rdbuf();
            result.compile_log = ls.str();
        }

        if (!result.compiled) {
            std::cerr << "[FORGE] Compile failed for " << module_name << "\n"
                      << result.compile_log << "\n";
            return result;
        }
        std::cout << "[FORGE] Compiled: " << bin_path << "\n";

        // Step 4: Execute via crash-safe supervisor
        result.executed = run_guarded_mutation(guard, bin_path, source_path);
        
        if (!result.executed) {
            result.quarantined = is_quarantined(source_path);
            std::cerr << "[FORGE] " << module_name 
                      << (result.quarantined ? " CRASHED -> quarantined by supervisor." : " exited non-zero.")
                      << "\n";
        } else {
            std::cout << "[FORGE] Neural pathway " << module_name << " executed successfully.\n";
        }

        return result;
    }

    static std::string summary(const ForgeResult& r, const std::string& module_name) {
        if (r.pre_quarantined) return "[FORGE] " + module_name + ": PRE-QUARANTINED by heuristic.";
        if (!r.analyzed) return "[FORGE] " + module_name + ": analysis skipped.";
        if (!r.compiled) return "[FORGE] " + module_name + ": COMPILE FAILED.";
        if (!r.executed) return "[FORGE] " + module_name + (r.quarantined ? " CRASHED -> quarantined." : " exited non-zero.");
        return "[FORGE] " + module_name + " SUCCESS.";
    }

private:
    static void write_prequarantine(uint64_t hash, const std::string& module_name) {
        const char* env = std::getenv("SWAYAM_QUARANTINE_LOG");
        const std::string log_path = env ? env : "/tmp/quarantine.log";
        std::ofstream log(log_path, std::ios::app);
        if (!log) {
            std::cerr << "[FORGE WARN] Cannot write pre-quarantine entry.\n";
            return;
        }
        auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        log << hash << ",254," << now << ",pre-quarantine:" << module_name << "\n";
    }

    static bool compile(const std::string& source, const std::string& output, const std::string& log_path) {
        pid_t pid = fork();
        if (pid < 0) {
            throw std::runtime_error("[FORGE] Fork failed during compilation.");
        }
        if (pid == 0) {
            int fd = open(log_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd >= 0) {
                dup2(fd, STDERR_FILENO);
                dup2(fd, STDOUT_FILENO);
                close(fd);
            }
            const char* args[] = {
                "g++", "-O1", "-std=c++23", source.c_str(), "-o", output.c_str(), nullptr
            };
            execvp("g++", const_cast<char* const*>(args));
            _exit(127);
        }
        int status = 0;
        waitpid(pid, &status, 0);
        return WIFEXITED(status) && WEXITSTATUS(status) == 0;
    }
};

} // namespace Swayam
#endif // SWAYAM_COGNITIVE_FORGE_HPP
