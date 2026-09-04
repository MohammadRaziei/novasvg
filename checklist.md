# novasvg — کارهای باقی‌مونده

> کانتکست: این پروژه (`github.com/mohammadraziei/novasvg`) یه رندرر SVG→PNG/BMP/TGA/JPG هست.
> امروز روش کار شد: رندر foreignObject (مرمید)، بازنویسی کامل stb_image_write به سبک OOP خودمون
> (کلاس `BitmapCodec`)، یکدست‌سازی `NOVASVG_INLINE` توی کل کدبیس، و جدا کردن `detail/` به فایل‌های
> عمومیِ مستقل (`bitmap.h`, `canvas.h`, `font.h`, `color.h`). zip آخرین نسخه رو ضمیمه کن یا از
> `github.com/mohammadraziei/novasvg` بگیر.

## معماری — کارهای باقی‌مونده

- [ ] **`detail/svgelement.h` رو بشکن** (۶۰۸۳ خط، ۲۰۰K) — الان تنها فایل بزرگیه که دست‌نخورده مونده.
      حداقل به `document.h` (کلاس `Document`/`Element` عمومی) و یه چیزی برای درخت DOM داخلی
      (`SVGElement` و زیرکلاس‌هاش، صرفاً داخلی) تقسیم بشه.
- [ ] **`detail/svgparser.hpp`** رو بررسی کن ببین جدا کردنش (یا merge با svgelement) منطقی‌تره یا نه.
- [ ] تصمیم بگیر `detail/render/` بمونه همین اسم یا به چیزی مثل `detail/engine/` تغییر کنه —
      امروز تصمیم گرفتیم دست نزنیم چون واقعاً داخلیه و اسمش مشکلی نداره، ولی اگه نظر عوض شد اینجاست.
- [ ] بعد از هر تغییر ساختاری: pixel-diff + ۴۶ تستِ `novasvg_test_cpp` + تستِ standalone بودنِ
      هر فایل عمومی (`bitmap.h`/`canvas.h`/`font.h`/`color.h`) رو دوباره بزن — الگوش توی همین
      چت هست، هر بار جواب داد.

## باگ‌ها / محدودیت‌های شناخته‌شده (حل نشده، عمداً)

- [ ] **آیکون‌های Font Awesome توی foreignObject رندر نمی‌شن** (مثل `<i class="fa fa-car">`).
      علتش نیاز به دانلود واقعیِ فونت از یه CDN بیرونیه — تصمیم گرفتیم بهش دست نزنیم چون این
      کتابخونه عمداً بدون وابستگی به شبکه‌ست. اگه لازم شد، گزینه‌هاش توی چت قبلی بحث شده
      (fetch اختیاری، یا یه ست کوچیک آیکون embedded).
- [ ] **عدم تطابق فونت با مرورگر واقعی**: وقتی فونت اصلیِ SVG (مثلاً Trebuchet MS) روی سیستم نصب
      نیست، novasvg فونت جایگزین انتخاب می‌کنه که ممکنه عرضش فرق کنه. `ForeignObjectSimple` الان
      این حالت رو با فشرده‌کردن افقیِ متن (shrink-to-fit) پوشش می‌ده، ولی نتیجه بازم صد‌درصد با
      رندر واقعیِ مرورگر یکی نیست (فقط دیگه بریده نمی‌شه).
- [ ] `ForeignObjectSimple` فقط تگ‌های مستقیمِ تودرتو رو برای پس‌زمینه چک می‌کنه (نه selectorهای CSS
      پیچیده‌تر مثل `.a .b`). برای اکثر خروجی‌های Mermaid کافیه؛ برای HTML دلخواه ممکنه ناقص باشه.

## تست/اعتبارسنجی

- [ ] **پایتون‌بایندینگ هیچ‌وقت واقعاً build نشد** — پکیج `nanobind` توی این سندباکس نصب نیست.
      باید توی یه محیط با نصبِ کامل تست بشه (`pip install nanobind` بعد `cmake --build . --target novasvg_build_python`).
- [ ] تست‌ها فقط روی این سندباکس (Linux/GCC) زده شدن؛ روی macOS/Windows/Clang چک نشده.
- [ ] `ctoon` (ریپوی دیگه‌ی همین یوزر، کلون شده ولی هیچ‌وقت بررسی نشده) — اگه لازمه بررسیش کنیم.

## ایده‌های مطرح‌شده ولی بی‌جواب مونده

- [ ] `phasma` (پکیج pip خودِ mohammadraziei، رندرر PhantomJS-based) به‌عنوان مرجع تست استفاده شد
      ولی معلوم شد فونت‌های قدیمیِ PhantomJS گمراه‌کننده‌ست — مرجع نهایی Playwright/Chromium واقعی بود.
      اگه دوباره خواستی مقایسه‌ی خودکار بگیری، از Playwright استفاده کن نه phasma.

## کارهای این نشست — فیلترها و فیکس‌های SVG/CSS

> کانتکست: این بخش مربوط به یه نشست جدیده که novasvg رو در مقابل resvg/lunasvg/cairosvg/thorvg
> تست کردیم (`COMPARISON.md` رو ببین). موارد زیر توی همین نشست پیاده و تست شدن.

### انجام‌شده

- [x] **فیلترهای `feGaussianBlur` / `feDropShadow`** — `Canvas::boxBlur()` (تقریب box-blur سه‌پاسه)،
      با استفاده از همون مکانیزم offscreen-canvas/blendCanvas که mask ازش استفاده می‌کرد.
- [x] **گراف کامل filter primitives** — `feOffset`, `feFlood`, `feComposite` (با همه‌ی حالت‌های
      Porter-Duff: Over/In/Out/Atop/Xor)، `feMerge`/`feMergeNode`، زنجیره‌ی واقعیِ `in`/`in2`/`result`
      (شامل `SourceGraphic`/`SourceAlpha`). با Template Method پیاده شد:
      `SVGElement::applyFilterPrimitive()` مجازیه، هر primitive یه کلاس کوچیک جداست، و
      `SVGFilterElement` فقط اجراکننده‌ی pipeline هست (بدون switch روی نوع) — اضافه‌کردن primitive
      جدید بعداً یعنی فقط یه کلاس جدید، نه دست‌زدن به اجراکننده. تست شد: یه drop-shadow کامل که
      فقط از ۵ تا primitive زنجیره شده ساخته شد (`data/feature-filter-primitives.svg`) دقیقاً
      همون خروجیِ shorthand `feDropShadow` رو داد.
- [x] **CSS `transform:` property** — دو باگ مستقل، هر دو ریشه‌یابی و فیکس شدن:
      ۱) پارسر `matrix.h` واحدهای CSS (`deg`/`rad`/...) رو قبول نمی‌کرد و کل transform رو حذف
      می‌کرد. ۲) دیکلریشن‌های داخل `<style>` از جدول lookup اشتباه (فقط hyphenated) استفاده
      می‌کردن، پس `transform` (camelCase) اصلاً دیده نمی‌شد.
- [x] **چسبیدن کلمات به هم توی متن foreignObject** (مثل `<br/>`) — `foreignObjectPlainText()`
      الان سر هر مرز تگ یه space می‌ذاره.
- [x] **باگ ارث‌بری رنگ متن foreignObject (green-on-green)** — ریشه‌یابی و فیکس شد: `classDef green`
      مرمید به `.green>*{fill:#9f6 !important}` تبدیل می‌شه که به‌درستی `<g class="label">` رو هم
      می‌گیره (چون اونم فرزند مستقیم همون g سبزه)، و چون `fill` توی SVG ارث‌بری می‌شه، متن هم سبز
      می‌شد و روی پس‌زمینه‌ی سبز نامرئی می‌شد. فیکس: یه تابع جدید `foreignObjectTextColor()` اضافه
      شد که دقیقاً مثل `foreignObjectBackgroundColor()` عمل می‌کنه ولی برای CSS property `color`
      (نه `fill`) — یعنی رنگ متن از خودِ استایل/کلاسِ HTML میاد، نه از زنجیره‌ی SVG. helperهای
      مشترک (`findTagColor`/`tagColor`/`findClassColor`/`parseColorDeclaration`) هم عمومی شدن تا
      بین background-color و color به اشتراک گذاشته بشن. تست شد: لیبل "Inner / circle..." الان
      مشکی دیده می‌شه، و لیبل‌های رنگی venn (Backend سبز، Frontend آبی) هم بدون رگرشن درستن.
- [x] **نکته‌ی مهم پردازشی**: جدول‌های lookup (`propertyid`/`csspropertyid`/`elementid`) با
      `std::lower_bound` (باینری سرچ) کار می‌کنن، پس باید همیشه sorted بمونن. اضافه‌کردن یه entry
      خارج از ترتیب الفبایی خطای build نمی‌ده، فقط silently لوکاپ رو برای یه بازه از کلیدها خراب
      می‌کنه. (یه بار روی `stdDeviation` گرفتارش شدیم.)

### عمداً پیاده نشده (scope کوچیک نگه داشته شد)

- [ ] `feComposite operator="arithmetic"` — به ۴ ضریب k1..k4 نیاز داره؛ فعلاً fallback به `over`.
- [ ] filter region جدا برای هر primitive (`x`/`y`/`width`/`height` روی `<filter>` یا خود primitive) —
      الان همه‌ی زنجیره یه ناحیه‌ی offscreen مشترک دارن (بر اساس bbox المان + margin).
- [ ] `feColorMatrix`, `feTurbulence`, `feDisplacementMap`, `feTile`, `feImage`,
      `feDiffuseLighting`/`feSpecularLighting`, `feConvolveMatrix`, `feMorphology`,
      `feComponentTransfer`, `feBlend` — پیاده نشدن، به عنوان element ثبت نشدن، پس مثل قبل
      silently نادیده گرفته می‌شن (نه crash).
- [ ] wrap واقعیِ چندخطی توی foreignObject — نیاز به یه لایه‌ی layout واقعی داره، نه یه پچ کوچیک.
- [ ] zenuml (HTML/CSS تودرتوی سنگین) — از scope "فیکس" خارجه، به یه HTML/CSS layout engine واقعی نیاز داره.

### نکات بهینه‌سازی (باگ نیستن، فقط برای بعداً یادداشت شدن)

> همه‌ی موارد زیر الان اسکالر (غیر-SIMD) هستن و درست کار می‌کنن، فقط سریع‌ترین حالت ممکن نیستن.
> فیلترها روی هر پیکسل از bbox (بزرگ‌شده‌ی) المان اجرا می‌شن، پس اگه یه جای پروژه قراره کند باشه
> احتمالاً همینجاست.

- **`Canvas::boxBlur()` / `boxBlurPass()` (`canvas.h`)** — مهم‌ترین مورد. الان هر پیکسل خروجی کل
  پنجره‌ی `2*radius+1` رو از صفر جمع می‌زنه یعنی `O(width*height*radius)` هر پاس. دو بهینه‌سازیِ
  مستقل، هرکدوم به تنهایی مهمن:
  - **moving-sum به‌جای جمع دوباره**: یه پنجره‌ی لغزنده (پیکسلی که از پنجره خارج می‌شه رو کم کن،
    پیکسل جدید رو اضافه کن) هر پاس رو به `O(width*height)` می‌رسونه، مستقل از radius — برای
    stdDeviation‌های معمولِ محتوای SVG (اغلب radius=۵ تا ۲۰+) این بیشترین تاثیر رو داره.
  - **SIMD روی ۴ کانال با هم**: هر پیکسل از یه `uint32_t` با شیفت جدا می‌شه و توی `float sum[4]`
    اسکالر جمع می‌شه. چون از قبل ۴ بایت پک‌شده‌ست، مستقیم روی SSE2/NEON می‌شه سوارش کرد: ۴ پیکسل
    (۱۶ بایت) با هم لود، widen به ۱۶/۳۲ بیت، جمع، narrow برگردون — یعنی هر instruction چند پیکسل
    با هر ۴ کانال رو با هم پردازش می‌کنه به‌جای یه کانال از یه پیکسل در هر iteration. یا با یه
    کرنل دستی SSE2/NEON، یا حتی فقط با بازنویسیِ حلقه به شکلی که autovectorization راحت‌تر باشه
    (branch مربوط به edge-clamp رو از داخل hot loop خارج کن، به‌جاش scratch buffer رو padding کن) —
    `-O3` باید بتونه بیشترش رو خودش vectorize کنه.
  - ترکیب هر دو (moving-sum + SIMD) بیشترین سود رو می‌ده اگه یه‌روز واقعاً مهم شد.
- **`Canvas::compositeWith()` (`canvas.h`)** — یه `switch` روی `mode` *داخل* حلقه‌ی per-pixel هست.
  بردنش بیرون از حلقه (یا با template parameter روی mode، یا انتخاب یه تابع/لامبدا یه‌بار قبل حلقه)
  یه branch رو از هر پیکسل حذف می‌کنه و شانس auto-vectorization رو هم بیشتر می‌کنه. همین برای
  `tintToFloodColor()`/`shift()` هم صادقه، ولی اونا از قبل branch کمتری دارن.
- **الگوی "clone بعد mutate" توی `SVGFilterContext::cloneCanvas()`** — هر primitive که نیاز به
  کپیِ خودش داره (`feGaussianBlur`, `feOffset`, `feDropShadow`, `in2` توی `feComposite`) یه
  Canvas کامل جدید می‌سازه و یه `compositeOver` کامل می‌زنه فقط برای کپی ۱-به-۱ پیکسل‌ها، درحالی
  که هیچ blend واقعی‌ای لازم نیست. یه `Canvas::clone()` مخصوص (فقط `memcpy`، بدون ریاضیِ آلفا)
  هم ساده‌تره هم سریع‌تر از رد کردنِ یه کپیِ ساده از مسیر عمومیِ Porter-Duff.
- **`gaussianRadiusForSigma()`** — `sqrt`/ضرب رو هر بار از نو حساب می‌کنه؛ کوچیکه، ولی اگه
  `boxBlur` توی یه حلقه‌ی داغ صدا زده بشه (مثلاً فیلتر انیمیشنی)، memoize کردن بر اساس مقدار
  `stdDeviation` از تکرار محاسبات float بی‌فایده جلوگیری می‌کنه.

هیچ‌کدوم از اینا برای درستیِ کد لازم نیستن — همه‌چی الان به شکل اسکالر تست‌هاش رو پاس می‌کنه.
اینجا نوشته شدن که یه پاس بعدی نقطه‌ی شروع مشخص داشته باشه به‌جای "profile کن و ببین".
