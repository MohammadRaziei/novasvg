# novasvg — چک‌لیست ریفکتور (compiled + libstb + نامگذاری stl-like)

> این فایل جدا از `checklist.md` قدیمی پروژه است (آن فایل لیست کارهای خودت است و دست نخورده).
> هدف: header-only بودن را کنار بگذاریم، تا جای ممکن از `libstb` استفاده کنیم، نامگذاری را
> stl-like کنیم و طراحی/OOP را مرتب کنیم. این یک تغییر breaking است.

## قاعده‌ی کار

هر مرحله فقط وقتی «تمام» است که **همه‌ی این‌ها** درست باشد:

1. بیلد بدون خطا.
2. تست‌ها هم‌سطح baseline یا بهتر (اعداد پایین).
3. `tools/golden.py diff` روی رندر ۶۲ تصویر، تفاوت صفر (یا تفاوت توجیه‌شده و نوشته‌شده).
4. تیک همین فایل زده شود و توضیح کوتاه زیر مرحله بیاید.
5. zip کامل پروژه ساخته و فرستاده شود. هر مرحله یک zip؛ تو در برنچ جدید push می‌کنی.

هر مرحله را کوچک نگه می‌داریم تا اگر چیزی خراب شد، علتش معلوم باشد.

## نحوه‌ی راستی‌آزمایی

```sh
# بیلد (روی ماشین تک‌هسته‌ای هر TU چند دقیقه طول می‌کشد؛ ninja ادامه‌پذیر است)
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
      -DNOVASVG_BUILD_PYTHON=OFF -DNOVASVG_BUILD_TESTS=ON -DNOVASVG_BUILD_EXAMPLES=OFF
cmake --build build

# تست‌ها
./build/tests/cpp/test_novasvg_cpp
./build/tests/cpp/test_novasvg_embedded_font

# مقایسه‌ی پیکسلی با baseline
python3 tools/golden.py render ./build/novasvg /tmp/golden-new
python3 tools/golden.py diff refactor/golden-baseline /tmp/golden-new
```

`golden.py` PNGها را decode می‌کند، پس عوض شدن انکودر PNG (stb_image_write → libstb) به‌خودیِ‌خود
تفاوت حساب نمی‌شود؛ فقط پیکسل‌ها مقایسه می‌شوند. این ابزار آزموده شده: دو رندر پشت‌سرهم
تفاوت صفر دارند و تغییر یک پیکسل (delta=7) را می‌گیرد.

## Baseline (نسخه‌ی 0.11.4، commit `85ebfc7`)

تصاویر مرجع (golden) در `refactor/golden-baseline/` هستند و با `.gitignore` از گیت بیرونند (در zip می‌آیند). اگر گم شدند، از همان commit دوباره ساخته می‌شوند:

```sh
git stash -u && git checkout 85ebfc7          # یا یک worktree جدا
cmake -S . -B /tmp/b0 -G Ninja -DCMAKE_BUILD_TYPE=Release -DNOVASVG_BUILD_PYTHON=OFF \
      -DNOVASVG_BUILD_TESTS=OFF -DNOVASVG_BUILD_EXAMPLES=OFF && cmake --build /tmp/b0
python3 tools/golden.py render /tmp/b0/novasvg refactor/golden-baseline
```


| مورد | مقدار |
|---|---|
| `test_novasvg_cpp` | 64 تست، 377 assertion، همه موفق |
| `test_novasvg_embedded_font` | 5 تست، 71 assertion، همه موفق |
| golden | 62 تصویر (3٫2 مگابایت) در `refactor/golden-baseline/`؛ پوشه در `.gitignore` است و commit نمی‌شود |
| تست‌های پایتون | هنوز اجرا نشده (بایندینگ پایتون در baseline خاموش بود) |

## تصمیم‌های ثبت‌شده

- **namespace**: `novasvg`. اگر خواستی یک alias اختیاری `namespace nova = novasvg;` می‌گذاریم.
  دلیل: الان 295 بار `novasvg::` و صفر بار `nova::` استفاده شده، و libstb هم نام کامل خودش (`stb::`) را دارد.
- **نامگذاری**: همان قاعده‌ی libstb (پایین).
- **ترتیب مراحل نسبت به پیشنهاد اول عوض شد**: جایگزینی کد vendor شده با libstb *قبل از*
  جابه‌جایی کد به `.cpp` می‌آید، چون حدود 14 هزار خط کد را اول حذف می‌کند و باقی کار سبک‌تر می‌شود.
- `Bitmap` (premultiplied ARGB32، سطح رندر) با `stb::image` (RGBA ساده) یکی نیست. `Bitmap` می‌ماند
  به‌عنوان surface رندر، و مرز تبدیل بین این دو صریح و تست‌شده تعریف می‌شود (مرحله ۲).

## تصمیم‌های باز (قبل از مرحله‌ای که لازمشان دارد از تو می‌پرسم)

- [ ] libstb چطور وارد شود: `add_subdirectory` کنار هم، `FetchContent`، یا `find_package`؟ (مرحله ۱)
- [ ] libstb به‌صورت static یا shared لینک شود؟ libstb اخیراً shared شده و روی Windows (DLL) و
      musllinux (segfault) مشکل داشته؛ این‌ها به novasvg هم منتقل می‌شود. (مرحله ۱)
- [ ] خروجی `render()` در API عمومی `Bitmap` بماند یا `stb::image` برگرداند؟ (مرحله ۲)
- [ ] پشتیبانی از ABI: آیا کتابخانه‌ی novasvg خودش هم shared عرضه شود یا فقط static؟ (مرحله ۴)

## قاعده‌ی نامگذاری (برگرفته از libstb)

| چیز | قاعده | مثال |
|---|---|---|
| نوع (class/struct/enum) | `snake_case` | `document`, `text_node`, `blend_mode` |
| تابع و متد | `snake_case` | `load_file`, `has_attribute` |
| getter | بدون `get_` | `width()`, `bounding_box()` |
| setter | `set_` | `set_attribute(...)` |
| bool | `is_` / `has_` | `is_null()`, `has_attribute()` |
| factory / تبدیل | `from_*` / `to_*` | `matrix::from_rotation(...)`, `to_string()` |
| عضو داده‌ی private | پسوند `_` | `node_` |
| enum | `enum class`، مقادیر `snake_case` | `text_align::center` |
| exception | پسوند `_error`، از `std::` مشتق | `invalid_color_error` |
| ماکرو | `NOVASVG_*` | `NOVASVG_API` |
| namespace | حروف کوچک | `novasvg`, `novasvg::detail` |

نگاشت دقیق هر نام عمومی در مرحله‌ی ۶ جدا نوشته و برای تأیید تو آورده می‌شود؛ مثال‌ها فقط جهت را
نشان می‌دهند.

## مراحل

### مرحله ۰ — baseline و ابزار راستی‌آزمایی
- [x] کلون novasvg و libstb
- [x] libstb به‌صورت static بیلد می‌شود
- [x] baseline novasvg بیلد و تست‌ها سبز (اعداد بالا)
- [x] `tools/golden.py` نوشته و آزموده شد، 62 تصویر golden ساخته شد (در `refactor/`، ignore‌شده)
- [x] این چک‌لیست
- [ ] اجرای تست‌های پایتون (نیاز به nanobind؛ در مرحله‌ی ۷ برمی‌گردیم)

### مرحله ۱ — لوله‌کشی CMake برای libstb
بدون تغییر در کد یا نام‌ها.
- [ ] تصمیم نحوه‌ی ورود libstb (بالا)
- [ ] تارگت `novasvg` به `libstb::core` لینک شود
- [ ] یک تست کوچک که `stb::` را از داخل novasvg include و لینک می‌کند (smoke test)
- [ ] تست‌ها + golden بدون تغییر

### مرحله ۲ — تصویر و zlib: حذف stb_image و stb_image_write
novasvg از stb_image فقط `stbi_load`, `stbi_load_from_memory`, `stbi_image_free`,
`stbi_zlib_decode_malloc` را صدا می‌زند؛ بقیه‌ی حدود 8 هزار خطِ `render/surface.h` کد vendor شده است.
- [ ] `stbi_load*` ← `stb::image::open` / `stb::image::decode`
- [ ] `stbi_zlib_decode_malloc` (فونت WOFF) ← `stb::zlib_decompress`
- [ ] `BitmapCodec` و `writeTo{Png,Bmp,Tga,Jpg}` ← `stb::image::to_*` / `write_*`
- [ ] مرز تبدیل `Bitmap` (premultiplied ARGB32) ↔ `stb::image` (RGBA ساده)، با تست
- [ ] حذف stb_image و stb_image_write از هدرها
- [ ] تست‌ها + golden؛ مراقب: خطاها (exception به‌جای کد برگشتی)، premultiply، 16-bit/HDR، EXIF

### مرحله ۳ — فونت: حذف stb_truetype
novasvg فقط ۱۲ تابع stbtt استفاده می‌کند. API فعلی `stb::font` متن را به bitmap رندر می‌کند، ولی
SVG به **outline برداری گلیف** نیاز دارد (`stbtt_GetGlyphShape`)، پس باید به libstb اضافه شود.
- [ ] بررسی دقیق نیازها: `glyph_index`، `glyph_outline` (move/line/quad/cubic)، h-metrics، glyph box،
      font bbox، تعداد face در collection، kerning (kern و GPOS)
- [ ] اضافه‌کردن این API به libstb (در ریپوی libstb، با تست و zip جدا)
- [ ] `font_face` / `FontFace` در novasvg روی `stb::font` سوار شود
- [ ] shaping فارسی/عربی (bidi، joining، GSUB) از `stb::shape_*` استفاده شود
- [ ] حذف stb_truetype و stb_rect_pack (حدود 5 هزار خط) از `render/font.h`
- [ ] تست‌ها + golden؛ مراقب: متن RTL، kerning، فونت جاسازی‌شده، WOFF، `.ttc`

### مرحله ۴ — از header-only به compiled
- [ ] مرز public/internal: هدرهای عمومی نازک (`document`, `element`, `node`, `box`, `matrix`)
- [ ] `detail/` به `src/` منتقل شود (یک ماژول در هر مرحله‌ی کوچک)
- [ ] `svgelement.h` (~7٫6 هزار خط، ~90 نوع) به ماژول‌های جدا بشکند: DOM، property، filter، gradient، text، foreignObject
- [ ] موتور CSS/HTML (`Rule`, `SimpleSelector`, `RuleIndex`, `HtmlNode`, `StyleNode`) ماژول مستقل شود
- [ ] `NOVASVG_API` و visibility
- [ ] تارگت‌های CMake: `novasvg::novasvg` (لایبرری واقعی)؛ `noflag` تکلیفش روشن شود
- [ ] زمان بیلد قبل/بعد اندازه‌گیری و ثبت شود

### مرحله ۵ — حذف تکرارها
نقشه‌ی فعلی: هر مفهوم در چند لایه تعریف شده (عمومی / `detail/canvas.h` / `detail/render/`).
- [ ] `Color` ← `stb::color` (در اصل از همین کد مشتق شده)
- [ ] یک `matrix` به‌جای `Matrix` / `Transform` / `matrix`
- [ ] یک `rect` / `point` / `size` / `box`
- [ ] یک `path`
- [ ] `Font`/`FontFace`/`FontFaceCache` و `font_face`/`font_face_cache`/`glyph` یکی شوند
- [ ] `Canvas`/`canvas` و `Bitmap`/`surface` تکلیفشان روشن شود
- [ ] `render/vendor/ft_*` (کد FreeType/plutovg) می‌ماند؛ فقط جایش مستند شود

### مرحله ۶ — نامگذاری stl-like
- [ ] جدول نگاشت نام‌های عمومی نوشته و تأیید شود
- [ ] rename با اسکریپت (مکانیکی)، یک ماژول در هر commit
- [ ] CLI، تست‌ها، مثال‌ها، بایندینگ پایتون (می‌تواند pythonic بماند) و docs به‌روز شوند
- [ ] تصمیم نهایی alias `nova`

### مرحله ۶٫۵ — بازنگری طراحی / OOP
- [ ] سلسله‌مراتب کلاس‌ها (`SVGElement` و زیرکلاس‌ها) و نقش دستگیره‌های عمومی (`element`, `node`)
- [ ] مدیریت خطا: exception (هم‌سبک libstb) یا کد بازگشتی؟
- [ ] مالکیت و lifetime در API عمومی
- [ ] thread-safety (مثل `stb::font`: غیرقابل‌تغییر و کم‌هزینه برای کپی)

### مرحله ۷ — بسته‌بندی و انتشار
- [ ] بایندینگ پایتون و تست‌های پایتون
- [ ] wheels / CI (Windows، macOS، manylinux، musllinux)
- [ ] `CHANGELOG`، README، docs، نسخه (breaking)
- [ ] `find_package(novasvg)` و `novasvgConfig.cmake`
