"""Compares each engine's render against the Chromium ground-truth render
for the same sample, pixel by pixel.

Metric: MAE (mean absolute error) over all 4 RGBA channels, in raw 0-255
intensity units. Picked over MSE/NMSE/NMAE because:
- it's directly interpretable ("pixels differ by X/255 on average" --
  ~2-3 is essentially imperceptible, ~20+ is a visibly different image),
- MSE squares differences, which mostly just exaggerates single-pixel
  anti-aliasing/edge mismatches that aren't visually meaningful here,
- NMSE/NMAE normalize by the reference image's own variance/mean, which
  is unstable for a corpus like this one -- several samples are mostly
  transparent single-color icons, where that denominator is near zero and
  the normalized score swings wildly for a visually tiny difference.
"""
import io

import numpy as np
from PIL import Image


def compute_mae(png_bytes_a, png_bytes_b):
    """Mean absolute error across all RGBA channels, 0-255 scale. Raises if
    the two images differ in pixel dimensions (shouldn't happen here --
    every engine renders each sample at the same corpus.py-computed size)."""
    img_a = Image.open(io.BytesIO(png_bytes_a)).convert("RGBA")
    img_b = Image.open(io.BytesIO(png_bytes_b)).convert("RGBA")
    if img_a.size != img_b.size:
        raise ValueError(f"size mismatch: {img_a.size} vs {img_b.size}")
    a = np.asarray(img_a, dtype=np.float64)
    b = np.asarray(img_b, dtype=np.float64)
    return float(np.mean(np.abs(a - b)))
