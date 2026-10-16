"""Bitmap interop: memoryview, NumPy, DLPack, __array_interface__, Pillow, array import."""

import gc

import pytest

import novasvg

np = pytest.importorskip("numpy")

SVG = (
    '<svg xmlns="http://www.w3.org/2000/svg" width="8" height="6">'
    '<rect width="8" height="6" fill="#336699"/>'
    '<rect width="4" height="6" fill="red" fill-opacity="0.5"/></svg>'
)


@pytest.fixture
def bitmap():
    return novasvg.Document.load_from_data(SVG).render_to_bitmap(8, 6)


def test_shape_and_repr(bitmap):
    assert bitmap.shape == (6, 8, 4)
    assert "width=8" in repr(bitmap)
    assert repr(novasvg.Bitmap()) == "Bitmap(null)"


def test_memoryview_is_zero_copy_and_writable(bitmap):
    mv = memoryview(bitmap)
    assert mv.shape == (6, 8, 4) and not mv.readonly
    mv[0, 0, 0] = 77
    assert bitmap.numpy()[0, 0, 0] == 77
    assert bytes(memoryview(bitmap).cast("B")) == bitmap.tobytes(rgba=False)


def test_numpy_view_and_asarray_share_memory(bitmap):
    a = bitmap.numpy()
    assert a.shape == (6, 8, 4) and a.dtype == np.uint8
    assert np.shares_memory(a, np.asarray(bitmap))
    assert np.shares_memory(a, np.from_dlpack(bitmap))
    del bitmap
    gc.collect()
    assert a[5, 7].tolist() == [153, 102, 51, 255]  # BGRA of #336699


def test_numpy_rgba_is_straight_copy(bitmap):
    a = bitmap.numpy(rgba=True)
    assert not np.shares_memory(a, bitmap.numpy())
    assert a[5, 7].tolist() == [51, 102, 153, 255]
    assert a[0, 0, 3] == 255
    np.testing.assert_array_equal(a.reshape(-1), np.frombuffer(bitmap.tobytes(), np.uint8))
    assert bitmap.numpy(copy=True).tolist() == bitmap.numpy().tolist()
    assert not np.shares_memory(bitmap.numpy(copy=True), bitmap.numpy())
    assert bitmap.numpy(dtype=np.float32).dtype == np.float32


def test_null_bitmap_numpy_is_empty():
    assert novasvg.Bitmap().numpy().size == 0
    with pytest.raises(BufferError):
        memoryview(novasvg.Bitmap())
    with pytest.raises(ValueError):
        novasvg.Bitmap().tobytes()


def test_dlpack_device_and_copy(bitmap):
    assert bitmap.__dlpack_device__() == (1, 0)
    a = np.from_dlpack(bitmap)
    assert a.shape == (6, 8, 4)
    if np.lib.NumpyVersion(np.__version__) >= "2.1.0":
        b = np.from_dlpack(bitmap, copy=True)
        assert not np.shares_memory(b, bitmap.numpy())
        np.testing.assert_array_equal(b, bitmap.numpy())


def test_array_interface(bitmap):
    ai = bitmap.__array_interface__
    assert ai["shape"] == (6, 8, 4) and ai["typestr"] == "|u1" and ai["version"] == 3
    with pytest.raises(AttributeError):
        novasvg.Bitmap().__array_interface__


def test_copy_is_deep(bitmap):
    c = bitmap.copy()
    c.numpy()[0, 0, 0] ^= 0xFF
    assert c.numpy()[0, 0, 0] != bitmap.numpy()[0, 0, 0]
    import copy

    assert copy.deepcopy(bitmap).tobytes() == bitmap.tobytes()
    assert copy.copy(bitmap).numpy().ctypes.data != bitmap.numpy().ctypes.data


def test_encoding_unaffected_by_interop(bitmap):
    png = bitmap.to_png()
    back = novasvg.Bitmap.from_array(np.frombuffer(bitmap.tobytes(), np.uint8).reshape(6, 8, 4))
    assert back.to_png() == png


# --- import -----------------------------------------------------------------


@pytest.mark.parametrize("c", [1, 2, 3, 4])
def test_import_channel_counts(c):
    rng = np.random.default_rng(c)
    arr = rng.integers(0, 256, (5, 7, c), dtype=np.uint8)
    bm = novasvg.Bitmap.from_array(arr)
    assert bm.shape == (5, 7, 4)
    out = bm.numpy(rgba=True)
    if c == 1:
        np.testing.assert_array_equal(out[..., :3], np.repeat(arr, 3, axis=2))
        assert (out[..., 3] == 255).all()
    elif c == 3:
        np.testing.assert_array_equal(out[..., :3], arr)
        assert (out[..., 3] == 255).all()
    elif c == 4:
        np.testing.assert_array_equal(out[..., 3], arr[..., 3])
        opaque = arr[..., 3] == 255
        np.testing.assert_array_equal(out[opaque], arr[opaque])


def test_import_2d_gray_and_constructor():
    arr = np.full((3, 4), 200, np.uint8)
    for bm in (novasvg.Bitmap.from_array(arr), novasvg.Bitmap(arr)):
        assert bm.tobytes()[:4] == bytes([200, 200, 200, 255])


def test_import_is_a_copy():
    arr = np.zeros((2, 2, 4), np.uint8)
    bm = novasvg.Bitmap(arr)
    bm.numpy()[:] = 255
    assert arr.sum() == 0


def test_import_strided_and_readonly():
    base = np.arange(6 * 8 * 4, dtype=np.uint8).reshape(6, 8, 4)
    view = base[::2, ::2, :]
    bm = novasvg.Bitmap.from_array(view)
    assert bm.shape == (3, 4, 4)
    ro = base.copy()
    ro.setflags(write=False)
    assert novasvg.Bitmap.from_array(ro).shape == (6, 8, 4)


def test_raw_roundtrip_is_exact(bitmap):
    raw = bitmap.numpy().copy()
    back = novasvg.Bitmap.from_array(raw, raw=True)
    assert back.numpy().tolist() == raw.tolist()
    via_dlpack = novasvg.Bitmap.from_dlpack(np.from_dlpack(bitmap), raw=True)
    assert via_dlpack.tobytes() == bitmap.tobytes()


def test_straight_roundtrip_opaque_exact(bitmap):
    rgba = bitmap.numpy(rgba=True)
    opaque = rgba[:, 4:]
    bm = novasvg.Bitmap.from_array(np.ascontiguousarray(opaque))
    np.testing.assert_array_equal(bm.numpy(rgba=True), opaque)


def test_import_from_plain_buffer_without_numpy_types():
    raw = bytearray(range(24))
    bm = novasvg.Bitmap.from_array(memoryview(raw).cast("B", shape=[2, 4, 3]))
    assert bm.shape == (2, 4, 4)
    assert bm.tobytes()[:4] == bytes([0, 1, 2, 255])


def test_import_errors():
    with pytest.raises(TypeError):
        novasvg.Bitmap.from_array(np.zeros((2, 2, 4), np.float32))
    with pytest.raises(TypeError):
        novasvg.Bitmap.from_array("nope")
    with pytest.raises(ValueError):
        novasvg.Bitmap.from_array(np.zeros((2, 2, 5), np.uint8))
    with pytest.raises(ValueError):
        novasvg.Bitmap.from_array(np.zeros((0, 2, 4), np.uint8))
    with pytest.raises(ValueError):
        novasvg.Bitmap.from_array(np.zeros((2, 2, 3), np.uint8), raw=True)
    with pytest.raises(TypeError):
        novasvg.Bitmap.from_dlpack(b"abc")
    assert novasvg.Bitmap(3, 2).shape == (2, 3, 4)  # the size constructor still works


# --- Pillow -----------------------------------------------------------------


def test_to_pil_has_right_colours(bitmap):
    PIL = pytest.importorskip("PIL.Image")
    pil = bitmap.to_pil()
    assert pil.mode == "RGBA" and pil.size == (8, 6)
    assert pil.getpixel((7, 5)) == (51, 102, 153, 255)
    assert list(pil.tobytes()) == list(bitmap.tobytes())


def test_from_pil_modes():
    PIL = pytest.importorskip("PIL.Image")
    rgba = PIL.new("RGBA", (3, 2), (10, 20, 30, 255))
    assert novasvg.Bitmap.from_pil(rgba).to_pil().getpixel((1, 1)) == (10, 20, 30, 255)
    for mode, color in (("L", 100), ("RGB", (1, 2, 3)), ("LA", (50, 255)), ("1", 1)):
        bm = novasvg.Bitmap.from_pil(PIL.new(mode, (3, 2), color))
        assert bm.shape == (2, 3, 4)
    pal = PIL.new("RGB", (2, 2), (9, 8, 7)).convert("P")
    expected = pal.convert("RGB").getpixel((0, 0)) + (255,)  # whatever the palette maps it to
    assert novasvg.Bitmap.from_pil(pal).to_pil().getpixel((0, 0)) == expected
    with pytest.raises(TypeError):
        novasvg.Bitmap.from_pil(PIL.new("F", (2, 2)))
    with pytest.raises(TypeError):
        novasvg.Bitmap.from_pil("not an image")


def test_bitmap_accepts_a_pillow_image_directly():
    PIL = pytest.importorskip("PIL.Image")  # via __array_interface__
    pil = PIL.new("RGB", (4, 3), (5, 6, 7))
    bm = novasvg.Bitmap(pil)
    assert bm.to_pil().getpixel((3, 2)) == (5, 6, 7, 255)


def test_semi_transparent_pillow_roundtrip(bitmap):
    PIL = pytest.importorskip("PIL.Image")
    pil = bitmap.to_pil()
    again = novasvg.Bitmap.from_pil(pil).to_pil()
    a, b = np.asarray(pil, dtype=int), np.asarray(again, dtype=int)
    assert np.abs(a - b).max() <= 2  # premultiply / unpremultiply rounding


# --- libstb -----------------------------------------------------------------


def test_to_stb_is_straight_rgba_copy(bitmap):
    libstb = pytest.importorskip("libstb")
    img = bitmap.to_stb()
    assert isinstance(img, libstb.Image)
    assert (img.width, img.height, img.channels) == (8, 6, 4)
    assert img.tobytes() == bitmap.tobytes()
    assert img.numpy()[5, 7].tolist() == [51, 102, 153, 255]
    gc.collect()  # independent of any temporary
    assert img.numpy()[5, 7].tolist() == [51, 102, 153, 255]
    with pytest.raises(ValueError):
        novasvg.Bitmap().to_stb()


def test_from_stb_roundtrip_and_channels(bitmap):
    libstb = pytest.importorskip("libstb")
    back = novasvg.Bitmap.from_stb(bitmap.to_stb())
    assert back.shape == bitmap.shape
    a, b = bitmap.numpy(rgba=True).astype(int), back.numpy(rgba=True).astype(int)
    assert np.abs(a - b).max() <= 2
    gray = libstb.Image(np.full((3, 4, 1), 90, np.uint8))
    assert novasvg.Bitmap.from_stb(gray).tobytes()[:4] == bytes([90, 90, 90, 255])
    with pytest.raises(TypeError):
        novasvg.Bitmap.from_stb(np.zeros((2, 2, 4), np.uint8))


def test_stb_writes_what_novasvg_rendered(bitmap, tmp_path):
    libstb = pytest.importorskip("libstb")
    out = tmp_path / "x.png"
    bitmap.to_stb().write(str(out))
    back = libstb.Image.open(str(out))
    assert back.tobytes() == bitmap.tobytes()
