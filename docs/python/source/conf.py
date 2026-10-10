# Configuration file for Sphinx documentation (Python API).

from pathlib import Path
import re
import sys

DOCS_DIR     = Path(__file__).resolve().parent
PROJECT_ROOT = DOCS_DIR.parent.parent.parent
IMAGES_DIR   = PROJECT_ROOT / 'docs' / 'images'

# ── Version ───────────────────────────────────────────────────────────────────
NOVASVG_H = PROJECT_ROOT / 'include' / 'novasvg' / 'novasvg.h'
version = '0.0.0'

if NOVASVG_H.exists():
    text   = NOVASVG_H.read_text()
    _major = re.search(r'#define\s+NOVASVG_VERSION_MAJOR\s+(\d+)', text)
    _minor = re.search(r'#define\s+NOVASVG_VERSION_MINOR\s+(\d+)', text)
    _patch = re.search(r'#define\s+NOVASVG_VERSION_PATCH\s+(\d+)', text)
    major  = int(_major.group(1)) if _major else 0
    minor  = int(_minor.group(1)) if _minor else 0
    patch  = int(_patch.group(1)) if _patch else 0
    version = f'{major}.{minor}.{patch}'

# ── Import novasvg from build/python/install ────────────────────────────────────
sys.path.insert(0, '.')

import novasvg  # noqa: E402

# ── Project info ──────────────────────────────────────────────────────────────
project   = 'NovaSVG Python'
copyright = '2025, Mohammad Raziei'
author    = 'Mohammad Raziei'
release   = version

# ── Extensions ────────────────────────────────────────────────────────────────
extensions = [
    'sphinx.ext.autodoc',
    'sphinx.ext.viewcode',
    'sphinx.ext.autosummary',
]

autodoc_member_order   = 'groupwise'
autodoc_typehints      = 'description'
autosummary_generate   = True

# ── Hide internal novasvg_py module — rewrite to novasvg ─────────────────────────
add_module_names       = False
modindex_common_prefix = ['novasvg.']

def _fix_novasvg_py(app, what, name, obj, options, lines):
    """Rewrite novasvg_py → novasvg in docstring lines."""
    for i, line in enumerate(lines):
        lines[i] = line.replace('novasvg_py.', 'novasvg.').replace('novasvg_py::', 'novasvg::')

def _fix_novasvg_py_sig(app, what, name, obj, options, signature, return_annotation):
    """Rewrite novasvg_py → novasvg in signatures."""
    if signature:
        signature = signature.replace('novasvg_py.', 'novasvg.')
    if return_annotation:
        return_annotation = return_annotation.replace('novasvg_py.', 'novasvg.')
    return signature, return_annotation

def setup(app):
    app.connect('autodoc-process-docstring', _fix_novasvg_py)
    app.connect('autodoc-process-signature', _fix_novasvg_py_sig)
    # Patch __module__ on all exported symbols so sphinx shows 'novasvg' not 'novasvg.novasvg_py'
    import novasvg as _novasvg
    import novasvg.novasvg_py as _novasvg_py
    for _name in dir(_novasvg_py):
        _obj = getattr(_novasvg_py, _name, None)
        if _obj is not None and hasattr(_obj, '__module__') and _obj.__module__ == 'novasvg.novasvg_py':
            try:
                _obj.__module__ = 'novasvg'
            except (AttributeError, TypeError):
                pass

# NovaSVG docstrings use plain Sphinx (:param:) style, so sphinx.ext.napoleon is
# not enabled (its skip-member hook crashes on nanobind members).

# ── HTML output ───────────────────────────────────────────────────────────────
language         = 'en'
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']
html_theme       = 'novasvg'
html_theme_path  = [str(DOCS_DIR / '_themes')]
html_css_files   = []
pygments_style   = 'monokai'

_logo = IMAGES_DIR / 'novasvg-sq.svg'
if _logo.exists():
    html_logo = str(_logo)

html_last_updated_fmt = '%Y-%m-%d %H:%M'
