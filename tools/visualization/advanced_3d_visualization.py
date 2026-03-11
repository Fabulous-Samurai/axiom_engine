#!/usr/bin/env python3
"""
🚀 AXIOM 3D Visualization Suite
Advanced 3D plotting and visualization capabilities
"""

import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
import matplotlib.animation as animation
from matplotlib.widgets import Slider
import tkinter as tk
from tkinter import ttk, messagebox
import time

# Constants for default function strings
DEFAULT_SURFACE_FUNC = "sin(sqrt(x**2 + y**2))"
DEFAULT_PARAMETRIC_X = "cos(t)"
DEFAULT_PARAMETRIC_Y = "sin(t)"
DEFAULT_PARAMETRIC_Z = "t"

class Advanced3DVisualization:
    """🎯 Advanced 3D Visualization Suite 🎯"""
    
    def __init__(self):
        self.figures = []
        self.animations = []
    
    def surface_plot_3d(self, func_str=DEFAULT_SURFACE_FUNC, 
                       x_range=(-5, 5), y_range=(-5, 5), 
                       resolution=50, title="3D Surface"):
        """Create advanced 3D surface plot"""
        try:
            # Generate mesh
            x = np.linspace(x_range[0], x_range[1], resolution)
            y = np.linspace(y_range[0], y_range[1], resolution)
            X, Y = np.meshgrid(x, y)
            
            # Evaluate function
            # Safe evaluation with numpy functions
            safe_dict = {
                'sin': np.sin, 'cos': np.cos, 'tan': np.tan,
                'exp': np.exp, 'log': np.log, 'sqrt': np.sqrt,
                'abs': np.abs, 'pi': np.pi, 'e': np.e,
                'x': X, 'y': Y, 'X': X, 'Y': Y
            }
            
            Z = eval(func_str, {"__builtins__": {}}, safe_dict)
            
            # Create 3D plot
            fig = plt.figure(figsize=(14, 10))
            
            # Main surface plot
            ax1 = fig.add_subplot(221, projection='3d')
            surf = ax1.plot_surface(X, Y, Z, cmap='viridis', alpha=0.8, 
                                  linewidth=0, antialiased=True)
            ax1.set_title(f'Surface: {func_str}')
            ax1.set_xlabel('X')
            ax1.set_ylabel('Y')
            ax1.set_zlabel('Z')
            fig.colorbar(surf, ax=ax1, shrink=0.5)
            
            # Wireframe plot
            ax2 = fig.add_subplot(222, projection='3d')
            ax2.plot_wireframe(X, Y, Z, color='blue', alpha=0.7)
            ax2.set_title('Wireframe View')
            ax2.set_xlabel('X')
            ax2.set_ylabel('Y')
            ax2.set_zlabel('Z')
            
            # Contour plot
            ax3 = fig.add_subplot(223)
            contour = ax3.contour(X, Y, Z, levels=20)
            ax3.clabel(contour, inline=True, fontsize=8)
            ax3.set_title('Contour Plot')
            ax3.set_xlabel('X')
            ax3.set_ylabel('Y')
            
            # Filled contour plot
            ax4 = fig.add_subplot(224)
            contourf = ax4.contourf(X, Y, Z, levels=20, cmap='viridis')
            fig.colorbar(contourf, ax=ax4, shrink=0.5)
            ax4.set_title('Filled Contour')
            ax4.set_xlabel('X')
            ax4.set_ylabel('Y')
            
            plt.suptitle(f'🎯 Advanced 3D Visualization: {title}', 
                        fontsize=16, fontweight='bold')
            plt.tight_layout()
            plt.show()
            
            self.figures.append(fig)
            return fig, (X, Y, Z)
            
        except Exception as e:
            print(f"❌ 3D Surface plot error: {str(e)}")
            return None, None
    
    def parametric_3d_plot(self, x_func=DEFAULT_PARAMETRIC_X, y_func=DEFAULT_PARAMETRIC_Y, z_func=DEFAULT_PARAMETRIC_Z, 
                          t_range=(0, 4*np.pi), resolution=1000, title="Parametric Curve"):
        """Create parametric 3D curve"""
        try:
            # Parameter range
            t = np.linspace(t_range[0], t_range[1], resolution)
            
            # Safe evaluation
            safe_dict = {
                'sin': np.sin, 'cos': np.cos, 'tan': np.tan,
                'exp': np.exp, 'log': np.log, 'sqrt': np.sqrt,
                'abs': np.abs, 'pi': np.pi, 'e': np.e, 't': t
            }
            
            # Evaluate parametric equations
            x = eval(x_func, {"__builtins__": {}}, safe_dict)
            y = eval(y_func, {"__builtins__": {}}, safe_dict)
            z = eval(z_func, {"__builtins__": {}}, safe_dict)
            
            # Create 3D plot
            fig = plt.figure(figsize=(12, 8))
            ax = fig.add_subplot(111, projection='3d')
            
            # Plot parametric curve with color gradient
            _ = ax.plot(x, y, z, linewidth=3, alpha=0.8)
            
            # Add starting and ending points
            ax.scatter([x[0]], [y[0]], [z[0]], color='green', s=100, label='Start')
            ax.scatter([x[-1]], [y[-1]], [z[-1]], color='red', s=100, label='End')
            
            ax.set_title(f'Parametric 3D Curve\nX(t)={x_func}, Y(t)={y_func}, Z(t)={z_func}')
            ax.set_xlabel('X')
            ax.set_ylabel('Y')
            ax.set_zlabel('Z')
            ax.legend()
            
            plt.suptitle(f'🎯 {title}', fontsize=16, fontweight='bold')
            plt.show()
            
            self.figures.append(fig)
            return fig, (x, y, z, t)
            
        except Exception as e:
            print(f"❌ Parametric 3D plot error: {str(e)}")
            return None, None
    
    def vector_field_3d(self, x_range=(-2, 2), y_range=(-2, 2), z_range=(-2, 2), 
                       resolution=8, title="3D Vector Field"):
        """Create 3D vector field visualization"""
        try:
            # Generate 3D grid
            x = np.linspace(x_range[0], x_range[1], resolution)
            y = np.linspace(y_range[0], y_range[1], resolution)
            z = np.linspace(z_range[0], z_range[1], resolution)
            X, Y, Z = np.meshgrid(x, y, z)
            
            # Example vector field: F = (-y, x, z)
            U = -Y  # dx component
            V = X   # dy component
            W = 0.1 * Z  # dz component
            
            # Create 3D vector field plot
            fig = plt.figure(figsize=(12, 10))
            ax = fig.add_subplot(111, projection='3d')
            
            # Plot vector field
            ax.quiver(X, Y, Z, U, V, W, length=0.3, normalize=True, 
                     alpha=0.7, arrow_length_ratio=0.1)
            
            ax.set_title('3D Vector Field: F = (-y, x, 0.1*z)')
            ax.set_xlabel('X')
            ax.set_ylabel('Y')
            ax.set_zlabel('Z')
            
            plt.suptitle(f'🎯 {title}', fontsize=16, fontweight='bold')
            plt.show()
            
            self.figures.append(fig)
            return fig, (X, Y, Z, U, V, W)
            
        except Exception as e:
            print(f"❌ 3D Vector field error: {str(e)}")
            return None, None
    
    def animated_3d_surface(self, base_func=DEFAULT_SURFACE_FUNC, 
                           time_modulation=DEFAULT_PARAMETRIC_X, duration=10):
        """Create animated 3D surface"""
        try:
            # Generate mesh
            x = np.linspace(-5, 5, 50)
            y = np.linspace(-5, 5, 50)
            X, Y = np.meshgrid(x, y)
            
            # Create figure and axis
            fig = plt.figure(figsize=(12, 8))
            ax = fig.add_subplot(111, projection='3d')
            
            # Initialize plot
            t = 0
            safe_dict = {
                'sin': np.sin, 'cos': np.cos, 'tan': np.tan,
                'exp': np.exp, 'log': np.log, 'sqrt': np.sqrt,
                'abs': np.abs, 'pi': np.pi, 'e': np.e,
                'x': X, 'y': Y, 't': t
            }
            
            z_base = eval(base_func, {"__builtins__": {}}, safe_dict)
            time_mod = eval(time_modulation, {"__builtins__": {}}, safe_dict)
            Z = z_base * time_mod
            
            _ = ax.plot_surface(X, Y, Z, cmap='viridis', alpha=0.8)
            ax.set_title(f'Animated Surface: {base_func} * {time_modulation}')
            ax.set_xlabel('X')
            ax.set_ylabel('Y')
            ax.set_zlabel('Z')
            ax.set_zlim(-3, 3)
            
            def animate(frame):
                ax.clear()
                
                t = frame * 0.1
                safe_dict['t'] = t
                
                z_base = eval(base_func, {"__builtins__": {}}, safe_dict)
                time_mod = eval(time_modulation, {"__builtins__": {}}, safe_dict)
                Z = z_base * time_mod
                
                surface = ax.plot_surface(X, Y, Z, cmap='viridis', alpha=0.8)
                ax.set_title(f'Animated Surface: {base_func} * {time_modulation} (t={t:.1f})')
                ax.set_xlabel('X')
                ax.set_ylabel('Y')
                ax.set_zlabel('Z')
                ax.set_zlim(-3, 3)
                
                return [surface]
            
            # Create animation
            frames = int(duration * 10)  # 10 fps
            anim = animation.FuncAnimation(fig, animate, frames=frames, 
                                         interval=100, blit=False)
            
            plt.suptitle('🎯 Animated 3D Surface', fontsize=16, fontweight='bold')
            plt.show()
            
            self.animations.append(anim)
            return fig, anim
            
        except Exception as e:
            print(f"❌ Animated 3D surface error: {str(e)}")
            return None, None
    
    def molecular_visualization(self, molecule_type="water"):
        """Visualize molecular structures"""
        try:
            fig = plt.figure(figsize=(12, 8))
            ax = fig.add_subplot(111, projection='3d')
            
            if molecule_type == "water":
                # Water molecule H2O
                # Oxygen at origin
                ax.scatter([0], [0], [0], c='red', s=1000, alpha=0.8, label='O')
                
                # Hydrogen atoms
                bond_length = 1.0
                bond_angle = 104.5 * np.pi / 180  # HOH angle in radians
                
                h1_x = bond_length * np.sin(bond_angle/2)
                h1_y = bond_length * np.cos(bond_angle/2)
                h1_z = 0
                
                h2_x = -bond_length * np.sin(bond_angle/2)
                h2_y = bond_length * np.cos(bond_angle/2)
                h2_z = 0
                
                ax.scatter([h1_x, h2_x], [h1_y, h2_y], [h1_z, h2_z], 
                          c='white', s=500, alpha=0.8, label='H')
                
                # Draw bonds
                ax.plot([0, h1_x], [0, h1_y], [0, h1_z], 'k-', linewidth=3)
                ax.plot([0, h2_x], [0, h2_y], [0, h2_z], 'k-', linewidth=3)
                
                ax.set_title('Water Molecule (H₂O)')
                
            elif molecule_type == "methane":
                # Methane molecule CH4 (tetrahedral)
                # Carbon at origin
                ax.scatter([0], [0], [0], c='black', s=1000, alpha=0.8, label='C')
                
                # Hydrogen atoms in tetrahedral arrangement
                bond_length = 1.1
                # Tetrahedral angle = 109.47° (for reference)
                
                # Tetrahedral coordinates
                h_coords = [
                    [1, 1, 1],
                    [1, -1, -1],
                    [-1, 1, -1], 
                    [-1, -1, 1]
                ]
                
                h_coords = np.array(h_coords) * bond_length / np.sqrt(3)
                
                ax.scatter(h_coords[:, 0], h_coords[:, 1], h_coords[:, 2],
                          c='white', s=500, alpha=0.8, label='H')
                
                # Draw bonds
                for h_coord in h_coords:
                    ax.plot([0, h_coord[0]], [0, h_coord[1]], [0, h_coord[2]], 
                           'k-', linewidth=3)
                
                ax.set_title('Methane Molecule (CH₄)')
            
            ax.set_xlabel('X (Å)')
            ax.set_ylabel('Y (Å)')
            ax.set_zlabel('Z (Å)')
            ax.legend()
            
            # Equal aspect ratio
            max_range = 1.5
            ax.set_xlim([-max_range, max_range])
            ax.set_ylim([-max_range, max_range])
            ax.set_zlim([-max_range, max_range])
            
            plt.suptitle(f'🧬 Molecular Visualization: {molecule_type.title()}', 
                        fontsize=16, fontweight='bold')
            plt.show()
            
            self.figures.append(fig)
            return fig
            
        except Exception as e:
            print(f"❌ Molecular visualization error: {str(e)}")
            return None
    
    def interactive_3d_plot(self, func_str="sin(x)*cos(y)"):
        """Create interactive 3D plot with sliders"""
        try:
            # Generate initial mesh
            x = np.linspace(-5, 5, 50)
            y = np.linspace(-5, 5, 50)
            X, Y = np.meshgrid(x, y)
            
            # Create figure with subplots for sliders
            fig = plt.figure(figsize=(14, 10))
            ax = fig.add_subplot(111, projection='3d')
            plt.subplots_adjust(bottom=0.25)
            
            # Initial parameters
            amplitude = 1.0
            frequency = 1.0
            
            def update_surface():
                safe_dict = {
                    'sin': np.sin, 'cos': np.cos, 'tan': np.tan,
                    'exp': np.exp, 'log': np.log, 'sqrt': np.sqrt,
                    'abs': np.abs, 'pi': np.pi, 'e': np.e,
                    'x': X * frequency, 'y': Y * frequency, 'A': amplitude
                }
                
                Z = amplitude * eval(func_str, {"__builtins__": {}}, safe_dict)
                return Z
            
            # Initial surface
            Z = update_surface()
            surface = ax.plot_surface(X, Y, Z, cmap='viridis', alpha=0.8)
            
            ax.set_title(f'Interactive 3D: {func_str}')
            ax.set_xlabel('X')
            ax.set_ylabel('Y')
            ax.set_zlabel('Z')
            
            # Add sliders
            ax_amp = plt.axes([0.2, 0.1, 0.5, 0.03])
            slider_amp = Slider(ax_amp, 'Amplitude', 0.1, 3.0, valinit=amplitude)
            
            ax_freq = plt.axes([0.2, 0.05, 0.5, 0.03])
            slider_freq = Slider(ax_freq, 'Frequency', 0.1, 3.0, valinit=frequency)
            
            def update(val):
                nonlocal surface, amplitude, frequency
                amplitude = slider_amp.val
                frequency = slider_freq.val
                
                ax.clear()
                Z = update_surface()
                surface = ax.plot_surface(X, Y, Z, cmap='viridis', alpha=0.8)
                ax.set_title(f'Interactive 3D: {func_str} (A={amplitude:.1f}, f={frequency:.1f})')
                ax.set_xlabel('X')
                ax.set_ylabel('Y')
                ax.set_zlabel('Z')
                plt.draw()
            
            slider_amp.on_changed(update)
            slider_freq.on_changed(update)
            
            plt.suptitle('🎯 Interactive 3D Visualization', fontsize=16, fontweight='bold')
            plt.show()
            
            self.figures.append(fig)
            return fig
            
        except Exception as e:
            print(f"❌ Interactive 3D plot error: {str(e)}")
            return None
    
    def create_3d_gui(self):
        """Create 3D visualization GUI"""
        viz_window = tk.Toplevel()
        viz_window.title("🎯 3D Visualization Suite")
        viz_window.geometry("800x700")
        
        # Header
        ttk.Label(viz_window, text="🎯 Advanced 3D Visualization Suite", 
                 font=('Segoe UI', 16, 'bold')).pack(pady=10)
        
        # Function input frame
        func_frame = ttk.LabelFrame(viz_window, text="Function Definition")
        func_frame.pack(fill=tk.X, padx=20, pady=10)
        
        ttk.Label(func_frame, text="Z = f(x,y):").pack(side=tk.LEFT, padx=5)
        func_var = tk.StringVar(value=DEFAULT_SURFACE_FUNC)
        func_entry = ttk.Entry(func_frame, textvariable=func_var, width=40)
        func_entry.pack(side=tk.LEFT, padx=5, fill=tk.X, expand=True)
        
        # Range settings
        range_frame = ttk.LabelFrame(viz_window, text="Plot Range")
        range_frame.pack(fill=tk.X, padx=20, pady=10)
        
        ttk.Label(range_frame, text="X Range:").grid(row=0, column=0, sticky='w', padx=5, pady=2)
        x_min_var = tk.DoubleVar(value=-5)
        x_max_var = tk.DoubleVar(value=5)
        ttk.Entry(range_frame, textvariable=x_min_var, width=8).grid(row=0, column=1, padx=2, pady=2)
        ttk.Label(range_frame, text="to").grid(row=0, column=2, padx=2, pady=2)
        ttk.Entry(range_frame, textvariable=x_max_var, width=8).grid(row=0, column=3, padx=2, pady=2)
        
        ttk.Label(range_frame, text="Y Range:").grid(row=0, column=4, sticky='w', padx=5, pady=2)
        y_min_var = tk.DoubleVar(value=-5)
        y_max_var = tk.DoubleVar(value=5)
        ttk.Entry(range_frame, textvariable=y_min_var, width=8).grid(row=0, column=5, padx=2, pady=2)
        ttk.Label(range_frame, text="to").grid(row=0, column=6, padx=2, pady=2)
        ttk.Entry(range_frame, textvariable=y_max_var, width=8).grid(row=0, column=7, padx=2, pady=2)
        
        # Resolution
        ttk.Label(range_frame, text="Resolution:").grid(row=1, column=0, sticky='w', padx=5, pady=2)
        res_var = tk.IntVar(value=50)
        ttk.Entry(range_frame, textvariable=res_var, width=8).grid(row=1, column=1, padx=2, pady=2)
        
        # Visualization buttons
        viz_buttons_frame = ttk.LabelFrame(viz_window, text="Visualization Options")
        viz_buttons_frame.pack(fill=tk.X, padx=20, pady=10)
        
        def plot_surface():
            try:
                func = func_var.get()
                x_range = (x_min_var.get(), x_max_var.get())
                y_range = (y_min_var.get(), y_max_var.get())
                resolution = res_var.get()
                
                self.surface_plot_3d(func, x_range, y_range, resolution)
            except Exception as e:
                messagebox.showerror("Error", f"Surface plot failed: {str(e)}")
        
        def plot_interactive():
            try:
                func = func_var.get()
                self.interactive_3d_plot(func)
            except Exception as e:
                messagebox.showerror("Error", f"Interactive plot failed: {str(e)}")
        
        buttons = [
            ("🏔️ Surface Plot", plot_surface),
            ("🎛️ Interactive Plot", plot_interactive),
            ("🌊 Vector Field", lambda: self.vector_field_3d()),
            ("🧬 Water Molecule", lambda: self.molecular_visualization("water")),
            ("🧬 Methane Molecule", lambda: self.molecular_visualization("methane")),
            ("🎬 Animated Surface", lambda: self.animated_3d_surface())
        ]
        
        for i, (text, command) in enumerate(buttons):
            ttk.Button(viz_buttons_frame, text=text, command=command).grid(
                row=i//2, column=i%2, padx=10, pady=5, sticky='ew')
        
        viz_buttons_frame.grid_columnconfigure(0, weight=1)
        viz_buttons_frame.grid_columnconfigure(1, weight=1)
        
        # Parametric curve section
        param_frame = ttk.LabelFrame(viz_window, text="Parametric Curves")
        param_frame.pack(fill=tk.X, padx=20, pady=10)
        
        ttk.Label(param_frame, text="X(t):").grid(row=0, column=0, sticky='w', padx=5, pady=2)
        x_param_var = tk.StringVar(value=DEFAULT_PARAMETRIC_X)
        ttk.Entry(param_frame, textvariable=x_param_var, width=15).grid(row=0, column=1, padx=5, pady=2)
        
        ttk.Label(param_frame, text="Y(t):").grid(row=0, column=2, sticky='w', padx=5, pady=2)
        y_param_var = tk.StringVar(value=DEFAULT_PARAMETRIC_Y)
        ttk.Entry(param_frame, textvariable=y_param_var, width=15).grid(row=0, column=3, padx=5, pady=2)
        
        ttk.Label(param_frame, text="Z(t):").grid(row=1, column=0, sticky='w', padx=5, pady=2)
        z_param_var = tk.StringVar(value="t")
        ttk.Entry(param_frame, textvariable=z_param_var, width=15).grid(row=1, column=1, padx=5, pady=2)
        
        ttk.Label(param_frame, text="t Range:").grid(row=1, column=2, sticky='w', padx=5, pady=2)
        t_range_var = tk.StringVar(value="0, 4*pi")
        ttk.Entry(param_frame, textvariable=t_range_var, width=15).grid(row=1, column=3, padx=5, pady=2)
        
        def plot_parametric():
            try:
                x_func = x_param_var.get()
                y_func = y_param_var.get()
                z_func = z_param_var.get()
                t_range_str = t_range_var.get()
                
                # Parse t range
                t_min, t_max = eval(f"({t_range_str})", {"pi": np.pi})
                t_range = (t_min, t_max)
                
                self.parametric_3d_plot(x_func, y_func, z_func, t_range)
            except Exception as e:
                messagebox.showerror("Error", f"Parametric plot failed: {str(e)}")
        
        ttk.Button(param_frame, text="📈 Plot Parametric Curve", 
                  command=plot_parametric).grid(row=2, column=0, columnspan=4, pady=10)
    
    def demonstrate_3d_capabilities(self):
        """Demonstrate 3D visualization capabilities"""
        print("🎯 AXIOM 3D Visualization Suite Demonstration")
        print("=" * 60)
        
        # Surface plot
        print("🏔️ Creating surface plot...")
        self.surface_plot_3d(DEFAULT_SURFACE_FUNC, title="Ripple Function")
        
        # Parametric plot
        print("📈 Creating parametric curve...")
        self.parametric_3d_plot(DEFAULT_PARAMETRIC_X, DEFAULT_PARAMETRIC_Y, "t/3", (0, 6*np.pi), title="Helix")
        
        # Molecular visualization
        print("🧬 Creating molecular visualization...")
        self.molecular_visualization("water")
        
        print("✅ 3D Visualization Suite Ready!")

if __name__ == "__main__":
    print("🚀 Starting 3D Visualization Suite...")
    
    # Create visualization suite
    viz_suite = Advanced3DVisualization()
    
    # Demonstrate capabilities
    viz_suite.demonstrate_3d_capabilities()
    
    # Start GUI
    root = tk.Tk()
    root.withdraw()  # Hide main window
    
    # Open 3D GUI
    viz_suite.create_3d_gui()
    
    root.mainloop()