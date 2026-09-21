"""Compares each engine's render against the Chromium ground-truth render
for the same sample, pixel by pixel.

Metric: MSE (mean squared error) over all 4 RGBA channels, in raw 0-255
intensity units squared. Squaring means large per-pixel mismatches (a
missing filter, a wrong fill, a shifted shape) dominate the score much
more than they would under MAE, while the routine few-value anti-aliasing
noise along edges barely registers -- closer to "did this engine get the
image structurally right" than a flat per-pixel average.
"""
import io

import numpy as np
from PIL import Image


def compute_mse(png_bytes_a, png_bytes_b):
    """Mean squared error across all RGBA channels, 0-255 scale (so values
    range 0-65025). Raises if the two images differ in pixel dimensions
    (shouldn't happen here -- every engine renders each sample at the same
    corpus.py-computed size)."""
    img_a = Image.open(io.BytesIO(png_bytes_a)).convert("RGBA")
    img_b = Image.open(io.BytesIO(png_bytes_b)).convert("RGBA")
    if img_a.size != img_b.size:
        raise ValueError(f"size mismatch: {img_a.size} vs {img_b.size}")
    a = np.asarray(img_a, dtype=np.float64)
    b = np.asarray(img_b, dtype=np.float64)
    return float(np.mean((a - b) ** 2))
