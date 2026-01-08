#include "statistics_engine.h"
#include <cmath>
#include <unordered_map>

EngineResult StatisticsEngine::Mean(const Vector& data) {
    if (data.empty()) return {{}, {CalcErr::ArgumentMismatch}};
    
    double sum = 0.0;
    for (double val : data) {
        if (!std::isfinite(val)) return {{}, {CalcErr::DomainError}};
        sum += val;
    }
    return EngineSuccessResult(sum / data.size());
}

EngineResult StatisticsEngine::Median(Vector data) {
    if (data.empty()) return {{}, {CalcErr::ArgumentMismatch}};
    
    std::sort(data.begin(), data.end());
    size_t n = data.size();
    
    if (n % 2 == 0) {
        return EngineSuccessResult((data[n/2-1] + data[n/2]) / 2.0);
    } else {
        return EngineSuccessResult(data[n/2]);
    }
}

EngineResult StatisticsEngine::Mode(const Vector& data) {
    if (data.empty()) return {{}, {CalcErr::ArgumentMismatch}};
    
    std::unordered_map<double, int> frequency;
    for (double val : data) {
        frequency[val]++;
    }
    
    double mode_val = data[0];
    int max_count = 0;
    for (const auto& [val, count] : frequency) {
        if (count > max_count) {
            max_count = count;
            mode_val = val;
        }
    }
    
    return EngineSuccessResult(mode_val);
}

EngineResult StatisticsEngine::Variance(const Vector& data) {
    if (data.size() < 2) return {{}, {CalcErr::ArgumentMismatch}};
    
    auto mean_result = Mean(data);
    if (!mean_result.result.has_value()) return mean_result;

    // Use the EngineResult helper to extract a double regardless of underlying variant
    auto mean_val_opt = mean_result.GetDouble();
    if (!mean_val_opt.has_value()) return {{}, {CalcErr::ArgumentMismatch}};
    double mean_val = *mean_val_opt;

    double sum_sq_diff = 0.0;
    
    for (double val : data) {
        double diff = val - mean_val;
        sum_sq_diff += diff * diff;
    }
    
    return EngineSuccessResult(sum_sq_diff / (data.size() - 1));
}

EngineResult StatisticsEngine::StandardDeviation(const Vector& data) {
    auto var_result = Variance(data);
    if (!var_result.result.has_value()) return var_result;
    
    double variance = std::get<double>(*var_result.result);
    return EngineSuccessResult(std::sqrt(variance));
}

EngineResult StatisticsEngine::Correlation(const Vector& x, const Vector& y) {
    if (x.size() != y.size() || x.size() < 2) {
        return {{}, {CalcErr::ArgumentMismatch}};
    }
    
    auto x_mean_result = Mean(x);
    auto y_mean_result = Mean(y);
    if (!x_mean_result.result.has_value() || !y_mean_result.result.has_value()) {
        return {{}, {CalcErr::DomainError}};
    }
    
    double x_mean = std::get<double>(*x_mean_result.result);
    double y_mean = std::get<double>(*y_mean_result.result);
    
    double numerator = 0.0, sum_x_sq = 0.0, sum_y_sq = 0.0;
    
    for (size_t i = 0; i < x.size(); ++i) {
        double x_diff = x[i] - x_mean;
        double y_diff = y[i] - y_mean;
        numerator += x_diff * y_diff;
        sum_x_sq += x_diff * x_diff;
        sum_y_sq += y_diff * y_diff;
    }
    
    double denominator = std::sqrt(sum_x_sq * sum_y_sq);
    if (denominator == 0.0) return {{}, {CalcErr::DivideByZero}};
    
    return EngineSuccessResult(numerator / denominator);
}

EngineResult StatisticsEngine::LinearRegression(const Vector& x, const Vector& y) {
    if (x.size() != y.size() || x.size() < 2) {
        return {{}, {CalcErr::ArgumentMismatch}};
    }
    
    auto x_mean_result = Mean(x);
    auto y_mean_result = Mean(y);
    if (!x_mean_result.result.has_value() || !y_mean_result.result.has_value()) {
        return {{}, {CalcErr::DomainError}};
    }
    
    double x_mean = std::get<double>(*x_mean_result.result);
    double y_mean = std::get<double>(*y_mean_result.result);
    
    double numerator = 0.0, denominator = 0.0;
    
    for (size_t i = 0; i < x.size(); ++i) {
        double x_diff = x[i] - x_mean;
        numerator += x_diff * (y[i] - y_mean);
        denominator += x_diff * x_diff;
    }
    
    if (denominator == 0.0) return {{}, {CalcErr::DivideByZero}};
    
    double slope = numerator / denominator;
    double intercept = y_mean - slope * x_mean;
    
    // Return [slope, intercept]
    return EngineSuccessResult(Vector{slope, intercept});
}

EngineResult StatisticsEngine::Percentile(Vector data, double p) {
    if (data.empty() || p < 0 || p > 100) {
        return {{}, {CalcErr::ArgumentMismatch}};
    }
    
    std::sort(data.begin(), data.end());
    
    if (p == 0) return EngineSuccessResult(data[0]);
    if (p == 100) return EngineSuccessResult(data.back());
    
    double index = (p / 100.0) * (data.size() - 1);
    size_t lower = static_cast<size_t>(index);
    size_t upper = lower + 1;
    
    if (upper >= data.size()) {
        return EngineSuccessResult(data.back());
    }
    
    double weight = index - lower;
    double result = data[lower] * (1.0 - weight) + data[upper] * weight;
    
    return EngineSuccessResult(result);
}

EngineResult StatisticsEngine::MovingAverage(const Vector& data, int window_size) {
    if (data.empty() || window_size <= 0 || window_size > static_cast<int>(data.size())) {
        return {{}, {CalcErr::ArgumentMismatch}};
    }
    
    Vector result;
    result.reserve(data.size() - window_size + 1);
    
    for (size_t i = 0; i <= data.size() - window_size; ++i) {
        double sum = 0.0;
        for (int j = 0; j < window_size; ++j) {
            sum += data[i + j];
        }
        result.push_back(sum / window_size);
    }
    
    return EngineSuccessResult(result);
}