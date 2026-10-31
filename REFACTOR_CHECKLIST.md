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

- [x] libstb چطور وارد شود: **`FetchContent` از گیت** (فعلاً؛ تو بعداً می‌گویی اگر عوض شود). پین‌شده روی commit `765f48c`.
- [x] libstb **حتماً dynamic (shared)** لینک می‌شود؛ در CMake اجباری است (اگر shared نباشد configure خطا می‌دهد).
      هنوز باز: libstb اخیراً shared شده و روی Windows (DLL) و musllinux (segfault) مشکل داشته؛ این‌ها به
      novasvg هم منتقل می‌شود و در مرحله ۷ (wheels) باید جدی بررسی شود.
- [x] خروجی `render()` در API عمومی: **`stb::image`** (و `Bitmap` از API عمومی بیرون می‌رود، مرحله ۲ب).
      سطح رسم داخلی **premultiplied می‌ماند و خصوصی است**. بنچمارک (۴۰۹۶×۴۰۹۶، ۱۶٫۷ مگاپیکسل):
      composite روی premultiplied ۱٫۶ تا ۲٫۶ ns/پیکسل، روی straight alpha ۴٫۸ تا ۶٫۰ ns/پیکسل (۲ تا ۴ برابر کندتر،
      چه پس‌زمینه شفاف چه opaque). تبدیل نهایی premultiplied → RGBA در محل: ۱٫۳ تا ۵٫۳ ns/پیکسل، یعنی ۱٫۵ تا ۶ درصد
      کل رندر (۲۴ تا ۸۹ ms در برابر ۱٫۴ تا ۱٫۶ s)؛ و بدون کپی با `stb::image::wrap` تحویل داده می‌شود.
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

### مرحله ۱ — لوله‌کشی CMake برای libstb ✅
بدون تغییر در کد یا نام‌ها.
- [x] ورود libstb با `FetchContent` (repo و commit قابل override: `NOVASVG_LIBSTB_GIT_REPOSITORY` / `NOVASVG_LIBSTB_GIT_TAG`؛
      برای کار با نسخه‌ی لوکال: `-DFETCHCONTENT_SOURCE_DIR_LIBSTB=/path/to/libstb`؛ آفلاین: `-DFETCHCONTENT_FULLY_DISCONNECTED=ON`)
- [x] `novasvg_noflag` به `libstb::core` لینک شد (`novasvg::novasvg` از آن ارث می‌برد)
- [x] shared بودن اجباری است (`STB_BUILD_SHARED=ON` با FORCE + بررسی نوع تارگت)
- [x] تست دودی `test_novasvg_libstb`: فقط به `novasvg::noflag` لینک می‌شود، round-trip رنگ/zlib/PNG می‌زند،
      و (روی Linux/macOS با `dladdr`) ثابت می‌کند libstb یک shared object جداست
- [x] install و `find_package(novasvg)` آزموده شد با یک پروژه‌ی مصرف‌کننده‌ی جدا
- [x] تست‌ها + golden بدون تغییر

نتیجه: `test_novasvg_cpp` 64/377، `test_novasvg_embedded_font` 5/71، `test_novasvg_libstb` OK،
golden 62 تصویر با تفاوت صفر. هیچ شیء سنگین تست دوباره کامپایل نشد (دستور کامپایل‌شان عوض نشده).

آنچه در این مرحله پیدا و درست شد:
- **تداخل ماژول‌های CMake**: هر دو پروژه `Startup`/`Optimize`/`DynamicVersion` دارند و libstb مسیر خودش را
  *بعد از* مسیر novasvg به `CMAKE_MODULE_PATH` اضافه می‌کند؛ پس بی‌صدا ماژول novasvg را برمی‌داشت و فلگ‌هایش
  عوض می‌شد (`-fstrict-aliasing` افتاده بود). مسیر ماژول‌های ما هنگام کانفیگ libstb موقتاً خالی می‌شود.
  پیشنهاد برای ریپوی libstb: `list(PREPEND CMAKE_MODULE_PATH ...)` به‌جای `APPEND`، تا این مشکل از ریشه برود.
- **install**: config باید `find_dependency(libstb)` داشته باشد (اضافه شد). نصب novasvg، libstb را هم در همان
  prefix نصب می‌کند.
- **RPATH نصب‌شده**: CLI نصب‌شده `$ORIGIN/../lib` را گرفت (روی macOS `@loader_path/../lib`)، وگرنه بعد از
  نصب libstb را پیدا نمی‌کرد. روی Windows `stb.dll` کنار `novasvg.exe` نصب می‌شود.
- حداقل نسخه‌ی CMake از 3.19 به **3.21** رفت (libstb لازم دارد).
- `ldd novasvg` هنوز libstb را نشان نمی‌دهد: CLI فعلاً نمادی از آن صدا نمی‌زند و لینکر (`--as-needed`) حذفش
  می‌کند. از مرحله‌ی ۲ که کد از `stb::` استفاده کند ظاهر می‌شود.
- تست‌های موجود (`test_novasvg_cpp`، `test_novasvg_embedded_font`) هنوز به `novasvg::noflag` لینک نمی‌شوند؛
  در مرحله‌ی ۲ که به libstb نیاز پیدا می‌کنند، این اتصال اضافه می‌شود.

### مرحله ۲ — تصویر و zlib: حذف stb_image و stb_image_write

#### ۲الف — حذف کد vendor شده ✅
- [x] `stbi_load` / `stbi_load_from_memory` ← `stb::image::open` / `decode` با `format = rgba` (EXIF اعمال نمی‌شود،
      مثل قبل)؛ خطا همچنان `nullptr` برمی‌گرداند
- [x] `stbi_zlib_decode_malloc` (فونت جاسازی‌شده) ← `stb::zlib_decompress`
- [x] `BitmapCodec` (حدود ۱٬۴۰۰ خط، پورت stb_image_write) حذف شد؛ `writeTo{Png,Bmp,Tga,Jpg}` روی `stb::image`
- [x] stb_image (حدود ۷٬۷۰۰ خط) از `render/surface.h` حذف شد (جمعاً حدود ۹٬۱۵۰ خط)
- [x] تست‌ها به `libstb::core` لینک شدند
- [x] تست‌ها: 64/377، 5/71، libstb link OK؛ golden 62 تصویر با تفاوت صفر؛ نماد `stbi_*` در باینری‌ها صفر
- نتیجه: CLI از 1٫4 به 1٫3 مگابایت، تست اصلی از 2٫4 به 1٫8 مگابایت؛ `ldd novasvg` حالا libstb را نشان می‌دهد
- باگ پنهان که ضمن کار درست شد: نوشتن یک `Bitmap` با stride بزرگ‌تر از `width*4` (Bitmap روی حافظه‌ی دست کاربر)
  در `BitmapRgbaScratch` پشت بافر می‌نوشت (heap overflow)؛ حالا سطر به سطر تبدیل می‌شود
- تفاوت‌های ریز و شناخته‌شده: بایت‌های PNG/JPEG/BMP/TGA خروجی عوض می‌شود (انکودر libstb، پیکسل‌ها یکسان)؛ فرمت Softimage PIC
  که stb_image می‌خواند دیگر پشتیبانی نمی‌شود؛ حد decode برابر 512 MiB است؛ کیفیت JPEG همچنان به 1..100 clamp می‌شود
- [ ] README/docs/لایسنس‌ها هنوز از stb_image نام می‌برند (در مرحله ۷ مرتب می‌شود)

#### ۲ب — `Bitmap` از API عمومی بیرون برود
- [ ] `render()` → `stb::image` (تبدیل در محل + `image::wrap` با keep_alive روی surface، بدون کپی)
- [ ] رندر روی تصویر موجود: لایه‌ی شفاف رندر شود و با `stb::image::blend` ترکیب شود (به‌جای رسم مستقیم روی straight alpha)
- [ ] `Bitmap` به نوع داخلی (`detail`) تبدیل شود؛ `writeTo*` حذف (از `stb::image` استفاده می‌شود)
- [ ] CLI، تست‌ها، مثال‌ها و بایندینگ پایتون (numpy interop) به‌روز شوند
- [ ] تست‌ها + golden؛ مراقب: premultiply/unpremultiply، رندر روی تصویر موجود، stride

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
