export module axiom.dynamic_calc_types;

export namespace AXIOM::Modules {
    inline constexpr int kPilotModuleVersion = 1;

    inline double FastAdd(double lhs, double rhs) noexcept {
        return lhs + rhs;
    }
}
