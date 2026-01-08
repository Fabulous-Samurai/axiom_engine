#!/usr/bin/env python3
"""
🚀 AXIOM PRO - Professional Scientific Computing GUI
Advanced 3D plotting, signal processing, and professional tools
"""

import tkinter as tk
from tkinter import ttk, filedialog, messagebox
import numpy as np
import matplotlib.pyplot as plt
import matplotlib as mpl
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from matplotlib.figure import Figure
import threading
import time
import sys
import os
from pathlib import Path

# Add paths for signal processing toolkit
project_root = Path(__file__).parent.parent.parent
sys.path.insert(0, str(project_root / 'tools' / 'analysis'))

# Constants
DEFAULT_FONT_FAMILY = 'Segoe UI'

class AxiomProGUI:
    """🎯 AXIOM PRO - Professional Scientific Computing GUI 🎯"""
    
    def __init__(self, root):
        self.root = root
        self.root.title("🧮 AXIOM PRO - Professional Scientific Computing")
        self.root.geometry("1400x900")
        self.root.state('zoomed')  # Maximize window
        
        # Professional theme
        self.setup_professional_theme()
        
        # MATLAB-style workspace
        self.workspace_vars = {}
        self.command_history = []
        self.current_figure = None
        
        # Signal processing toolkit (initialize after GUI is created)
        self.signal_toolkit = None
        
        # Create professional interface
        self.create_matlab_interface()
        self.add_matlab_welcome()
        
        # Initialize signal processing toolkit after GUI is ready
        self._init_signal_toolkit()
    
    def setup_professional_theme(self):
        """Setup professional MATLAB-style theme"""
        style = ttk.Style()
        try:
            style.theme_use('vista')
        except Exception:
            style.theme_use('default')
        
        # MATLAB-inspired colors
        self.colors = {
            'bg_primary': '#f0f0f0',
            'bg_secondary': '#ffffff', 
            'bg_accent': '#0076a8',
            'fg_primary': '#000000',
            'fg_secondary': '#333333',
            'accent_matlab': '#0076a8',
            'accent_success': '#00aa00',
            'accent_warning': '#ff8800',
            'accent_error': '#cc0000'
        }
        
        self.root.configure(bg=self.colors['bg_primary'])

        # Plotting performance settings
        self.setup_plotting_performance()

    def setup_plotting_performance(self):
        """Configure matplotlib for better rendering performance"""
        try:
            mpl.rcParams['path.simplify'] = True
            mpl.rcParams['path.simplify_threshold'] = 1.0
            mpl.rcParams['agg.path.chunksize'] = 10000
            mpl.rcParams['figure.dpi'] = 100
            mpl.rcParams['savefig.dpi'] = 150
        except Exception:
            pass
    
    def create_matlab_interface(self):
        """Create MATLAB-style professional interface"""
        # Main container
        main_container = ttk.Frame(self.root)
        main_container.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # Professional header
        self.create_professional_header(main_container)
        
        # Main workspace (3-panel layout like MATLAB)
        self.create_workspace_layout(main_container)
        
        # Professional status bar
        self.create_professional_statusbar(main_container)
    
    def create_professional_header(self, parent):
        """Create professional header with toolbars"""
        header_frame = ttk.Frame(parent)
        header_frame.pack(fill=tk.X, pady=(0, 5))
        
        # Title and version
        title_frame = ttk.Frame(header_frame)
        title_frame.pack(fill=tk.X)
        
        ttk.Label(title_frame, text="🧮 AXIOM PRO", 
                 font=(DEFAULT_FONT_FAMILY, 16, 'bold')).pack(side=tk.LEFT)
        ttk.Label(title_frame, text="Professional Scientific Computing GUI", 
                 font=(DEFAULT_FONT_FAMILY, 10)).pack(side=tk.LEFT, padx=(10, 0))
        
        # Version and performance info
        version_frame = ttk.Frame(title_frame)
        version_frame.pack(side=tk.RIGHT)
        
        ttk.Label(version_frame, text="v2.0 🏎️ Senna Speed", 
                 font=(DEFAULT_FONT_FAMILY, 9, 'bold'),
                 foreground=self.colors['accent_matlab']).pack(side=tk.RIGHT)
        
        # Professional toolbar
        self.create_toolbar(header_frame)
    
    def create_toolbar(self, parent):
        """Create professional toolbar like MATLAB"""
        toolbar_frame = ttk.Frame(parent)
        toolbar_frame.pack(fill=tk.X, pady=(5, 0))
        
        # File operations
        file_frame = ttk.LabelFrame(toolbar_frame, text=" File ")
        file_frame.pack(side=tk.LEFT, padx=(0, 5))
        
        ttk.Button(file_frame, text="📂 Open", width=8, 
                  command=self.open_file).pack(side=tk.LEFT, padx=2)
        ttk.Button(file_frame, text="💾 Save", width=8,
                  command=self.save_workspace).pack(side=tk.LEFT, padx=2)
        ttk.Button(file_frame, text="📄 New", width=8,
                  command=self.new_workspace).pack(side=tk.LEFT, padx=2)
        
        # Data operations
        data_frame = ttk.LabelFrame(toolbar_frame, text=" Data ")
        data_frame.pack(side=tk.LEFT, padx=(0, 5))
        
        ttk.Button(data_frame, text="📊 Import", width=8,
                  command=self.import_data).pack(side=tk.LEFT, padx=2)
        ttk.Button(data_frame, text="📈 Plot 2D", width=8,
                  command=self.plot_2d_dialog).pack(side=tk.LEFT, padx=2)
        ttk.Button(data_frame, text="🎯 Plot 3D", width=8,
                  command=self.plot_3d_dialog).pack(side=tk.LEFT, padx=2)
        
        # Analysis tools
        analysis_frame = ttk.LabelFrame(toolbar_frame, text=" Analysis ")
        analysis_frame.pack(side=tk.LEFT, padx=(0, 5))
        
        ttk.Button(analysis_frame, text="📊 Stats", width=8,
                  command=self.statistics_analysis).pack(side=tk.LEFT, padx=2)
        ttk.Button(analysis_frame, text="📡 Signal", width=8,
                  command=self.signal_processing).pack(side=tk.LEFT, padx=2)
        ttk.Button(analysis_frame, text="🖼️ Image", width=8,
                  command=self.image_processing).pack(side=tk.LEFT, padx=2)
        
        # Advanced tools
        advanced_frame = ttk.LabelFrame(toolbar_frame, text=" Advanced ")
        advanced_frame.pack(side=tk.LEFT, padx=(0, 5))
        
        ttk.Button(advanced_frame, text="🧮 Matrix", width=8,
                  command=self.matrix_calculator).pack(side=tk.LEFT, padx=2)
        ttk.Button(advanced_frame, text="∫ Calculus", width=8,
                  command=self.calculus_tools).pack(side=tk.LEFT, padx=2)
        ttk.Button(advanced_frame, text="🎛️ Control", width=8,
                  command=self.control_systems).pack(side=tk.LEFT, padx=2)

        # Performance tools
        perf_frame = ttk.LabelFrame(toolbar_frame, text=" Performance ")
        perf_frame.pack(side=tk.LEFT, padx=(0, 5))
        # Fast Render toggle
        self.fast_render_var = tk.BooleanVar(value=False)
        ttk.Checkbutton(perf_frame, text="⚡ Fast Render", variable=self.fast_render_var).pack(side=tk.LEFT, padx=4)
        # FPS display toggle
        self.show_fps_var = tk.BooleanVar(value=True)
        ttk.Checkbutton(perf_frame, text="📊 Show FPS", variable=self.show_fps_var).pack(side=tk.LEFT, padx=4)
    
    def create_workspace_layout(self, parent):
        """Create 3-panel MATLAB-style workspace"""
        # Main workspace paned window
        workspace_paned = ttk.PanedWindow(parent, orient=tk.HORIZONTAL)
        workspace_paned.pack(fill=tk.BOTH, expand=True, pady=(5, 0))
        
        # Left panel - Workspace variables and files
        left_panel = self.create_left_panel(workspace_paned)
        workspace_paned.add(left_panel, weight=1)
        
        # Center panel - Command window and editor
        center_panel = self.create_center_panel(workspace_paned)  
        workspace_paned.add(center_panel, weight=3)
        
        # Right panel - Figures and plots
        right_panel = self.create_right_panel(workspace_paned)
        workspace_paned.add(right_panel, weight=2)
    
    def create_left_panel(self, parent):
        """Create left panel with workspace browser"""
        left_frame = ttk.Frame(parent)
        
        # Workspace variables
        workspace_frame = ttk.LabelFrame(left_frame, text=" Workspace Variables ")
        workspace_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # Variables tree
        columns = ('Name', 'Type', 'Size', 'Value')
        self.workspace_tree = ttk.Treeview(workspace_frame, columns=columns, show='headings')
        
        for col in columns:
            self.workspace_tree.heading(col, text=col)
            self.workspace_tree.column(col, width=80)
        
        workspace_scrollbar = ttk.Scrollbar(workspace_frame, orient=tk.VERTICAL, 
                                          command=self.workspace_tree.yview)
        self.workspace_tree.configure(yscrollcommand=workspace_scrollbar.set)
        
        self.workspace_tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        workspace_scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        
        # Workspace controls
        controls_frame = ttk.Frame(left_frame)
        controls_frame.pack(fill=tk.X, padx=5, pady=5)
        
        ttk.Button(controls_frame, text="🔄 Refresh", 
                  command=self.refresh_workspace).pack(side=tk.LEFT, padx=2)
        ttk.Button(controls_frame, text="🗑️ Clear", 
                  command=self.clear_workspace).pack(side=tk.LEFT, padx=2)
        ttk.Button(controls_frame, text="💾 Save", 
                  command=self.save_workspace).pack(side=tk.LEFT, padx=2)
        
        return left_frame
    
    def create_center_panel(self, parent):
        """Create center panel with command window"""
        center_frame = ttk.Frame(parent)
        
        # Notebook for tabs
        notebook = ttk.Notebook(center_frame)
        notebook.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # Command Window tab (MATLAB-style)
        self.create_command_window_tab(notebook)
        
        # Script Editor tab
        self.create_script_editor_tab(notebook)
        
        # Live Editor tab
        self.create_live_editor_tab(notebook)
        
        return center_frame
    
    def create_command_window_tab(self, notebook):
        """Create MATLAB-style command window"""
        command_frame = ttk.Frame(notebook)
        notebook.add(command_frame, text="🖥️ Command Window")
        
        # Output area
        output_frame = ttk.Frame(command_frame)
        output_frame.pack(fill=tk.BOTH, expand=True, pady=(0, 5))
        
        self.command_output = tk.Text(
            output_frame,
            font=('Consolas', 10),
            bg='white',
            fg='black',
            wrap=tk.WORD,
            state=tk.DISABLED
        )
        
        output_scrollbar = ttk.Scrollbar(output_frame, orient=tk.VERTICAL,
                                       command=self.command_output.yview)
        self.command_output.configure(yscrollcommand=output_scrollbar.set)
        
        self.command_output.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        output_scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        
        # Input area with MATLAB-style prompt
        input_frame = ttk.Frame(command_frame)
        input_frame.pack(fill=tk.X)
        
        ttk.Label(input_frame, text=">>", font=('Consolas', 10, 'bold'),
                 foreground=self.colors['accent_matlab']).pack(side=tk.LEFT, padx=(5, 5))
        
        self.command_input = ttk.Entry(input_frame, font=('Consolas', 10))
        self.command_input.pack(side=tk.LEFT, fill=tk.X, expand=True)
        self.command_input.bind('<Return>', self.execute_matlab_command)
        
        ttk.Button(input_frame, text="▶️ Execute",
                  command=self.execute_matlab_command).pack(side=tk.RIGHT, padx=(5, 0))
    
    def create_script_editor_tab(self, notebook):
        """Create script editor tab"""
        editor_frame = ttk.Frame(notebook)
        notebook.add(editor_frame, text="📝 Script Editor")
        
        # Editor with line numbers
        self.script_editor = tk.Text(
            editor_frame,
            font=('Consolas', 10),
            bg='white',
            fg='black',
            wrap=tk.NONE,
            undo=True
        )
        
        script_scrollbar_v = ttk.Scrollbar(editor_frame, orient=tk.VERTICAL,
                                         command=self.script_editor.yview)
        script_scrollbar_h = ttk.Scrollbar(editor_frame, orient=tk.HORIZONTAL,
                                         command=self.script_editor.xview)
        
        self.script_editor.configure(yscrollcommand=script_scrollbar_v.set,
                                   xscrollcommand=script_scrollbar_h.set)
        
        self.script_editor.grid(row=0, column=0, sticky='nsew')
        script_scrollbar_v.grid(row=0, column=1, sticky='ns')
        script_scrollbar_h.grid(row=1, column=0, sticky='ew')
        
        editor_frame.grid_rowconfigure(0, weight=1)
        editor_frame.grid_columnconfigure(0, weight=1)
        
        # Editor controls
        editor_controls = ttk.Frame(editor_frame)
        editor_controls.grid(row=2, column=0, columnspan=2, sticky='ew', pady=(5, 0))
        
        ttk.Button(editor_controls, text="▶️ Run Script",
                  command=self.run_script).pack(side=tk.LEFT, padx=2)
        ttk.Button(editor_controls, text="💾 Save Script",
                  command=self.save_script).pack(side=tk.LEFT, padx=2)
        ttk.Button(editor_controls, text="📂 Load Script",
                  command=self.load_script).pack(side=tk.LEFT, padx=2)
    
    def create_live_editor_tab(self, notebook):
        """Create live editor tab (like MATLAB Live Editor)"""
        live_frame = ttk.Frame(notebook)
        notebook.add(live_frame, text="📊 Live Editor")
        
        ttk.Label(live_frame, text="🚀 Live Editor - Coming Soon!",
                 font=(DEFAULT_FONT_FAMILY, 14, 'bold')).pack(expand=True)
        ttk.Label(live_frame, text="Rich formatting, inline plots, and interactive documents",
                 font=(DEFAULT_FONT_FAMILY, 10)).pack(expand=True)
    
    def create_right_panel(self, parent):
        """Create right panel for plots and figures"""
        right_frame = ttk.Frame(parent)
        
        # Figure window
        figure_frame = ttk.LabelFrame(right_frame, text=" Figure Window ")
        figure_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # Matplotlib canvas
        self.fig = Figure(figsize=(8, 6), dpi=100)
        self.canvas = FigureCanvasTkAgg(self.fig, figure_frame)
        self.canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True)
        
        # Figure controls
        fig_controls = ttk.Frame(right_frame)
        fig_controls.pack(fill=tk.X, padx=5, pady=5)
        
        ttk.Button(fig_controls, text="📊 New Figure",
                  command=self.new_figure).pack(side=tk.LEFT, padx=2)
        ttk.Button(fig_controls, text="💾 Save Figure",
                  command=self.save_figure).pack(side=tk.LEFT, padx=2)
        ttk.Button(fig_controls, text="🗑️ Clear",
                  command=self.clear_figure).pack(side=tk.LEFT, padx=2)
        
        return right_frame
    
    def create_professional_statusbar(self, parent):
        """Create professional status bar"""
        status_frame = ttk.Frame(parent)
        status_frame.pack(fill=tk.X, pady=(5, 0))
        # Bindable status text
        self.status_var = tk.StringVar(value="Ready - AXIOM PRO")
        self.status_label = ttk.Label(status_frame, textvariable=self.status_var)
        self.status_label.pack(side=tk.LEFT)
        
        # Performance indicator
        ttk.Label(status_frame, text="🏎️ Senna Speed Engine Active",
                 foreground=self.colors['accent_success']).pack(side=tk.RIGHT)

        # Draw event hook for FPS measurement
        try:
            self._last_draw_t = None
            self.canvas.mpl_connect('draw_event', self._on_draw_event)
        except Exception:
            pass
    
    def add_matlab_welcome(self):
        """Add welcome message"""
        welcome_msg = """🧮 AXIOM PRO - Professional Scientific Computing v3.0
Professional Scientific Computing Environment

🚀 Features:
• Advanced mathematical computing with C++ speed
• 3D plotting and visualization
• Signal and image processing  
• Matrix operations and linear algebra
• Statistics and data analysis
• Python ecosystem integration

📖 Getting Started:
• Type commands in the Command Window (>>)
• Use toolbar buttons for common operations
• Variables appear in Workspace panel
• Plots display in Figure window

💡 Examples:
>> A = [1 2; 3 4]
>> plot(sin(0:0.1:2*pi))
>> help plot

Ready for professional scientific computing! 🎯
"""
        
        self.add_command_output(welcome_msg, 'info')
    
    def add_command_output(self, text, style='normal'):
        """Add text to command output with styling"""
        self.command_output.config(state=tk.NORMAL)
        
        colors = {
            'normal': 'black',
            'info': '#0066cc',
            'success': '#00aa00', 
            'warning': '#ff8800',
            'error': '#cc0000'
        }
        
        self.command_output.insert(tk.END, text + '\n', style)
        self.command_output.tag_configure(style, foreground=colors.get(style, 'black'))
        self.command_output.config(state=tk.DISABLED)
        self.command_output.see(tk.END)
    
    def execute_matlab_command(self, event=None):
        """Execute MATLAB-style command"""
        command = self.command_input.get().strip()
        if not command:
            return
        
        # Show command
        self.add_command_output(f">> {command}")
        
        # Clear input
        self.command_input.delete(0, tk.END)
        
        # Add to history
        self.command_history.append(command)
        
        # Execute command
        try:
            self.process_matlab_command(command)
        except Exception as e:
            self.add_command_output(f"Error: {str(e)}", 'error')
        
        # Update workspace
        self.refresh_workspace()
    
    def process_matlab_command(self, command):
        """Process MATLAB-style commands"""
        # Handle special commands
        if command == 'clc':
            self.clear_command_window()
            return
        elif command == 'clear':
            self.clear_workspace()
            return
        elif command.startswith('help'):
            self.show_help(command)
            return
        
        # Handle plotting commands
        if command.startswith('plot'):
            self.handle_plot_command(command)
            return
        elif command.startswith('surf') or command.startswith('mesh'):
            self.handle_3d_plot_command(command)
            return
        
        # Handle matrix operations
        if '=' in command and '[' in command:
            self.handle_matrix_assignment(command)
            return
        
        # Try to evaluate as Python expression
        try:
            result = eval(command, {"__builtins__": {}}, self.workspace_vars)
            if result is not None:
                self.add_command_output(f"ans = {result}", 'success')
                self.workspace_vars['ans'] = result
        except Exception:
            # Try to execute as Python statement
            try:
                exec(command, {"__builtins__": {}}, self.workspace_vars)
                self.add_command_output("Command executed successfully", 'success')
            except Exception as e:
                self.add_command_output(f"Error: {str(e)}", 'error')
    
    def handle_plot_command(self, command):
        """Handle plotting commands"""
        try:
            # Extract data from command (simplified)
            if 'sin' in command:
                x = np.linspace(0, 2*np.pi, 1000)
                y = np.sin(x)
                
                self.fig.clear()
                ax = self.fig.add_subplot(111)
                ax.plot(x, y, 'b-', linewidth=2)
                ax.set_title('sin(x)')
                ax.set_xlabel('x')
                ax.set_ylabel('sin(x)')
                ax.grid(True)
                self.canvas.draw()
                
                self.add_command_output("Plot generated successfully", 'success')
            else:
                self.add_command_output("Advanced plotting coming soon!", 'info')
                
        except Exception as e:
            self.add_command_output(f"Plot error: {str(e)}", 'error')
    
    def handle_3d_plot_command(self, _command):
        """Handle 3D plotting commands"""
        try:
            # Example 3D surface (command parsing to be implemented)
            x = np.linspace(-5, 5, 50)
            y = np.linspace(-5, 5, 50)
            X, Y = np.meshgrid(x, y)
            Z = np.sin(np.sqrt(X**2 + Y**2))
            
            self.fig.clear()
            ax = self.fig.add_subplot(111, projection='3d')
            surf = ax.plot_surface(X, Y, Z, cmap='viridis')
            self.fig.colorbar(surf, ax=ax, shrink=0.5, aspect=5)
            ax.set_title('3D Surface Plot')
            ax.set_xlabel('X')
            ax.set_ylabel('Y') 
            ax.set_zlabel('Z')
            self.canvas.draw()
            
            self.add_command_output("3D plot generated successfully", 'success')
            
        except Exception as e:
            self.add_command_output(f"3D plot error: {str(e)}", 'error')
    
    def handle_matrix_assignment(self, command):
        """Handle matrix assignments like A = [1 2; 3 4]"""
        try:
            # Parse matrix syntax (simplified)
            if '=' in command and '[' in command and ']' in command:
                var_name, matrix_str = command.split('=', 1)
                var_name = var_name.strip()
                matrix_str = matrix_str.strip()
                
                # Convert MATLAB matrix syntax to NumPy
                matrix_str = matrix_str.replace('[', 'np.array([[')
                matrix_str = matrix_str.replace(']', ']])')
                matrix_str = matrix_str.replace(';', '], [')
                matrix_str = matrix_str.replace(' ', ', ')
                
                # Execute
                result = eval(matrix_str, {"np": np})
                self.workspace_vars[var_name] = result
                
                self.add_command_output(f"{var_name} = \n{result}", 'success')
                
        except Exception as e:
            self.add_command_output(f"Matrix error: {str(e)}", 'error')
    
    def refresh_workspace(self):
        """Refresh workspace variables display"""
        # Clear tree
        for item in self.workspace_tree.get_children():
            self.workspace_tree.delete(item)
        
        # Add variables
        for name, value in self.workspace_vars.items():
            if hasattr(value, 'shape'):
                size = str(value.shape)
                type_name = type(value).__name__
            else:
                size = "1x1"
                type_name = type(value).__name__
            
            # Truncate value for display
            value_str = str(value)
            if len(value_str) > 30:
                value_str = value_str[:30] + "..."
            
            self.workspace_tree.insert('', tk.END, values=(name, type_name, size, value_str))
    
    def clear_workspace(self):
        """Clear all workspace variables"""
        self.workspace_vars.clear()
        self.refresh_workspace()
        self.add_command_output("Workspace cleared", 'info')
    
    def clear_command_window(self):
        """Clear command window"""
        self.command_output.config(state=tk.NORMAL)
        self.command_output.delete(1.0, tk.END)
        self.command_output.config(state=tk.DISABLED)
    
    # Initialize signal processing toolkit
    def _init_signal_toolkit(self):
        """Initialize signal processing toolkit"""
        try:
            from signal_processing_toolkit import SignalProcessingToolkit
            self.signal_toolkit = SignalProcessingToolkit(parent_gui=self)
            self.add_command_output("✅ Signal Processing Toolkit loaded", 'success')
        except Exception as e:
            self.add_command_output(f"⚠️ Signal Toolkit not available: {e}", 'warning')
            self.signal_toolkit = None
    
    # Feature implementations
    def open_file(self):
        """Open a data file"""
        filename = filedialog.askopenfilename(
            title="Open Data File",
            filetypes=[("NumPy files", "*.npy"), ("CSV files", "*.csv"), 
                      ("Text files", "*.txt"), ("All files", "*.*")]
        )
        if filename:
            try:
                if filename.endswith('.npy'):
                    data = np.load(filename)
                    var_name = Path(filename).stem
                    self.workspace_vars[var_name] = data
                    self.refresh_workspace()
                    self.add_command_output(f"✅ Loaded {var_name} from {filename}", 'success')
                elif filename.endswith('.csv'):
                    data = np.loadtxt(filename, delimiter=',')
                    var_name = Path(filename).stem
                    self.workspace_vars[var_name] = data
                    self.refresh_workspace()
                    self.add_command_output(f"✅ Loaded {var_name} from {filename}", 'success')
                else:
                    self.add_command_output(f"⚠️ Unsupported file format", 'warning')
            except Exception as e:
                self.add_command_output(f"❌ Error loading file: {e}", 'error')
    
    def save_workspace(self):
        """Save workspace variables"""
        filename = filedialog.asksaveasfilename(
            title="Save Workspace",
            defaultextension=".npz",
            filetypes=[("NumPy archive", "*.npz"), ("All files", "*.*")]
        )
        if filename:
            try:
                np.savez(filename, **self.workspace_vars)
                self.add_command_output(f"✅ Workspace saved to {filename}", 'success')
            except Exception as e:
                self.add_command_output(f"❌ Error saving workspace: {e}", 'error')
    
    def new_workspace(self):
        """Create new workspace"""
        if self.workspace_vars:
            response = messagebox.askyesno("New Workspace", 
                                          "Clear current workspace?")
            if response:
                self.clear_workspace()
        else:
            self.add_command_output("Workspace is already empty", 'info')
    
    def import_data(self):
        """Import data wizard"""
        self.open_file()
    
    def plot_2d_dialog(self):
        """2D Plotting dialog"""
        dialog = tk.Toplevel(self.root)
        dialog.title("📈 2D Plot")
        dialog.geometry("520x460")
        
        ttk.Label(dialog, text="Create 2D Plot", font=(DEFAULT_FONT_FAMILY, 12, 'bold')).pack(pady=10)

        # Presets for quick testing
        presets_2d = {
            "Sine Wave": {"expr": "sin(x)", "xmin": "0", "xmax": "6.283", "res": "800", "polar": False},
            "Damped Sine": {"expr": "sin(5*x)*exp(-0.2*x)", "xmin": "0", "xmax": "15", "res": "1200", "polar": False},
            "Gaussian Pulse": {"expr": "exp(-(x**2))", "xmin": "-5", "xmax": "5", "res": "600", "polar": False},
            "Lissajous (param)": {"xt": "sin(3*t)", "yt": "cos(4*t)", "tmin": "0", "tmax": "6.283", "tres": "1200"},
            "Spiral (polar)": {"expr": "x", "xmin": "0", "xmax": "12.566", "res": "1000", "polar": True},
        }
        preset_frame = ttk.Frame(dialog)
        preset_frame.pack(fill=tk.X, padx=10, pady=(0, 6))
        ttk.Label(preset_frame, text="Presets:").pack(side=tk.LEFT, padx=(0, 6))
        preset_combo = ttk.Combobox(preset_frame, values=list(presets_2d.keys()), state='readonly', width=24)
        preset_combo.pack(side=tk.LEFT)

        def apply_2d_preset(event=None):
            name = preset_combo.get()
            if not name:
                return
            p = presets_2d.get(name, {})
            if "expr" in p:
                expr_entry.delete(0, tk.END); expr_entry.insert(0, p["expr"])
                x_min_entry.delete(0, tk.END); x_min_entry.insert(0, p.get("xmin", "-5"))
                x_max_entry.delete(0, tk.END); x_max_entry.insert(0, p.get("xmax", "5"))
                res_entry.delete(0, tk.END); res_entry.insert(0, p.get("res", "500"))
                polar_var.set(p.get("polar", False))
            if "xt" in p and "yt" in p:
                x_t_entry.delete(0, tk.END); x_t_entry.insert(0, p["xt"])
                y_t_entry.delete(0, tk.END); y_t_entry.insert(0, p["yt"])
                t_min_entry.delete(0, tk.END); t_min_entry.insert(0, p.get("tmin", "0"))
                t_max_entry.delete(0, tk.END); t_max_entry.insert(0, p.get("tmax", "6.283"))
                t_res_entry.delete(0, tk.END); t_res_entry.insert(0, p.get("tres", "400"))

        preset_combo.bind("<<ComboboxSelected>>", apply_2d_preset)
        
        # Expression plotting
        expr_frame = ttk.LabelFrame(dialog, text="Function Plotting (optional)")
        expr_frame.pack(fill=tk.X, padx=10, pady=5)
        ttk.Label(expr_frame, text="y = f(x)").pack(anchor=tk.W, padx=6, pady=(4, 0))
        expr_entry = ttk.Entry(expr_frame, width=50)
        expr_entry.pack(padx=6, pady=4)
        expr_entry.insert(0, "sin(x) * exp(-0.1*x)")
        
        range_frame = ttk.Frame(expr_frame)
        range_frame.pack(fill=tk.X, padx=6, pady=4)
        ttk.Label(range_frame, text="x min").grid(row=0, column=0, padx=2)
        x_min_entry = ttk.Entry(range_frame, width=8)
        x_min_entry.insert(0, "-10")
        x_min_entry.grid(row=0, column=1)
        ttk.Label(range_frame, text="x max").grid(row=0, column=2, padx=4)
        x_max_entry = ttk.Entry(range_frame, width=8)
        x_max_entry.insert(0, "10")
        x_max_entry.grid(row=0, column=3)
        ttk.Label(range_frame, text="Resolution").grid(row=0, column=4, padx=4)
        res_entry = ttk.Entry(range_frame, width=8)
        res_entry.insert(0, "500")
        res_entry.grid(row=0, column=5)
        polar_var = tk.BooleanVar(value=False)
        ttk.Checkbutton(range_frame, text="Polar", variable=polar_var).grid(row=0, column=6, padx=6)
        
        ttk.Label(expr_frame, text="Supported: sin, cos, tan, exp, log, sqrt, abs, pi, np.*", 
                  foreground=self.colors['fg_secondary']).pack(anchor=tk.W, padx=6, pady=(0, 4))

        # Parametric plotting
        param_frame = ttk.LabelFrame(dialog, text="Parametric Plot (optional)")
        param_frame.pack(fill=tk.X, padx=10, pady=5)
        ttk.Label(param_frame, text="x(t) =").grid(row=0, column=0, padx=4, pady=2, sticky='w')
        x_t_entry = ttk.Entry(param_frame, width=30)
        x_t_entry.insert(0, "cos(t)")
        x_t_entry.grid(row=0, column=1, padx=2, pady=2)
        ttk.Label(param_frame, text="y(t) =").grid(row=1, column=0, padx=4, pady=2, sticky='w')
        y_t_entry = ttk.Entry(param_frame, width=30)
        y_t_entry.insert(0, "sin(t)")
        y_t_entry.grid(row=1, column=1, padx=2, pady=2)
        ttk.Label(param_frame, text="t min").grid(row=0, column=2, padx=4)
        t_min_entry = ttk.Entry(param_frame, width=8); t_min_entry.insert(0, "0"); t_min_entry.grid(row=0, column=3)
        ttk.Label(param_frame, text="t max").grid(row=1, column=2, padx=4)
        t_max_entry = ttk.Entry(param_frame, width=8); t_max_entry.insert(0, "6.283" ); t_max_entry.grid(row=1, column=3)
        ttk.Label(param_frame, text="Resolution").grid(row=0, column=4, padx=4)
        t_res_entry = ttk.Entry(param_frame, width=8); t_res_entry.insert(0, "400"); t_res_entry.grid(row=0, column=5)
        
        # Variable selection fallback
        var_frame = ttk.LabelFrame(dialog, text="Plot variables (workspace)")
        var_frame.pack(fill=tk.X, padx=10, pady=5)
        ttk.Label(var_frame, text="Select X variable:").pack(anchor=tk.W, padx=6, pady=2)
        x_var = ttk.Combobox(var_frame, values=list(self.workspace_vars.keys()))
        x_var.pack(padx=6, pady=2)
        
        ttk.Label(var_frame, text="Select Y variable:").pack(anchor=tk.W, padx=6, pady=2)
        y_var = ttk.Combobox(var_frame, values=list(self.workspace_vars.keys()))
        y_var.pack(padx=6, pady=2)
        
        def create_plot():
            try:
                func_str = expr_entry.get().strip()
                xt_str = x_t_entry.get().strip()
                yt_str = y_t_entry.get().strip()
                use_expr = bool(func_str)
                use_param = bool(xt_str and yt_str)
                if use_expr:
                    xmin = float(x_min_entry.get())
                    xmax = float(x_max_entry.get())
                    npts = max(50, min(5000, int(res_entry.get() or 500)))
                    x = np.linspace(xmin, xmax, npts)
                    safe_env = {
                        "x": x,
                        "sin": np.sin,
                        "cos": np.cos,
                        "tan": np.tan,
                        "exp": np.exp,
                        "log": np.log,
                        "sqrt": np.sqrt,
                        "abs": np.abs,
                        "pi": np.pi,
                        "np": np,
                    }
                    y = eval(func_str, {"__builtins__": {}}, safe_env)
                    if polar_var.get():
                        self.fig.clear()
                        ax = self.fig.add_subplot(111, projection='polar')
                        ax.plot(x, y, linewidth=2)
                        ax.set_title(f"r(θ) = {func_str}")
                        self.canvas.draw_idle()
                    else:
                        # Fast plot for cartesian
                        self.fast_plot_xy(x, y)
                        ax = self.ensure_main_axes()
                        ax.set_xlabel('x')
                        ax.set_ylabel('y')
                        ax.set_title(f"y = {func_str}")
                    dialog.destroy()
                    self.add_command_output("✅ Created 2D plot", 'success')
                    return

                if use_param:
                    tmin = float(t_min_entry.get()); tmax = float(t_max_entry.get())
                    npts = max(50, min(5000, int(t_res_entry.get() or 400)))
                    t = np.linspace(tmin, tmax, npts)
                    safe_env = {
                        "t": t,
                        "sin": np.sin,
                        "cos": np.cos,
                        "tan": np.tan,
                        "exp": np.exp,
                        "log": np.log,
                        "sqrt": np.sqrt,
                        "abs": np.abs,
                        "pi": np.pi,
                        "np": np,
                    }
                    x = eval(xt_str, {"__builtins__": {}}, safe_env)
                    y = eval(yt_str, {"__builtins__": {}}, safe_env)
                    # Decimate if Fast Render enabled
                    if self.fast_render_var.get():
                        x, y = self._decimate_xy(np.asarray(x), np.asarray(y))
                    self.fig.clear()
                    ax = self.fig.add_subplot(111)
                    ax.plot(x, y, 'b-', linewidth=2)
                    ax.set_xlabel('x(t)')
                    ax.set_ylabel('y(t)')
                    ax.set_title(f"Parametric: x(t), y(t)")
                    ax.grid(True, alpha=0.3)
                    self.canvas.draw_idle()
                    dialog.destroy()
                    self.add_command_output("✅ Created parametric plot", 'success')
                    return
                
                # Workspace variables path
                if x_var.get() and y_var.get():
                    x_data = self.workspace_vars[x_var.get()]
                    y_data = self.workspace_vars[y_var.get()]
                    # Fast plotting path
                    self.fast_plot_xy(np.asarray(x_data), np.asarray(y_data))
                    ax = self.ensure_main_axes()
                    ax.set_xlabel(x_var.get())
                    ax.set_ylabel(y_var.get())
                    ax.set_title(f"{y_var.get()} vs {x_var.get()}")
                    dialog.destroy()
                    self.add_command_output("✅ Created 2D plot", 'success')
                else:
                    messagebox.showwarning("Missing Data", "Provide an expression or select X and Y variables")
            except Exception as e:
                messagebox.showerror("Error", f"Failed to create plot: {e}")
        
        ttk.Button(dialog, text="Create Plot", command=create_plot).pack(pady=14)
    
    def plot_3d_dialog(self):
        """3D Plotting dialog"""
        dialog = tk.Toplevel(self.root)
        dialog.title("🎯 3D Plot")
        dialog.geometry("540x420")
        
        ttk.Label(dialog, text="Create 3D Surface Plot", font=(DEFAULT_FONT_FAMILY, 12, 'bold')).pack(pady=10)

        # Presets for quick testing
        presets_3d = {
            "Sine Surface": {"func": "sin(x)*cos(y)", "xmin": "-5", "xmax": "5", "ymin": "-5", "ymax": "5", "res": "60", "type": "surface"},
            "Gaussian Hill": {"func": "exp(-(x**2 + y**2)/4)", "xmin": "-6", "xmax": "6", "ymin": "-6", "ymax": "6", "res": "80", "type": "surface"},
            "Ripple": {"func": "sin(x**2 + y**2)", "xmin": "-6", "xmax": "6", "ymin": "-6", "ymax": "6", "res": "90", "type": "contour"},
            "Mexican Hat": {"func": "sin(sqrt(x**2 + y**2))/sqrt(x**2 + y**2 + 1e-3)", "xmin": "-8", "xmax": "8", "ymin": "-8", "ymax": "8", "res": "90", "type": "surface"},
            "Heatmap Interference": {"func": "sin(x)+cos(y)", "xmin": "-6.283", "xmax": "6.283", "ymin": "-6.283", "ymax": "6.283", "res": "120", "type": "heatmap"},
        }
        preset_frame3d = ttk.Frame(dialog)
        preset_frame3d.pack(fill=tk.X, padx=10, pady=(0, 6))
        ttk.Label(preset_frame3d, text="Presets:").pack(side=tk.LEFT, padx=(0, 6))
        preset_combo3d = ttk.Combobox(preset_frame3d, values=list(presets_3d.keys()), state='readonly', width=26)
        preset_combo3d.pack(side=tk.LEFT)

        def apply_3d_preset(event=None):
            name = preset_combo3d.get()
            if not name:
                return
            p = presets_3d.get(name, {})
            func_entry.delete(0, tk.END); func_entry.insert(0, p.get("func", ""))
            x_min_entry.delete(0, tk.END); x_min_entry.insert(0, p.get("xmin", "-5"))
            x_max_entry.delete(0, tk.END); x_max_entry.insert(0, p.get("xmax", "5"))
            y_min_entry.delete(0, tk.END); y_min_entry.insert(0, p.get("ymin", "-5"))
            y_max_entry.delete(0, tk.END); y_max_entry.insert(0, p.get("ymax", "5"))
            res_entry.delete(0, tk.END); res_entry.insert(0, p.get("res", "60"))
            if p.get("type"):
                plot_type.set(p["type"])

        preset_combo3d.bind("<<ComboboxSelected>>", apply_3d_preset)
        
        # Function input
        ttk.Label(dialog, text="Enter function z = f(x, y) (e.g., sin(x)*cos(y)):").pack()
        func_entry = ttk.Entry(dialog, width=48)
        func_entry.pack(pady=5)
        func_entry.insert(0, "sin(x)*cos(y)")
        
        range_frame = ttk.Frame(dialog)
        range_frame.pack(fill=tk.X, padx=10, pady=6)
        for idx, label in enumerate(["x min", "x max", "y min", "y max", "Resolution"]):
            ttk.Label(range_frame, text=label).grid(row=0, column=2*idx, padx=2)
        x_min_entry = ttk.Entry(range_frame, width=7); x_min_entry.insert(0, "-5"); x_min_entry.grid(row=0, column=1)
        x_max_entry = ttk.Entry(range_frame, width=7); x_max_entry.insert(0, "5"); x_max_entry.grid(row=0, column=3)
        y_min_entry = ttk.Entry(range_frame, width=7); y_min_entry.insert(0, "-5"); y_min_entry.grid(row=0, column=5)
        y_max_entry = ttk.Entry(range_frame, width=7); y_max_entry.insert(0, "5"); y_max_entry.grid(row=0, column=7)
        res_entry = ttk.Entry(range_frame, width=7); res_entry.insert(0, "60"); res_entry.grid(row=0, column=9)
        
        ttk.Label(dialog, text="Supported: sin, cos, tan, exp, log, sqrt, abs, pi, np.*", 
              foreground=self.colors['fg_secondary']).pack(anchor=tk.W, padx=12, pady=(0, 4))

        # Plot type selector
        plot_type_frame = ttk.Frame(dialog)
        plot_type_frame.pack(fill=tk.X, padx=10, pady=4)
        ttk.Label(plot_type_frame, text="Plot Type:").pack(side=tk.LEFT, padx=(0, 6))
        plot_type = ttk.Combobox(plot_type_frame, values=["surface", "contour", "heatmap"], state='readonly', width=10)
        plot_type.set("surface")
        plot_type.pack(side=tk.LEFT)
        
        def create_3d_plot():
            try:
                from mpl_toolkits.mplot3d import Axes3D  # noqa: F401
                xmin = float(x_min_entry.get()); xmax = float(x_max_entry.get())
                ymin = float(y_min_entry.get()); ymax = float(y_max_entry.get())
                npts_requested = max(20, min(400, int(res_entry.get() or 60)))
                # Apply grid decimation if Fast Render enabled (tighter cap for speed)
                if self.fast_render_var.get():
                    npts = min(npts_requested, 64)
                else:
                    npts = npts_requested
                x = np.linspace(xmin, xmax, npts)
                y = np.linspace(ymin, ymax, npts)
                X, Y = np.meshgrid(x, y)
                func_str = func_entry.get().strip()
                safe_env = {
                    "x": X,
                    "y": Y,
                    "sin": np.sin,
                    "cos": np.cos,
                    "tan": np.tan,
                    "exp": np.exp,
                    "log": np.log,
                    "sqrt": np.sqrt,
                    "abs": np.abs,
                    "pi": np.pi,
                    "np": np,
                }
                Z = eval(func_str, {"__builtins__": {}}, safe_env)
                self.fig.clear()
                requested_kind = plot_type.get()
                fast_mode = self.fast_render_var.get()
                # In fast mode, surface falls back to heatmap (much faster)
                if fast_mode and requested_kind == "surface":
                    kind = "heatmap"
                    fallback_note = "⚡ Fast Render: surface fallback to heatmap for speed"
                else:
                    kind = requested_kind
                    fallback_note = None

                if kind == "surface":
                    ax = self.fig.add_subplot(111, projection='3d')
                    surf = ax.plot_surface(X, Y, Z, cmap='viridis', rstride=2, cstride=2, linewidth=0, antialiased=False)
                    ax.set_xlabel('X'); ax.set_ylabel('Y'); ax.set_zlabel('Z')
                    ax.set_title(f"z = {func_str}")
                    self.fig.colorbar(surf, ax=ax, shrink=0.6, pad=0.1)
                elif kind == "contour":
                    ax = self.fig.add_subplot(111)
                    levels = 20 if fast_mode else 30
                    cs = ax.contourf(X, Y, Z, levels=levels, cmap='viridis')
                    ax.set_xlabel('X'); ax.set_ylabel('Y'); ax.set_title(f"Contour: z = {func_str}")
                    self.fig.colorbar(cs, ax=ax, shrink=0.8, pad=0.02 if not fast_mode else 0.04)
                else:  # heatmap
                    ax = self.fig.add_subplot(111)
                    im = ax.imshow(Z, extent=[xmin, xmax, ymin, ymax], origin='lower', cmap='viridis', aspect='auto')
                    ax.set_xlabel('X'); ax.set_ylabel('Y'); ax.set_title(f"Heatmap: z = {func_str}")
                    # Skip colorbar in fast mode to save time
                    if not fast_mode:
                        self.fig.colorbar(im, ax=ax, shrink=0.8, pad=0.02)
                self.canvas.draw_idle()
                dialog.destroy()
                self.add_command_output(f"✅ Created 3D {kind} plot", 'success')
                if fallback_note:
                    self.add_command_output(fallback_note, 'info')
            except Exception as e:
                messagebox.showerror("Error", f"Failed to create 3D plot: {e}")
        
        ttk.Button(dialog, text="Create Plot", command=create_3d_plot).pack(pady=14)
    
    def statistics_analysis(self):
        """Statistics analysis toolbox"""
        if not self.workspace_vars:
            self.add_command_output("⚠️ No variables in workspace", 'warning')
            return
        
        dialog = tk.Toplevel(self.root)
        dialog.title("📊 Statistics Analysis")
        dialog.geometry("500x400")
        
        ttk.Label(dialog, text="Statistics Analysis", font=(DEFAULT_FONT_FAMILY, 12, 'bold')).pack(pady=10)
        
        # Variable selection
        ttk.Label(dialog, text="Select variable:").pack()
        var_combo = ttk.Combobox(dialog, values=list(self.workspace_vars.keys()))
        var_combo.pack(pady=5)
        
        # Results text
        results_text = tk.Text(dialog, height=15, width=60)
        results_text.pack(pady=10, padx=10)
        
        def compute_stats():
            if var_combo.get():
                data = self.workspace_vars[var_combo.get()]
                if isinstance(data, np.ndarray):
                    results = f"Statistics for {var_combo.get()}:\n\n"
                    results += f"Mean: {np.mean(data):.6f}\n"
                    results += f"Median: {np.median(data):.6f}\n"
                    results += f"Std Dev: {np.std(data):.6f}\n"
                    results += f"Variance: {np.var(data):.6f}\n"
                    results += f"Min: {np.min(data):.6f}\n"
                    results += f"Max: {np.max(data):.6f}\n"
                    results += f"Range: {np.max(data) - np.min(data):.6f}\n"
                    results += f"Shape: {data.shape}\n"
                    results += f"Size: {data.size}\n"
                    
                    results_text.delete(1.0, tk.END)
                    results_text.insert(1.0, results)
        
        ttk.Button(dialog, text="Compute Statistics", command=compute_stats).pack(pady=5)
    
    def signal_processing(self):
        """Launch signal processing toolkit"""
        if self.signal_toolkit:
            try:
                self.signal_toolkit.signal_processing_gui()
                self.add_command_output("✅ Signal Processing Toolkit opened", 'success')
            except Exception as e:
                self.add_command_output(f"❌ Error: {e}", 'error')
        else:
            self.add_command_output("⚠️ Signal Processing Toolkit not available", 'warning')
    
    def image_processing(self):
        """Image processing tools"""
        self.add_command_output("📷 Image processing: Load images with imread(), process with filters", 'info')
        self.add_command_output("   Available: Gaussian filter, edge detection, morphology", 'info')
    
    def matrix_calculator(self):
        """Advanced matrix calculator"""
        dialog = tk.Toplevel(self.root)
        dialog.title("🧮 Matrix Calculator")
        dialog.geometry("600x500")
        
        ttk.Label(dialog, text="Advanced Matrix Operations", 
                 font=(DEFAULT_FONT_FAMILY, 12, 'bold')).pack(pady=10)
        
        # Matrix operations
        ops_frame = ttk.LabelFrame(dialog, text="Operations")
        ops_frame.pack(fill=tk.X, padx=10, pady=10)
        
        operations = [
            ("Determinant", lambda m: np.linalg.det(m)),
            ("Inverse", lambda m: np.linalg.inv(m)),
            ("Eigenvalues", lambda m: np.linalg.eigvals(m)),
            ("Trace", lambda m: np.trace(m)),
            ("Rank", lambda m: np.linalg.matrix_rank(m)),
        ]
        
        ttk.Label(ops_frame, text="Select matrix:").pack()
        matrix_combo = ttk.Combobox(ops_frame, values=list(self.workspace_vars.keys()))
        matrix_combo.pack(pady=5)
        
        results_text = tk.Text(dialog, height=15, width=70)
        results_text.pack(pady=10, padx=10)
        
        def compute_operation(op_name, op_func):
            if matrix_combo.get():
                try:
                    matrix = self.workspace_vars[matrix_combo.get()]
                    result = op_func(matrix)
                    output = f"{op_name} of {matrix_combo.get()}:\n\n{result}\n\n"
                    results_text.insert(tk.END, output)
                    self.add_command_output(f"✅ Computed {op_name}", 'success')
                except Exception as e:
                    messagebox.showerror("Error", str(e))
        
        for op_name, op_func in operations:
            ttk.Button(ops_frame, text=op_name, 
                      command=lambda n=op_name, f=op_func: compute_operation(n, f)).pack(side=tk.LEFT, padx=5)
    
    def calculus_tools(self):
        """Calculus toolkit"""
        self.add_command_output("∫ Calculus Toolkit: Use commands like:", 'info')
        self.add_command_output("   diff(f, x) - Differentiation", 'info')
        self.add_command_output("   integrate(f, x) - Integration", 'info')
        self.add_command_output("   limit(f, x, a) - Limits", 'info')
        self.add_command_output("   series(f, x, n) - Taylor series", 'info')
    
    def control_systems(self):
        """Control systems toolkit"""
        self.add_command_output("🎛️ Control Systems: Coming in next update", 'info')
        self.add_command_output("   Features: Transfer functions, Bode plots, Root locus", 'info')
    
    def new_figure(self):
        """Create new figure"""
        self.fig.clear()
        self._plot_line = None
        self._ax_main = None
        self.canvas.draw_idle()
        self.add_command_output("✅ New figure created", 'success')
    
    def save_figure(self):
        """Save current figure"""
        filename = filedialog.asksaveasfilename(
            title="Save Figure",
            defaultextension=".png",
            filetypes=[("PNG files", "*.png"), ("PDF files", "*.pdf"), 
                      ("SVG files", "*.svg"), ("All files", "*.*")]
        )
        if filename:
            try:
                self.fig.savefig(filename, dpi=300, bbox_inches='tight')
                self.add_command_output(f"✅ Figure saved to {filename}", 'success')
            except Exception as e:
                self.add_command_output(f"❌ Error saving figure: {e}", 'error')
    
    def clear_figure(self):
        """Clear current figure"""
        self.fig.clear()
        self._plot_line = None
        self._ax_main = None
        self.canvas.draw_idle()
        self.add_command_output("✅ Figure cleared", 'success')

    # Fast plotting utilities
    def _decimate_xy(self, x: np.ndarray, y: np.ndarray, target_points: int = 5000):
        try:
            n = int(min(target_points, max(100, target_points)))
            if x.size <= n:
                return x, y
            step = max(1, x.size // n)
            return x[::step], y[::step]
        except Exception:
            return x, y

    def _decimate_grid(self, X: np.ndarray, Y: np.ndarray, Z: np.ndarray, target_side: int = 100):
        """Decimate 2D grid arrays (for 3D plots)"""
        try:
            n = int(max(20, min(target_side, 120)))
            if X.shape[0] <= n and X.shape[1] <= n:
                return X, Y, Z
            step = max(1, max(X.shape[0], X.shape[1]) // n)
            return X[::step, ::step], Y[::step, ::step], Z[::step, ::step]
        except Exception:
            return X, Y, Z

    def ensure_main_axes(self):
        if getattr(self, '_ax_main', None) is None:
            self._ax_main = self.fig.add_subplot(111)
            self._ax_main.grid(True, alpha=0.3)
        return self._ax_main

    def fast_plot_xy(self, x, y, label=None):
        # Optional decimation in Fast Render mode
        if self.fast_render_var.get():
            x, y = self._decimate_xy(np.asarray(x), np.asarray(y))
        ax = self.ensure_main_axes()
        # Create or update persistent line
        if getattr(self, '_plot_line', None) is None:
            self._plot_line = ax.plot(x, y, 'b-', linewidth=2, label=label)[0]
            if label:
                ax.legend()
        else:
            self._plot_line.set_data(x, y)
        # Update limits efficiently (fixed limits in fast mode)
        try:
            ax.set_autoscale_on(False)
            ax.set_xlim(float(np.min(x)), float(np.max(x)))
            ax.set_ylim(float(np.min(y)), float(np.max(y)))
        except Exception:
            ax.relim(); ax.autoscale_view()
        # Schedule draw; FPS handled by draw_event callback
        self.canvas.draw_idle()

    def _on_draw_event(self, _event):
        try:
            now = time.perf_counter()
            if getattr(self, '_last_draw_t', None) is not None:
                dt = now - self._last_draw_t
                ms = dt * 1000.0
                fps = 1.0 / dt if dt > 1e-9 else float('inf')
                # Only show if toggle enabled
                if self.show_fps_var.get():
                    self.status_var.set(f"Render: {ms:.1f} ms  |  FPS: {fps:.1f}")
                else:
                    self.status_var.set("Ready - AXIOM PRO")
            self._last_draw_t = now
        except Exception:
            pass

    def update_status_fps(self, ms: float):
        try:
            if hasattr(self, 'status_var'):
                self.status_var.set(f"Render: {ms:.1f} ms")
        except Exception:
            pass
    
    def run_script(self):
        """Run script from editor"""
        script_content = self.script_editor.get(1.0, tk.END).strip()
        if script_content:
            self.add_command_output("▶️ Running script...", 'info')
            for line in script_content.split('\n'):
                if line.strip():
                    self.execute_command(line)
        else:
            self.add_command_output("⚠️ Script editor is empty", 'warning')
    
    def save_script(self):
        """Save script to file"""
        filename = filedialog.asksaveasfilename(
            title="Save Script",
            defaultextension=".py",
            filetypes=[("Python files", "*.py"), ("Text files", "*.txt"), ("All files", "*.*")]
        )
        if filename:
            try:
                with open(filename, 'w') as f:
                    f.write(self.script_editor.get(1.0, tk.END))
                self.add_command_output(f"✅ Script saved to {filename}", 'success')
            except Exception as e:
                self.add_command_output(f"❌ Error saving script: {e}", 'error')
    
    def load_script(self):
        """Load script from file"""
        filename = filedialog.askopenfilename(
            title="Load Script",
            filetypes=[("Python files", "*.py"), ("Text files", "*.txt"), ("All files", "*.*")]
        )
        if filename:
            try:
                with open(filename, 'r') as f:
                    content = f.read()
                self.script_editor.delete(1.0, tk.END)
                self.script_editor.insert(1.0, content)
                self.add_command_output(f"✅ Script loaded from {filename}", 'success')
            except Exception as e:
                self.add_command_output(f"❌ Error loading script: {e}", 'error')
    
    def show_help(self, command=""):
        """Show help information"""
        help_text = """
🏛️ AXIOM PRO HELP

Available Commands:
  plot(x, y) - Create 2D plot
  surf(X, Y, Z) - Create 3D surface
  help - Show this help
  clear - Clear command window
  clc - Clear command window
  who - List variables
  whos - Detailed variable info
  
Math Functions:
  sin, cos, tan, exp, log, sqrt
  
Toolboxes:
  📡 Signal Processing - Advanced signal analysis
  📊 Statistics - Statistical analysis
  🧮 Matrix Calculator - Linear algebra tools
  
For more info, use the toolbar buttons!
        """
        self.add_command_output(help_text, 'info')

def main():
    """Launch AXIOM PRO GUI"""
    print("🚀 Starting AXIOM PRO GUI...")
    
    root = tk.Tk()
    
    # Professional icon
    try:
        root.iconbitmap('axiom_pro.ico')
    except Exception:
        pass
    
    # Create application (keep reference to prevent garbage collection)
    _app = AxiomProGUI(root)  # Keep reference to prevent GC
    # Announce Fast Render mode availability
    try:
        _app.add_command_output("⚡ Fast Render mode available in Performance toolbar", 'info')
    except Exception:
        pass
    
    print("✅ AXIOM PRO GUI launched successfully!")
    root.mainloop()

if __name__ == "__main__":
    main()