#pragma once
#include "iParser.h"
#include "dynamic_calc_types.h" // Contains 'enum class Precedence'
#include <map>
#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <functional>
#include <shared_mutex>
#include <cstring>
#include <cmath>
#include <charconv>
#include <optional>
#include <unordered_map>

// ========================================================
// 1. MEMORY ARENA (High Performance Allocation)
// ========================================================
class Arena {
    struct Block { std::unique_ptr<char[]> memory; size_t size; size_t used; };
    std::vector<Block> blocks;
public:
    explicit Arena(size_t blockSize = 1024 * 64) { allocateBlock(blockSize); }
    ~Arena() = default;
    Arena(const Arena&) = delete;
    Arena& operator=(const Arena&) = delete;
    
    void allocateBlock(size_t size) {
        auto mem = std::make_unique<char[]>(size);
        blocks.emplace_back(std::move(mem), size, 0);
    }
    void reset() { 
        // AXIOM v3.1: Rewind Strategy - prevent heap fragmentation in Daemon Mode
        if (!blocks.empty() && blocks[0].size >= 1024 * 64) {
            // Rewind: Reset used offset instead of deallocating
            for (auto& block : blocks) block.used = 0;
        } else {
            // First allocation or insufficient capacity: reallocate
            blocks.clear();
            allocateBlock(1024 * 64);
        }
    }
    
    template <typename T, typename... Args>
    T* alloc(Args&&... args) {
        size_t sizeNeeded = sizeof(T); size_t align = alignof(T);
        Block* current = &blocks.back();
        auto currentPtr = (uintptr_t)(current->memory.get() + current->used);
        size_t padding = (align - (currentPtr % align)) % align;
        
        if (current->used + padding + sizeNeeded > current->size) {
            allocateBlock(std::max(current->size * 2, sizeNeeded + align));
            current = &blocks.back(); currentPtr = (uintptr_t)(current->memory.get()); padding = 0;
        }
        current->used += padding; void* ptr = current->memory.get() + current->used; current->used += sizeNeeded;
        return new (ptr) T(std::forward<Args>(args)...);
    }
    
    std::string_view allocString(std::string_view sv) {
        size_t len = sv.length(); Block* current = &blocks.back();
        if (current->used + len > current->size) { allocateBlock(std::max(current->size * 2, len)); current = &blocks.back(); }
        char* ptr = current->memory.get() + current->used; std::memcpy(ptr, sv.data(), len); current->used += len;
        return std::string_view(ptr, len);
    }
};

namespace AXIOM {

enum class NodeType {
    Number,
    Variable,
    BinaryOp,
    UnaryOp,
    MultiArgFunction
};

// ========================================================
// 2. TRANSPARENT HASH & MAP ALIASES (Sonar S6045)
// ========================================================
struct StringHash {
    using is_transparent = void;
    size_t operator()(std::string_view sv) const {
        return std::hash<std::string_view>{}(sv);
    }
};

template<typename V>
using StringUnorderedMap = std::unordered_map<std::string, V, StringHash, std::equal_to<>>;

template<typename V>
using StringMap = std::map<std::string, V, std::less<>>;

// ========================================================
// 3. AST NODE DEFINITIONS
// ========================================================

struct ExprNode;
using NodePtr = ExprNode*;

struct EvalResult {
    // AXIOM v3.1: Enhanced to support complex numbers
    std::optional<AXIOM::Number> value;
    CalcErr error = CalcErr::None;

    static EvalResult Success(double val) {
        EvalResult result;
        result.value = AXIOM::Number(val);
        result.error = CalcErr::None;
        return result;
    }
    
    static EvalResult Success(const std::complex<double>& val) {
        EvalResult result;
        result.value = AXIOM::Number(val);
        result.error = CalcErr::None;
        return result;
    }
    
    static EvalResult Success(const AXIOM::Number& val) {
        EvalResult result;
        result.value = val;
        result.error = CalcErr::None;
        return result;
    }
    
    static EvalResult Failure(CalcErr err) {
        EvalResult result;
        result.error = err;
        return result;
    }
    
    bool HasValue() const { return value.has_value() && error == CalcErr::None; }
    
    // Legacy compatibility for existing code
    std::optional<double> GetDouble() const {
        return value.has_value() ? std::optional<double>(AXIOM::GetReal(value.value())) : std::nullopt;
    }
};

struct ExprNode {
    virtual ~ExprNode() = default;
    virtual NodeType GetType() const = 0;
    // AXIOM v3.1: Enhanced context to support complex variables
    virtual EvalResult Evaluate(const StringUnorderedMap<AXIOM::Number>& vars) const = 0;
    virtual NodePtr Derivative(Arena& arena, std::string_view var) const = 0;
    virtual NodePtr Simplify(Arena& arena) const = 0;

    // [UPDATED] Smart Pretty Printer
    // Uses 'Precedence' enum to decide if parentheses are needed.
    // Default is None (lowest precedence), meaning no parentheses.
    virtual std::string ToString(Precedence parent_prec = Precedence::None) const = 0;
};

// ========================================================
// 3. PARSER CLASS DEFINITION
// ========================================================

class AlgebraicParser : public IParser {
public:
    AlgebraicParser();
    
    // Standard execution
    EngineResult ParseAndExecute(const std::string& input) override;
    
    // [NEW] Execution with Context (Critical for 'Ans' variable and complex numbers)
    EngineResult ParseAndExecuteWithContext(const std::string& input, const StringUnorderedMap<AXIOM::Number>& context);

    // Compatibility overload for ordered-map callers.
    EngineResult ParseAndExecuteWithContext(const std::string& input, const StringMap<AXIOM::Number>& context) {
        StringUnorderedMap<AXIOM::Number> fast_context;
        fast_context.reserve(context.size());
        for (const auto& [key, value] : context) {
            fast_context.emplace(key, value);
        }
        return ParseAndExecuteWithContext(input, fast_context);
    }
    
    // Legacy compatibility method
    EngineResult ParseAndExecuteWithContext(const std::string& input, const StringUnorderedMap<double>& context) {
        // Convert double context to Number context
        StringUnorderedMap<AXIOM::Number> number_context;
        number_context.reserve(context.size());
        for (const auto& [key, value] : context) {
            number_context[key] = AXIOM::Number(value);
        }
        return ParseAndExecuteWithContext(input, number_context);
    }

    // Compatibility overload for ordered-map callers.
    EngineResult ParseAndExecuteWithContext(const std::string& input, const StringMap<double>& context) {
        StringUnorderedMap<double> fast_context;
        fast_context.reserve(context.size());
        for (const auto& [key, value] : context) {
            fast_context.emplace(key, value);
        }
        return ParseAndExecuteWithContext(input, fast_context);
    }

private:
    Arena arena_;
    mutable std::shared_mutex mutex_s;

    // Performance: Expression memoization cache
    mutable StringUnorderedMap<EvalResult> eval_cache_;
    mutable StringUnorderedMap<NodePtr> parse_cache_;
    static constexpr size_t MAX_CACHE_SIZE = 1000;

    struct CommandEntry { std::string command; std::function<EngineResult(const std::string&)> handler; };
    std::vector<CommandEntry> special_commands_;

    void RegisterSpecialCommands();
    NodePtr ParseExpression(std::string_view input);
    
    EngineResult HandleQuadratic(const std::string& input);
    EngineResult HandleNonLinearSolve(const std::string& input);
    EngineResult HandleDerivative(const std::string& input);
    EngineResult HandlePlotFunction(const std::string& input);
    
    EngineResult SolveQuadratic(double a, double b, double c);
    EngineResult SolveNonLinearSystem(const std::vector<std::string>& equations, StringUnorderedMap<double>& guess);
};

} // namespace AXIOM