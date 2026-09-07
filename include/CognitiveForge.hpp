#ifndef SWAYAM_COGNITIVE_FORGE_HPP
#define SWAYAM_COGNITIVE_FORGE_HPP
// =============================================================
// SWAYAM CognitiveForge — The Metamorphic Neural Engine
// 
// ARCHITECTURE ENFORCEMENT: True Polymorphic Metamorphism.
// KILLS COMPILER TRAPS: Explicit static_cast<int> silences 
// -Werror=conversion during 64-bit to 32-bit RNG entropy shifts.
// =============================================================
#include <string>
#include <vector>
#include <random>
#include <chrono>
#include <regex>
#include <iomanip>
#include <sstream>

namespace Swayam {

class CognitiveForge {
private:
    static std::mt19937_64& get_rng() noexcept {
        thread_local std::mt19937_64 rng(std::chrono::high_resolution_clock::now().time_since_epoch().count());
        return rng;
    }

    static std::string generate_dynamic_identifier(const std::string& prefix = "swayam_") {
        auto& rng = get_rng();
        std::stringstream ss;
        ss << prefix << std::hex << std::uppercase << (rng() % 0xFFFFFF);
        return ss.str();
    }

    static std::string generate_opaque_predicate(const std::string& var_name) {
        auto& rng = get_rng();
        
        // THE APEX FIX: Explicit cast bypasses -Werror=conversion
        int logic_path = static_cast<int>(rng() % 3);
        
        switch(logic_path) {
            case 0: 
                return "if ((" + var_name + " * " + var_name + " + " + var_name + ") % 2 == 0) { " + var_name + " ^= 0xAA; } else { " + var_name + " = 0; }\n";
            case 1: 
                return var_name + " = (" + var_name + " << 1) ^ (" + var_name + " >> 1);\n";
            case 2: 
                return "constexpr int _c_" + var_name + " = 42; if (" + var_name + " != _c_" + var_name + ") { " + var_name + "++; }\n";
            default: return "";
        }
    }

    static std::string synthesize_logic_block() {
        auto& rng = get_rng();
        std::string var1 = generate_dynamic_identifier("_evo_core_");
        std::string var2 = generate_dynamic_identifier("_evo_mem_");
        
        std::string logic_block = "\n    // [SWAYAM-SYNTH] Neural Polymorphic Shift Initiated\n";
        
        logic_block += "    [[maybe_unused]] volatile unsigned long long " + var1 + " = " + std::to_string(rng() % 9999) + "ULL;\n";
        logic_block += "    [[maybe_unused]] volatile unsigned long long " + var2 + " = " + std::to_string(rng() % 9999) + "ULL;\n";
        
        logic_block += "    " + generate_opaque_predicate(var1);
        logic_block += "    " + generate_opaque_predicate(var2);
        
        // THE APEX FIX: Explicit cast bypasses -Werror=conversion
        int iterations = static_cast<int>((rng() % 5) + 1);
        logic_block += "    for(int i = 0; i < " + std::to_string(iterations) + "; ++i) {\n";
        logic_block += "        " + var1 + " = (" + var1 + " * 0x1B3) + i;\n";
        logic_block += "        " + var2 + " ^= " + var1 + ";\n";
        logic_block += "    }\n";
        
        return logic_block;
    }

public:
    static std::string evolve_codebase(const std::string& base_algorithm) {
        auto& rng = get_rng();
        std::string mutated_code = base_algorithm;
        std::string logic_block = synthesize_logic_block();

        std::regex main_re(R"(int\s+main\s*\([^)]*\)\s*\{)");
        std::smatch match;
        
        if (std::regex_search(mutated_code, match, main_re)) {
            mutated_code.insert(match.position() + match.length(), logic_block);
        } else {
            mutated_code += "\n// [SWAYAM-ORPHAN-BLOCK]\nvoid _evo_orphan() {" + logic_block + "}\n";
        }

        std::string genome_id = "\n// [EVO-GENOME-ID: 0x" + std::to_string(rng()) + "]\n";
        return mutated_code + genome_id;
    }
};

} // namespace Swayam
#endif // SWAYAM_COGNITIVE_FORGE_HPP
