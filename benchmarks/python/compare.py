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
import io

import numpy as np
from PIL import Image


def compute_rmse(png_bytes_a, png_bytes_b):
    """Root mean squared error across all RGBA channels, 0-255 scale.
    Raises if the two images differ in pixel dimensions (shouldn't happen
    here -- every engine renders each sample at the same
    corpus.py-computed size)."""
    img_a = Image.open(io.BytesIO(png_bytes_a)).convert("RGBA")
    img_b = Image.open(io.BytesIO(png_bytes_b)).convert("RGBA")
    if img_a.size != img_b.size:
        raise ValueError(f"size mismatch: {img_a.size} vs {img_b.size}")
    a = np.asarray(img_a, dtype=np.float64)
    b = np.asarray(img_b, dtype=np.float64)
    return float(np.sqrt(np.mean((a - b) ** 2)))
