/**
 * @file plot_engine.h
 * @brief Plotting and visualization capabilities using ASCII art
 * Generates terminal-based plots for functions and data
 */
#pragma once

#include "dynamic_calc_types.h"
#include <functional>

struct PlotConfig {
    int width = 80;
    int height = 20;
    double x_min = -10;
    double x_max = 10;
    double y_min = -5;
    double y_max = 5;
    bool show_grid = true;
    bool show_axes = true;
    char plot_char = '*';
};

class PlotEngine {
public:
    // Function plotting
    std::string PlotFunction(const std::string& expression, const PlotConfig& config = {});
    std::string PlotParametric(const std::string& x_expr, const std::string& y_expr, 
                              double t_min, double t_max, const PlotConfig& config = {});
    
    // Data generation for Python plotting
    // Samples the function over [x_min, x_max] returning rows of [x, y]
    Matrix ComputeFunctionData(const std::string& expression, const PlotConfig& config = {}, int samples = -1);

    // Data plotting
    std::string PlotData(const Vector& x_data, const Vector& y_data, const PlotConfig& config = {});
    std::string Histogram(const Vector& data, int bins = 10, const PlotConfig& config = {});
    std::string BoxPlot(const Vector& data, const PlotConfig& config = {});
    
    // 3D Surface plots (top-down view)
    std::string PlotSurface(const std::string& expression, const PlotConfig& config = {});
    
    // Specialized plots  
    std::string PlotComplex(const Vector& real_parts, const Vector& imag_parts, const PlotConfig& config = {});
    std::string PolarPlot(const std::string& r_expression, const PlotConfig& config = {});
    
private:
    char GetCharForValue(double value, double min_val, double max_val);
    std::pair<int, int> MapToScreen(double x, double y, const PlotConfig& config);
};