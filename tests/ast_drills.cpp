#include <iostream>
#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <map>
#include <cmath>
#include <charconv>
#include <optional>
#include <cstring>

class Arena
{
    struct Block
    {
        char *memory;
        size_t size;
        size_t used;
    };

    std::vector<Block> blocks;

public:
    Arena(size_t blockSize = 1024 * 64)
    {
        allocateBlock(blockSize);
    }

    ~Arena()
    {
        for (auto &block : blocks)
            delete[] block.memory;
    }
    void allocateBlock(size_t size)
    {
        char *mem = new char[size];
        blocks.push_back({mem, size, 0});
    }

    template <typename T, typename... Args>
    T *alloc(Args &&...args)
    {
        // Alingment
        size_t sizeNeeded = sizeof(T);
        size_t align = alignof(T);

        Block *current = &blocks.back();

        // basic padding logic
        uintptr_t currentPtr = (uintptr_t)(current->memory + current->used);
        size_t padding = (align - (currentPtr % align)) % align;

        if (current->used + padding + sizeNeeded > current->size)
        {
            allocateBlock(std::max(current->size * 2, sizeNeeded + align));
            current = &blocks.back();
            currentPtr = (uintptr_t)(current->memory);
            padding = 0;
        }

        current->used += padding;
        void *ptr = current->memory + current->used;
        current->used += sizeNeeded;
        return new (ptr) T(std::forward<Args>(args)...);
    }

    // special func for copying strings to the arena.
    std::string_view allocString(std::string_view sv)
    {
        size_t len = sv.length();
        Block *current = &blocks.back();

        if (current->used + len > current->size)
        {
            allocateBlock(std::max(current->size * 2, len));
            current = &blocks.back();
        }

        char *ptr = current->memory + current->used;
        std::memcpy(ptr, sv.data(), len);
        current->used += len;

        return std::string_view(ptr, len);
    }
};

enum class NodeType {
    Number,
    Variable,
    BinaryOp,
    UnaryOp,
    MultiArgFunction
};

struct ExprNode;
using NodePtr = ExprNode *;

struct ExprNode
{
    virtual ~ExprNode() = default;
    virtual NodeType GetType() const = 0;

    virtual double Evaluate(const std::map<std::string, double> &vars) const = 0;

    virtual NodePtr Derivative(Arena &arena, std::string_view var) const = 0;
};

struct NumberNode : ExprNode
{
    double value;

    NumberNode(double val) : value(val) {}
    NodeType GetType() const override { return NodeType::Number; }
    double Evaluate(const std::map<std::string, double> &) const override { return value; }

    NodePtr Derivative(Arena &arena, std::string_view var) const override
    {
        return arena.alloc<NumberNode>(0.0);
    }
};

struct VariableNode : ExprNode
{
    std::string_view name;

    VariableNode(std::string_view varName) : name(varName) {}
    NodeType GetType() const override { return NodeType::Variable; }

    double Evaluate(const std::map<std::string, double> &vars) const override
    {
        auto it = vars.find(std::string(name));
        if (it != vars.end())
            return it->second;
        throw std::runtime_error("Variable not found: " + std::string(name));
    }

    NodePtr Derivative(Arena &arena, std::string_view var) const override
    {
        return arena.alloc<NumberNode>(name == var ? 1.0 : 0.0);
    }
};

struct BinaryOpNode : ExprNode
{
    char op;
    NodePtr left;
    NodePtr right;
    BinaryOpNode(char c, NodePtr l, NodePtr r) : op(c), left(l), right(r) {}
    NodeType GetType() const override { return NodeType::BinaryOp; }

    double Evaluate(const std::map<std::string, double> &vars) const override
    {
        double l = left->Evaluate(vars);
        double r = right->Evaluate(vars);

        if (op == '+')
            return l + r;
        if (op == '-')
            return l - r;
        if (op == '*')
            return l * r;
        if (op == '/')
            return l / r;
        return 0;
    }

    NodePtr Derivative(Arena &arena, std::string_view var) const override
    {
        auto dl = left->Derivative(arena, var);
        auto dr = right->Derivative(arena, var);

        if (op == '+' || op == '-')
        {
            return arena.alloc<BinaryOpNode>(op, dl, dr);
        }
        if (op == '*')
        {
            //(f' * g) + (f * g')
            auto term1 = arena.alloc<BinaryOpNode>('*', dl, right);
            auto term2 = arena.alloc<BinaryOpNode>('*', dr, left);
            return arena.alloc<BinaryOpNode>('+', term1, term2);
        }
        return arena.alloc<NumberNode>(0.0);
    }
};

std::optional<double> ToDouble(std::string_view sv)
{
    double result;
    auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), result);
    if (ec == std::errc())
        return result;
    return std::nullopt;
}
NodePtr Parse(Arena &arena, std::string_view input)
{
    size_t pos = input.find('+');
    if (pos == std::string_view::npos)
        pos = input.find('-');
    if (pos != std::string_view::npos)
    {
        auto left = input.substr(0, pos);
        auto right = input.substr(pos + 1);
        char op = input[pos];
        return arena.alloc<BinaryOpNode>(op, Parse(arena, left), Parse(arena, right));
    }

    pos = input.find('*');
    if (pos != std::string_view::npos)
    {
        auto left = input.substr(0, pos);
        auto right = input.substr(pos + 1);
        return arena.alloc<BinaryOpNode>('*', Parse(arena, left), Parse(arena, right));
    }

    if (auto val = ToDouble(input))
    {
        return arena.alloc<NumberNode>(*val);
    }
    else
    {
        // CRITICAL : copy the variable name to the arena ,so even if there is any input string, VariableNode still lives
        std::string_view safeName = arena.allocString(input);
        return arena.alloc<VariableNode>(safeName);
    }
}

int main() {
    Arena myArena(1024*1024);
    std::string expr = "x*x+5";
    std::cout<<"Expression:"<<expr<<"\n";

    NodePtr root = Parse(myArena,expr);
    // taking derivative
    NodePtr derivative = root->Derivative(myArena,"x");

    //calculation phase 
    std::map<std::string,double>ctx;
    ctx["x"] = 3.0;

    std::cout<<"f(3) = "<<root->Evaluate(ctx)<<"\n";
    std::cout<<"f'(3) = "<<derivative->Evaluate(ctx)<<"\n";

    return 0;
}
