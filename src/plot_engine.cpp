#include "plot_engine.h"
#include "algebraic_parser.h"
#include <sstream>
#include <algorithm>
#include <cmath>
#include <iomanip>

std::string PlotEngine::PlotFunction(const std::string& expression, const PlotConfig& config) {
    AlgebraicParser parser;
    std::vector<std::string> lines(config.height, std::string(config.width, ' '));
    
    // Calculate step size
    double x_step = (config.x_max - config.x_min) / (config.width - 1);
    
    for (int col = 0; col < config.width; ++col) {
        double x = config.x_min + col * x_step;
        
        std::map<std::string, double> context;
        context["x"] = x;
        
        auto result = parser.ParseAndExecuteWithContext(expression, context);
        if (result.result.has_value()) {
            auto y_opt = result.GetDouble();
            if (y_opt.has_value()) {
                double y = *y_opt;
            
                if (std::isfinite(y) && y >= config.y_min && y <= config.y_max) {
                    auto [screen_x, screen_y] = MapToScreen(x, y, config);
                    if (screen_x >= 0 && screen_x < config.width && 
                        screen_y >= 0 && screen_y < config.height) {
                        lines[screen_y][screen_x] = config.plot_char;
                    }
                }
            }
        }
    }
    
    // Add axes if requested
    if (config.show_axes) {
        // Y-axis (x = 0)
        if (0 >= config.x_min && 0 <= config.x_max) {
            auto [axis_x, _] = MapToScreen(0, 0, config);
            for (int row = 0; row < config.height; ++row) {
                if (axis_x >= 0 && axis_x < config.width) {
                    if (lines[row][axis_x] == ' ') {
                        lines[row][axis_x] = '|';
                    }
                }
            }
        }
        
        // X-axis (y = 0)
        if (0 >= config.y_min && 0 <= config.y_max) {
            auto [_, axis_y] = MapToScreen(0, 0, config);
            if (axis_y >= 0 && axis_y < config.height) {
                for (int col = 0; col < config.width; ++col) {
                    if (lines[axis_y][col] == ' ') {
                        lines[axis_y][col] = '-';
                    }
                }
            }
        }
    }
    
    // Convert to string
    std::stringstream result;
    for (const auto& line : lines) {
        result << line << "\n";
    }
    
    return result.str();
}

Matrix PlotEngine::ComputeFunctionData(const std::string& expression, const PlotConfig& config, int samples) {
    AlgebraicParser parser;
    Matrix data;
    // Determine sample count: default to plot width for consistency
    const int n = (samples > 0) ? samples : std::max(2, config.width);
    const double x_step = (config.x_max - config.x_min) / (n - 1);

    data.reserve(static_cast<size_t>(n));

    for (int i = 0; i < n; ++i) {
        const double x = config.x_min + i * x_step;
        std::map<std::string, double> context;
        context["x"] = x;

        auto result = parser.ParseAndExecuteWithContext(expression, context);
        if (result.result.has_value()) {
            auto y_opt = result.GetDouble();
            if (y_opt.has_value()) {
                const double y = *y_opt;
                if (std::isfinite(y)) {
                    data.push_back({x, y});
                }
            }
        }
    }

    return data;
}

std::string PlotEngine::PlotData(const Vector& x_data, const Vector& y_data, const PlotConfig& config) {
    if (x_data.size() != y_data.size() || x_data.empty()) {
        return "Error: Data vectors must be same size and non-empty\n";
    }
    
    std::vector<std::string> lines(config.height, std::string(config.width, ' '));
    
    for (size_t i = 0; i < x_data.size(); ++i) {
        double x = x_data[i];
        double y = y_data[i];
        
        if (std::isfinite(x) && std::isfinite(y) && 
            x >= config.x_min && x <= config.x_max &&
            y >= config.y_min && y <= config.y_max) {
            
            auto [screen_x, screen_y] = MapToScreen(x, y, config);
            if (screen_x >= 0 && screen_x < config.width && 
                screen_y >= 0 && screen_y < config.height) {
                lines[screen_y][screen_x] = config.plot_char;
            }
        }
    }
    
    std::stringstream result;
    for (const auto& line : lines) {
        result << line << "\\n";
    }
    
    return result.str();
}

std::string PlotEngine::Histogram(const Vector& data, int bins, const PlotConfig& config) {
    if (data.empty() || bins <= 0) {
        return "Error: Data must be non-empty and bins > 0\n";
    }
    
    // Find data range
    auto [min_it, max_it] = std::minmax_element(data.begin(), data.end());
    double data_min = *min_it;
    double data_max = *max_it;
    
    if (data_min == data_max) {
        return "Error: All data points are identical\n";
    }
    
    // Create histogram bins
    std::vector<int> hist(bins, 0);
    double bin_width = (data_max - data_min) / bins;
    
    for (double val : data) {
        int bin = static_cast<int>((val - data_min) / bin_width);
        if (bin >= bins) bin = bins - 1;  // Handle edge case
        hist[bin]++;
    }
    
    // Find max frequency for scaling
    int max_freq = *std::max_element(hist.begin(), hist.end());
    if (max_freq == 0) return "Error: No valid data\n";
    
    std::stringstream result;
    result << "Histogram (" << data.size() << " points, " << bins << " bins):\n";
    
    for (int i = bins - 1; i >= 0; --i) {
        double bin_start = data_min + i * bin_width;
        double bin_end = bin_start + bin_width;
        
        int bar_length = static_cast<int>((hist[i] * config.width) / max_freq);
        
        result << std::fixed << std::setprecision(2) 
               << "[" << bin_start << "-" << bin_end << ") ";
        
        for (int j = 0; j < bar_length; ++j) {
            result << config.plot_char;
        }
        result << " (" << hist[i] << ")\n";
    }
    
    return result.str();
}

std::pair<int, int> PlotEngine::MapToScreen(double x, double y, const PlotConfig& config) {
    int screen_x = static_cast<int>((x - config.x_min) / (config.x_max - config.x_min) * (config.width - 1));
    int screen_y = static_cast<int>((config.y_max - y) / (config.y_max - config.y_min) * (config.height - 1));
    return {screen_x, screen_y};
}