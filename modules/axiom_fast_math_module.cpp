export module axiom.fast_math;

export namespace AXIOM::Modules {
    inline double FastMul(double lhs, double rhs) noexcept {
        return lhs * rhs;
    }

    inline bool FastDiv(double lhs, double rhs, double& out) noexcept {
        if (rhs == 0.0) {
            return false;
        }
        out = lhs / rhs;
        return true;
    }
}
