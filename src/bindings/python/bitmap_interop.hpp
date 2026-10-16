#pragma once

// Bitmap <-> Python array interop: buffer protocol (memoryview), NumPy, DLPack
// (numpy / torch / jax / cupy), NumPy's __array_interface__ (Pillow) and import
// from any uint8 array-like. Nothing here imports NumPy or Pillow unless a
// NumPy object is actually asked for.
//
// Pixel formats. A Bitmap always stores premultiplied ARGB32: one native-endian
// uint32 per pixel, i.e. the bytes B, G, R, A on little-endian machines. That is
// what the renderer draws in, so every *zero-copy* export (memoryview, DLPack,
// __array_interface__, numpy()) shows exactly that memory ("raw"). Everything
// that needs ordinary straight-alpha RGBA (tobytes(), numpy(rgba=True),
// to_pil(), and importing arrays) converts, and therefore copies.
//
// Import always copies, so a Bitmap never views Python memory and nothing has to
// be kept alive for it.

#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/tuple.h>

#include <algorithm>
#include <climits>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <tuple>

#include "novasvg/novasvg.h"

namespace nb = nanobind;

namespace novasvg_py_interop {

using novasvg::Bitmap;

// surface_create() refuses sizes at or above this.
constexpr std::int64_t kMaxSide = (1 << 15) - 1;

// ---- pixel conversion -------------------------------------------------------

inline std::uint32_t pack_premultiplied(std::uint32_t r, std::uint32_t g, std::uint32_t b, std::uint32_t a) {
    if (a == 0) return 0;
    if (a != 255) {  // same truncating formula as the library's own RGBA -> ARGB conversion
        r = r * a / 255;
        g = g * a / 255;
        b = b * a / 255;
    }
    return (a << 24) | (r << 16) | (g << 8) | b;
}

inline void unpack_straight(std::uint32_t px, std::uint8_t out[4]) {
    const std::uint32_t a = px >> 24;
    if (a == 0) {
        out[0] = out[1] = out[2] = out[3] = 0;
        return;
    }
    std::uint32_t r = (px >> 16) & 0xFF, g = (px >> 8) & 0xFF, b = px & 0xFF;
    if (a != 255) {
        r = std::min<std::uint32_t>(255, r * 255 / a);  // clamp: raw data we did not make may be invalid
        g = std::min<std::uint32_t>(255, g * 255 / a);
        b = std::min<std::uint32_t>(255, b * 255 / a);
    }
    out[0] = std::uint8_t(r);
    out[1] = std::uint8_t(g);
    out[2] = std::uint8_t(b);
    out[3] = std::uint8_t(a);
}

// Straight RGBA, tightly packed (width * 4 bytes per row), into `dst`.
inline void to_straight_rgba(const Bitmap& bmp, std::uint8_t* dst) {
    const int w = bmp.width(), h = bmp.height(), stride = bmp.stride();
    for (int y = 0; y < h; ++y) {
        const auto* src = reinterpret_cast<const std::uint32_t*>(bmp.data() + std::size_t(y) * stride);
        std::uint8_t* out = dst + std::size_t(y) * w * 4;
        for (int x = 0; x < w; ++x) unpack_straight(src[x], out + std::size_t(x) * 4);
    }
}

// The memory as it is, tightly packed (a copy; drops any row padding).
inline void to_raw_packed(const Bitmap& bmp, std::uint8_t* dst) {
    const std::size_t row = std::size_t(bmp.width()) * 4;
    for (int y = 0; y < bmp.height(); ++y)
        std::memcpy(dst + std::size_t(y) * row, bmp.data() + std::size_t(y) * bmp.stride(), row);
}

// A new Bitmap from a strided uint8 view of (h, w, c) pixels.
//   raw == false: straight-alpha L / LA / RGB / RGBA (c = 1..4), converted.
//   raw == true : c must be 4 and the bytes are already in the Bitmap's own layout.
inline Bitmap bitmap_from_pixels(const std::uint8_t* p, std::int64_t h, std::int64_t w, std::int64_t c,
                                 std::int64_t s0, std::int64_t s1, std::int64_t s2, bool raw) {
    if (c < 1 || c > 4) throw std::invalid_argument("expected shape (H, W) or (H, W, 1..4)");
    if (h < 1 || w < 1) throw std::invalid_argument("the array is empty");
    if (h > kMaxSide || w > kMaxSide)
        throw std::invalid_argument("image too large: width and height must be below 32768");
    if (raw && c != 4)
        throw std::invalid_argument("raw=True needs shape (H, W, 4): premultiplied BGRA, the Bitmap's own layout");

    Bitmap bmp(static_cast<int>(w), static_cast<int>(h));
    if (bmp.isNull()) throw std::runtime_error("could not allocate the Bitmap");

    auto at = [&](std::int64_t y, std::int64_t x, std::int64_t k) -> std::uint32_t {
        return p[y * s0 + x * s1 + k * s2];
    };
    for (std::int64_t y = 0; y < h; ++y) {
        auto* dst = reinterpret_cast<std::uint32_t*>(bmp.data() + std::size_t(y) * bmp.stride());
        for (std::int64_t x = 0; x < w; ++x) {
            if (raw) {
                std::uint8_t bytes[4] = {std::uint8_t(at(y, x, 0)), std::uint8_t(at(y, x, 1)),
                                         std::uint8_t(at(y, x, 2)), std::uint8_t(at(y, x, 3))};
                std::memcpy(&dst[x], bytes, 4);
                continue;
            }
            switch (c) {
                case 1: { const auto g = at(y, x, 0); dst[x] = pack_premultiplied(g, g, g, 255); break; }
                case 2: { const auto g = at(y, x, 0); dst[x] = pack_premultiplied(g, g, g, at(y, x, 1)); break; }
                case 3: dst[x] = pack_premultiplied(at(y, x, 0), at(y, x, 1), at(y, x, 2), 255); break;
                default: dst[x] = pack_premultiplied(at(y, x, 0), at(y, x, 1), at(y, x, 2), at(y, x, 3)); break;
            }
        }
    }
    return bmp;
}

// ---- import: any uint8 array-like -------------------------------------------

struct py_buffer {  // releases a Py_buffer however the scope is left
    Py_buffer view{};
    bool ok = false;
    ~py_buffer() {
        if (ok) PyBuffer_Release(&view);
    }
};

// An object that only speaks NumPy's __array_interface__ (version 3), e.g. a Pillow
// image. `data` is (address, read-only flag) or an object with the buffer protocol
// (Pillow gives bytes). The pixels are converted into a new Bitmap, so nothing outlives
// the call. As with NumPy itself an address cannot be checked: hand over only objects
// you trust. A buffer is bounds-checked against shape and strides.
inline Bitmap bitmap_from_array_interface(nb::handle obj, bool raw) {
    nb::dict ai = nb::cast<nb::dict>(obj.attr("__array_interface__"));
    if (!ai.contains("typestr") || !ai.contains("shape"))
        throw std::invalid_argument("__array_interface__ needs 'shape' and 'typestr'");
    const std::string typestr = nb::cast<std::string>(ai["typestr"]);
    if (typestr != "|u1" && typestr != "<u1" && typestr != ">u1") throw nb::type_error("expected a uint8 array");

    nb::sequence shape = nb::cast<nb::sequence>(ai["shape"]);
    const std::size_t ndim = nb::len(shape);
    if (ndim != 2 && ndim != 3) throw std::invalid_argument("expected shape (H, W) or (H, W, 1..4)");
    const std::int64_t h = nb::cast<std::int64_t>(shape[0]), w = nb::cast<std::int64_t>(shape[1]);
    const std::int64_t c = ndim == 3 ? nb::cast<std::int64_t>(shape[2]) : 1;
    if (c < 1 || c > 4) throw std::invalid_argument("expected shape (H, W) or (H, W, 1..4)");
    if (h < 1 || w < 1) throw std::invalid_argument("the array is empty");

    std::int64_t s0 = w * c, s1 = c, s2 = 1;
    if (ai.contains("strides") && !ai["strides"].is_none()) {
        nb::sequence st = nb::cast<nb::sequence>(ai["strides"]);
        if (nb::len(st) != ndim) throw std::invalid_argument("__array_interface__: strides do not match shape");
        s0 = nb::cast<std::int64_t>(st[0]);
        s1 = nb::cast<std::int64_t>(st[1]);
        s2 = ndim == 3 ? nb::cast<std::int64_t>(st[2]) : 1;
    }
    const std::int64_t offset = ai.contains("offset") ? nb::cast<std::int64_t>(ai["offset"]) : 0;

    nb::object data = ai.contains("data") ? nb::object(ai["data"]) : nb::object(nb::none());
    if (data.is_none()) data = nb::borrow(obj);  // the spec: the object itself exposes the buffer

    if (nb::isinstance<nb::tuple>(data)) {  // (address, read-only)
        nb::tuple t = nb::cast<nb::tuple>(data);
        if (nb::len(t) != 2) throw std::invalid_argument("__array_interface__: bad 'data' tuple");
        const auto addr = nb::cast<std::uintptr_t>(t[0]);
        if (!addr) throw std::invalid_argument("__array_interface__: null data address");
        return bitmap_from_pixels(reinterpret_cast<const std::uint8_t*>(addr) + offset, h, w, c, s0, s1, s2, raw);
    }

    py_buffer buf;
    if (PyObject_GetBuffer(data.ptr(), &buf.view, PyBUF_SIMPLE) != 0) throw nb::python_error();
    buf.ok = true;
    // Every byte the strides can reach must lie inside the buffer.
    const std::int64_t lo = offset + std::min<std::int64_t>(0, (h - 1) * s0) + std::min<std::int64_t>(0, (w - 1) * s1) +
                            std::min<std::int64_t>(0, (c - 1) * s2);
    const std::int64_t hi = offset + std::max<std::int64_t>(0, (h - 1) * s0) + std::max<std::int64_t>(0, (w - 1) * s1) +
                            std::max<std::int64_t>(0, (c - 1) * s2);
    if (lo < 0 || hi >= std::int64_t(buf.view.len))
        throw std::invalid_argument("__array_interface__: the data is shorter than shape and strides need");
    return bitmap_from_pixels(static_cast<const std::uint8_t*>(buf.view.buf) + offset, h, w, c, s0, s1, s2, raw);
}

using array_ro = nb::ndarray<nb::ro, nb::device::cpu>;

inline Bitmap bitmap_from_object(nb::handle obj, bool raw) {
    array_ro a;
    if (!nb::try_cast(obj, a, /*convert=*/false)) {
        // Not a buffer / DLPack object: maybe one that only has NumPy's array interface (Pillow).
        if (nb::hasattr(obj, "__array_interface__")) return bitmap_from_array_interface(obj, raw);
        throw nb::type_error(
            "expected a uint8 array-like on the CPU: a buffer (memoryview, numpy), a DLPack object "
            "(numpy, torch, jax, cupy) or an object with __array_interface__ (Pillow)");
    }
    if (a.dtype() != nb::dtype<std::uint8_t>()) throw nb::type_error("expected a uint8 array");
    if (a.ndim() != 2 && a.ndim() != 3) throw std::invalid_argument("expected shape (H, W) or (H, W, 1..4)");
    const std::int64_t c = a.ndim() == 3 ? std::int64_t(a.shape(2)) : 1;
    return bitmap_from_pixels(static_cast<const std::uint8_t*>(a.data()), std::int64_t(a.shape(0)),
                              std::int64_t(a.shape(1)), c, a.stride(0), a.stride(1), a.ndim() == 3 ? a.stride(2) : 1,
                              raw);
}

// ---- buffer protocol --------------------------------------------------------

// memoryview(bitmap) is a writable 3-D (height, width, 4) uint8 view of the raw
// pixels, no copy, no numpy. The view holds a reference to the Bitmap, so the pixels
// cannot be freed under it. numpy reads this too, so np.asarray(bitmap) is zero-copy.
struct buffer_meta {
    Py_ssize_t shape[3];
    Py_ssize_t strides[3];
};

inline int bitmap_getbuffer(PyObject* exporter, Py_buffer* view, int flags) {
    if (!view) {
        PyErr_SetString(PyExc_BufferError, "no buffer requested");
        return -1;
    }
    auto* bmp = nb::inst_ptr<Bitmap>(nb::handle(exporter));
    if (bmp->isNull()) {
        PyErr_SetString(PyExc_BufferError, "cannot export a null Bitmap");
        return -1;
    }
    const auto h = Py_ssize_t(bmp->height()), w = Py_ssize_t(bmp->width()), stride = Py_ssize_t(bmp->stride());
    const bool tight = stride == w * 4;
    if (!tight && (flags & PyBUF_STRIDES) != PyBUF_STRIDES) {
        PyErr_SetString(PyExc_BufferError, "this Bitmap has row padding: ask for a strided buffer");
        return -1;
    }

    auto* meta = new buffer_meta{{h, w, 4}, {stride, 4, 1}};
    Py_INCREF(exporter);
    view->obj = exporter;
    view->buf = bmp->data();
    view->len = h * w * 4;
    view->readonly = 0;
    view->itemsize = 1;
    view->format = (flags & PyBUF_FORMAT) ? const_cast<char*>("B") : nullptr;
    view->suboffsets = nullptr;
    view->internal = meta;
    if (flags & PyBUF_ND) {  // asked for a shape: give the real 3-D one
        view->ndim = 3;
        view->shape = meta->shape;
        view->strides = (flags & PyBUF_STRIDES) == PyBUF_STRIDES ? meta->strides : nullptr;
    } else {  // a plain byte buffer, as the protocol requires when no shape is asked for
        view->ndim = 1;
        view->shape = nullptr;
        view->strides = nullptr;
    }
    return 0;
}

inline void bitmap_releasebuffer(PyObject*, Py_buffer* view) { delete static_cast<buffer_meta*>(view->internal); }

inline const PyType_Slot bitmap_slots[] = {
    {Py_bf_getbuffer, reinterpret_cast<void*>(bitmap_getbuffer)},
    {Py_bf_releasebuffer, reinterpret_cast<void*>(bitmap_releasebuffer)},
    {0, nullptr},
};

// ---- export helpers ---------------------------------------------------------

// numpy is an optional extra, imported only when a numpy object is actually asked for.
inline void require_numpy(const char* what) {
    static bool available = false;
    if (available) return;
    try {
        nb::module_::import_("numpy");
        available = true;
    } catch (nb::python_error&) {
        throw nb::import_error((std::string(what) +
                                " needs NumPy, which is not installed (pip install numpy). Everything else in "
                                "novasvg works without it: memoryview(bitmap) and bitmap.tobytes() give you the "
                                "pixels.")
                                   .c_str());
    }
}

using numpy_pixels = nb::ndarray<nb::numpy, std::uint8_t, nb::ndim<3>>;

// The raw pixels as an ndarray sharing the Bitmap's memory; `self` owns it.
inline numpy_pixels raw_pixels_of(nb::handle self) {
    auto& bmp = nb::cast<Bitmap&>(self);
    const std::size_t shape[3] = {std::size_t(bmp.height()), std::size_t(bmp.width()), 4};
    const std::int64_t strides[3] = {bmp.stride(), 4, 1};
    return numpy_pixels(bmp.data(), 3, shape, self, strides);
}

// A new ndarray holding straight RGBA (or the packed raw bytes), owned by itself.
inline numpy_pixels copied_pixels_of(const Bitmap& bmp, bool rgba) {
    const std::size_t shape[3] = {std::size_t(bmp.height()), std::size_t(bmp.width()), 4};
    auto* buf = new std::uint8_t[shape[0] * shape[1] * 4];
    if (rgba) to_straight_rgba(bmp, buf);
    else to_raw_packed(bmp, buf);
    nb::capsule owner(buf, [](void* p) noexcept { delete[] static_cast<std::uint8_t*>(p); });
    return numpy_pixels(buf, 3, shape, owner);
}

inline void require_pixels(const Bitmap& bmp, const char* what) {
    if (bmp.isNull()) throw std::invalid_argument(std::string("cannot use a null Bitmap for ") + what);
}

// An independent Bitmap with its own pixels (Bitmap's own copy constructor shares them).
inline Bitmap deep_copy(const Bitmap& bmp) {
    if (bmp.isNull()) return Bitmap();
    Bitmap out(bmp.width(), bmp.height());
    if (out.isNull()) throw std::runtime_error("could not allocate the Bitmap");
    for (int y = 0; y < bmp.height(); ++y)
        std::memcpy(out.data() + std::size_t(y) * out.stride(), bmp.data() + std::size_t(y) * bmp.stride(),
                    std::size_t(bmp.width()) * 4);
    return out;
}

// ---- the Python methods -----------------------------------------------------

inline void bind_bitmap_interop(nb::class_<Bitmap>& cls) {
    using namespace nb::literals;

    cls.def(
           "__init__", [](Bitmap* self, nb::handle array) { new (self) Bitmap(bitmap_from_object(array, false)); },
           "array"_a,
           "Bitmap(array): a new Bitmap from a uint8 array-like of shape (H, W) or (H, W, 1..4) holding straight\n"
           "(not premultiplied) L, LA, RGB or RGBA pixels. A copy. See Bitmap.from_array for raw=True.")
        .def_static(
            "from_array", [](nb::handle array, bool raw) { return bitmap_from_object(array, raw); }, "array"_a,
            nb::kw_only(), "raw"_a = false,
            "A Bitmap from any uint8 array-like: a buffer (memoryview, numpy), a DLPack object or an object with\n"
            "__array_interface__ (Pillow). Always a copy.\n\n"
            "raw=False (default): shape (H, W) or (H, W, 1..4) of straight-alpha L, LA, RGB or RGBA.\n"
            "raw=True: shape (H, W, 4) already in the Bitmap's own layout (premultiplied, BGRA on little-endian),\n"
            "e.g. what numpy() / memoryview(bitmap) / np.from_dlpack(bitmap) gave you.\n\n"
            "Raises TypeError (not uint8 / not an array) and ValueError (bad shape, too large).")
        .def_static(
            "from_dlpack",
            [](nb::handle obj, bool raw) {
                if (!nb::hasattr(obj, "__dlpack__"))
                    throw nb::type_error(
                        "Bitmap.from_dlpack needs an object with __dlpack__ (a torch/jax/cupy/numpy array, ...)");
                return bitmap_from_object(obj, raw);
            },
            "obj"_a, nb::kw_only(), "raw"_a = false,
            "A Bitmap from any object that implements DLPack: a numpy, torch, jax or cupy array. The mirror of\n"
            "np.from_dlpack(bitmap). The array must be uint8 on the CPU (call .cpu() on a GPU tensor); see\n"
            "from_array for the shapes and raw=. Always a copy.")

        // --- copying is explicit: Bitmap's C++ copy shares the pixels ---
        .def("copy", [](const Bitmap& b) { return deep_copy(b); },
             "An independent Bitmap that owns its own pixels (a deep copy).")
        .def("__copy__", [](const Bitmap& b) { return deep_copy(b); })
        .def("__deepcopy__", [](const Bitmap& b, nb::handle) { return deep_copy(b); }, "memo"_a)

        .def_prop_ro("shape",
                     [](const Bitmap& b) { return std::make_tuple(b.height(), b.width(), b.isNull() ? 0 : 4); },
                     "(height, width, 4), the shape of the arrays this Bitmap exports.")

        // DLPack: np.from_dlpack(bitmap), torch.from_dlpack(bitmap), jax.numpy.from_dlpack(bitmap),
        // cupy.from_dlpack(bitmap) ... zero-copy, and novasvg imports none of them. The pixels go out as
        // nanobind's own DLPack object (no numpy needed) that keeps this Bitmap alive.
        .def(
            "__dlpack__",
            [](nb::handle self, nb::args args, nb::kwargs kwargs) {
                nb::object owner = nb::borrow(self);
                if (kwargs.contains("copy")) {  // copy=True is ours to honour (nanobind refuses it)
                    nb::object want_copy = kwargs["copy"];
                    nb::del(kwargs["copy"]);
                    if (!want_copy.is_none() && nb::cast<bool>(want_copy))
                        owner = nb::cast(deep_copy(nb::cast<Bitmap&>(self)));
                }
                auto& bmp = nb::cast<Bitmap&>(owner);
                require_pixels(bmp, "DLPack");
                const std::size_t shape[3] = {std::size_t(bmp.height()), std::size_t(bmp.width()), 4};
                const std::int64_t strides[3] = {bmp.stride(), 4, 1};
                nb::ndarray<nb::array_api, std::uint8_t, nb::ndim<3>, nb::device::cpu> a(bmp.data(), 3, shape, owner,
                                                                                         strides);
                return nb::cast(a).attr("__dlpack__")(*args, **kwargs);
            },
            "DLPack export of the raw pixels (uint8, shape (height, width, 4), premultiplied BGRA on\n"
            "little-endian, no copy). Use np.from_dlpack(bitmap), torch.from_dlpack(bitmap), ...")
        .def("__dlpack_device__", [](const Bitmap&) { return std::make_tuple(1, 0); },  // kDLCPU, device 0
             "(device type, device id): always the CPU.")

        .def(
            "tobytes",
            [](const Bitmap& b, bool rgba) {
                require_pixels(b, "tobytes()");
                const std::size_t n = std::size_t(b.height()) * std::size_t(b.width()) * 4;
                PyObject* out = PyBytes_FromStringAndSize(nullptr, Py_ssize_t(n));
                if (!out) throw nb::python_error();
                auto* dst = reinterpret_cast<std::uint8_t*>(PyBytes_AsString(out));
                if (rgba) to_straight_rgba(b, dst);
                else to_raw_packed(b, dst);
                return nb::steal<nb::bytes>(out);
            },
            nb::kw_only(), "rgba"_a = true,
            "The pixels as bytes, row-major (height, width, 4), tightly packed. A copy; needs no numpy.\n"
            "rgba=True (default): straight, un-premultiplied R, G, B, A.\n"
            "rgba=False: the raw memory (premultiplied BGRA on little-endian).")

        .def(
            "__array__",
            [](nb::handle self, nb::object dtype, nb::object copy) -> nb::object {
                require_numpy("Bitmap.__array__ (np.asarray)");
                require_pixels(nb::cast<Bitmap&>(self), "NumPy");
                nb::object a = nb::cast(raw_pixels_of(self));
                if (!dtype.is_none()) {
                    nb::module_ np = nb::module_::import_("numpy");
                    if (!np.attr("dtype")(dtype).equal(np.attr("dtype")("uint8")))
                        return a.attr("astype")(dtype);  // always a copy
                }
                if (!copy.is_none() && nb::cast<bool>(copy)) return a.attr("copy")();
                return a;
            },
            "dtype"_a = nb::none(), "copy"_a = nb::none())
        .def_prop_ro(
            "__array_interface__",
            [](nb::handle self) {
                const auto& b = nb::cast<const Bitmap&>(self);
                if (b.isNull()) throw nb::attribute_error("a null Bitmap has no array interface");
                nb::dict d;
                d["version"] = 3;
                d["shape"] = nb::make_tuple(b.height(), b.width(), 4);
                d["typestr"] = "|u1";
                d["data"] = nb::make_tuple(reinterpret_cast<std::uintptr_t>(b.data()), false);
                d["strides"] = b.stride() == b.width() * 4 ? nb::object(nb::none())
                                                            : nb::object(nb::make_tuple(b.stride(), 4, 1));
                return d;
            },
            "NumPy's array interface (version 3) over the raw pixels. Keep the Bitmap alive while using the\n"
            "address. Note these are premultiplied BGRA, not RGBA: for Pillow use bitmap.to_pil().")
        .def(
            "numpy",
            [](nb::handle self, bool rgba, nb::object dtype, bool copy) -> nb::object {
                require_numpy("Bitmap.numpy()");
                auto& bmp = nb::cast<Bitmap&>(self);
                if (bmp.isNull())  // an empty array (the old code returned None here)
                    return nb::module_::import_("numpy").attr("zeros")(nb::make_tuple(0, 0, 4), "uint8");
                nb::object a = rgba ? nb::cast(copied_pixels_of(bmp, true))
                                    : (copy ? nb::cast(copied_pixels_of(bmp, false)) : nb::cast(raw_pixels_of(self)));
                if (!dtype.is_none()) {
                    nb::module_ np = nb::module_::import_("numpy");
                    if (!np.attr("dtype")(dtype).equal(np.attr("dtype")("uint8"))) return a.attr("astype")(dtype);
                }
                return a;
            },
            nb::kw_only(), "rgba"_a = false, "dtype"_a = nb::none(), "copy"_a = false,
            "The pixels as an (height, width, 4) ndarray.\n\n"
            "By default a view of the Bitmap's memory (no copy, writable): premultiplied BGRA on little-endian,\n"
            "until convert_to_rgba() is called. rgba=True returns a copy as ordinary straight RGBA instead\n"
            "(do not combine it with convert_to_rgba(), which already converts the buffer). copy=True copies the\n"
            "raw view. A dtype other than uint8 is always a copy.")
        .def("__repr__", [](const Bitmap& b) {
            if (b.isNull()) return std::string("Bitmap(null)");
            return "Bitmap(width=" + std::to_string(b.width()) + ", height=" + std::to_string(b.height()) + ")";
        });
}

}  // namespace novasvg_py_interop
