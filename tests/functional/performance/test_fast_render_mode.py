# Perf sanity: compare full vs decimated plot draw times (Agg backend)
import time
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt


def measure_draw_time(x, y):
    fig, ax = plt.subplots(figsize=(8, 6), dpi=100)
    t0 = time.perf_counter()
    ax.plot(x, y, 'b-', linewidth=1.5)
    fig.canvas.draw()
    dt = (time.perf_counter() - t0) * 1000.0
    plt.close(fig)
    return dt


def decimate_xy(x, y, target_points=5000):
    n = int(max(100, target_points))
    if x.size <= n:
        return x, y
    step = max(1, x.size // n)
    return x[::step], y[::step]


def main():
    n = 1_000_000
    x = np.linspace(0.0, 100.0, n, dtype=np.float64)
    y = np.sin(x) + 0.1 * np.cos(5 * x)

    full_ms = measure_draw_time(x, y)
    xd, yd = decimate_xy(x, y, target_points=5000)
    fast_ms = measure_draw_time(xd, yd)

    speedup = full_ms / fast_ms if fast_ms > 1e-6 else float('inf')
    print(f"Full draw: {full_ms:.1f} ms\nDecimated draw: {fast_ms:.1f} ms\nSpeedup: {speedup:.1f}x\nPoints: full={x.size}, decimated={xd.size}")


if __name__ == '__main__':
    main()
