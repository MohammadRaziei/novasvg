"""Compares each engine's render against the Chromium ground-truth render
for the same sample, pixel by pixel.

Metric: RMSE (root mean squared error) over all 4 RGBA channels, back in
raw 0-255 intensity units (unlike plain MSE, which is in units-squared and
hard to eyeball). Squaring before the square root still means large
per-pixel mismatches (a missing filter, a wrong fill, a shifted shape)
dominate the score much more than they would under plain MAE, while the
routine few-value anti-aliasing noise along edges barely registers --
closer to "did this engine get the image structurally right" than a flat
per-pixel average -- but the final number reads on the same 0-255 scale
as the pixels themselves.
"""
import libstb
import numpy as np


def compute_rmse(png_bytes_a, png_bytes_b):
    """Root mean squared error across all RGBA channels, 0-255 scale.
    Raises if the two images differ in pixel dimensions (shouldn't happen
    here -- every engine renders each sample at the same
    corpus.py-computed size)."""
    img_a = libstb.imread(png_bytes_a, channels=4)  # (height, width, 4) uint8, straight RGBA
    img_b = libstb.imread(png_bytes_b, channels=4)
    if img_a.shape != img_b.shape:
        raise ValueError(f"size mismatch: {img_a.shape[1]}x{img_a.shape[0]} vs {img_b.shape[1]}x{img_b.shape[0]}")
    a = img_a.astype(np.float64)
    b = img_b.astype(np.float64)
    return float(np.sqrt(np.mean((a - b) ** 2)))
