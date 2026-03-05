#!/usr/bin/env python3
"""
Quick plotting performance sanity check (non-interactive)
Measures draw time for large line updates.
"""
import time
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt


def run_perf(n_points=200_000, updates=3):
    x = np.linspace(0, 100, n_points)
    y = np.sin(x) + 0.1*np.random.default_rng(42).normal(0, 1, n_points)
    fig, ax = plt.subplots(figsize=(8, 4))
    line, = ax.plot(x, y, lw=1.0)
    ax.set_xlim(x.min(), x.max())
    ax.set_ylim(y.min(), y.max())

    # Initial draw
    t0 = time.perf_counter()
    fig.canvas.draw()
    t1 = time.perf_counter()

    # Update draws
    update_times = []
    for k in range(updates):
        y2 = np.sin(x + (k+1)*0.5)
        line.set_data(x, y2)
        t2 = time.perf_counter()
        fig.canvas.draw()
        t3 = time.perf_counter()
        update_times.append((t3 - t2) * 1000.0)

    init_ms = (t1 - t0) * 1000.0
    avg_update_ms = sum(update_times)/len(update_times)
    print(f"Init draw: {init_ms:.1f} ms, Avg update: {avg_update_ms:.1f} ms")
    return init_ms, avg_update_ms


if __name__ == '__main__':
    run_perf()
