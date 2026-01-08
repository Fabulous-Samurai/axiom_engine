#include "unit_manager.h"
#include <unordered_map>
#include <cmath>

UnitManager::UnitManager() {
    // Length units
    RegisterUnit("m", UnitType::Length, 1.0, "meter");
    RegisterUnit("km", UnitType::Length, 1000.0, "kilometer");  
    RegisterUnit("cm", UnitType::Length, 0.01, "centimeter");
    RegisterUnit("mm", UnitType::Length, 0.001, "millimeter");
    RegisterUnit("ft", UnitType::Length, 0.3048, "foot");
    RegisterUnit("in", UnitType::Length, 0.0254, "inch");
    RegisterUnit("yd", UnitType::Length, 0.9144, "yard");
    RegisterUnit("mi", UnitType::Length, 1609.344, "mile");
    
    // Mass units
    RegisterUnit("kg", UnitType::Mass, 1.0, "kilogram");
    RegisterUnit("g", UnitType::Mass, 0.001, "gram");
    RegisterUnit("lb", UnitType::Mass, 0.453592, "pound");
    RegisterUnit("oz", UnitType::Mass, 0.0283495, "ounce");
    RegisterUnit("t", UnitType::Mass, 1000.0, "metric ton");
    
    // Time units
    RegisterUnit("s", UnitType::Time, 1.0, "second");
    RegisterUnit("min", UnitType::Time, 60.0, "minute");
    RegisterUnit("h", UnitType::Time, 3600.0, "hour");
    RegisterUnit("day", UnitType::Time, 86400.0, "day");
    RegisterUnit("week", UnitType::Time, 604800.0, "week");
    RegisterUnit("year", UnitType::Time, 31557600.0, "year");
    
    // Temperature units (special handling needed)
    RegisterUnit("K", UnitType::Temperature, 1.0, "kelvin");
    RegisterUnit("C", UnitType::Temperature, 1.0, "celsius");
    RegisterUnit("F", UnitType::Temperature, 1.0, "fahrenheit");
    
    // Angle units
    RegisterUnit("rad", UnitType::Angle, 1.0, "radian");
    RegisterUnit("deg", UnitType::Angle, PI_CONST/180.0, "degree");
    RegisterUnit("grad", UnitType::Angle, PI_CONST/200.0, "gradian");
}

void UnitManager::RegisterUnit(const std::string& symbol, UnitType type, double scale, const std::string& name) {
    units_[symbol] = {type, scale, symbol, name};
}

EngineResult UnitManager::ConvertUnit(double value, const std::string& from_unit, const std::string& to_unit) {
    auto from_it = units_.find(from_unit);
    auto to_it = units_.find(to_unit);
    
    if (from_it == units_.end() || to_it == units_.end()) {
        return {{}, {CalcErr::OperationNotFound}};
    }
    
    if (from_it->second.type != to_it->second.type) {
        return {{}, {CalcErr::ArgumentMismatch}};
    }
    
    // Special handling for temperature
    if (from_it->second.type == UnitType::Temperature) {
        return ConvertTemperature(value, from_unit, to_unit);
    }
    
    // Standard linear conversion: convert to base unit, then to target unit
    double base_value = value * from_it->second.scale_factor;
    double result = base_value / to_it->second.scale_factor;
    
    return EngineSuccessResult(result);
}

EngineResult UnitManager::ConvertTemperature(double value, const std::string& from_unit, const std::string& to_unit) {
    // Convert to Kelvin first
    double kelvin;
    if (from_unit == "C") {
        kelvin = value + 273.15;
    } else if (from_unit == "F") {
        kelvin = (value - 32.0) * 5.0/9.0 + 273.15;
    } else if (from_unit == "K") {
        kelvin = value;
    } else {
        return {{}, {CalcErr::OperationNotFound}};
    }
    
    // Convert from Kelvin to target
    double result;
    if (to_unit == "K") {
        result = kelvin;
    } else if (to_unit == "C") {
        result = kelvin - 273.15;
    } else if (to_unit == "F") {
        result = (kelvin - 273.15) * 9.0/5.0 + 32.0;
    } else {
        return {{}, {CalcErr::OperationNotFound}};
    }
    
    return EngineSuccessResult(result);
}

bool UnitManager::AreCompatible(const std::string& unit1, const std::string& unit2) {
    auto u1 = units_.find(unit1);
    auto u2 = units_.find(unit2);
    return (u1 != units_.end() && u2 != units_.end() && u1->second.type == u2->second.type);
}

std::string UnitManager::GetCanonicalUnit(UnitType type) {
    switch (type) {
        case UnitType::Length: return "m";
        case UnitType::Mass: return "kg";
        case UnitType::Time: return "s";
        case UnitType::Temperature: return "K";
        case UnitType::Angle: return "rad";
        default: return "";
    }
}

std::vector<std::string> UnitManager::GetUnitsOfType(UnitType type) {
    std::vector<std::string> result;
    for (const auto& [symbol, unit] : units_) {
        if (unit.type == type) {
            result.push_back(symbol);
        }
    }
    return result;
}