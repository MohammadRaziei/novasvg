"""novasvg's Bitmap interop must work, and import cleanly, without NumPy and Pillow."""

import subprocess
import sys
import textwrap


def _run(body):
    code = "import sys\nsys.modules['numpy'] = None\nsys.modules['PIL'] = None\nsys.modules['libstb'] = None\n" + textwrap.dedent(body)
    return subprocess.run([sys.executable, "-c", code], capture_output=True, text=True)


def test_buffers_and_import_work_without_numpy():
    r = _run("""
        import novasvg
        bm = novasvg.Bitmap(2, 2)
        mv = memoryview(bm)
        assert mv.shape == (2, 2, 4)
        assert len(bm.tobytes()) == 16
        raw = bytearray(range(24))
        b2 = novasvg.Bitmap.from_array(memoryview(raw).cast("B", shape=[2, 4, 3]))
        assert b2.shape == (2, 4, 4) and b2.tobytes()[:4] == bytes([0, 1, 2, 255])
        assert novasvg.Bitmap.from_array(mv, raw=True).tobytes() == bm.tobytes()
    """)
    assert r.returncode == 0, r.stderr


def test_numpy_and_pillow_errors_are_clear():
    r = _run("""
        import novasvg
        bm = novasvg.Bitmap(2, 2)
        for access, word in ((lambda: bm.numpy(), "NumPy"), (lambda: bm.to_pil(), "Pillow"), (lambda: bm.to_stb(), "libstb")):
            try:
                access()
            except ImportError as e:
                assert word in str(e), e
            else:
                raise SystemExit("expected ImportError")
    """)
    assert r.returncode == 0, r.stderr
