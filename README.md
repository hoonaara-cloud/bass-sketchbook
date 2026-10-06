# 🎸✏️ Groove Sketchbook — مولتی‌افکت بیس با حس دفتر طراحی

پلاگین مولتی‌افکت مخصوص **گیتار بیس**؛ کل رابط کاربری شبیه یک صفحه از دفتر اسکچ است:
ناب‌ها، امپ، پدال‌ها و حتی جای‌گذاری میکروفن با خط مداد کشیده شده‌اند.

زنجیره سیگنال:

```
BASS IN → COMP → OCT → DRIVE → FILTER → CHORUS → DELAY → AMP → CAB+MIC → OUT
```

۶ پریست فکتوری بر اساس تن نوازنده‌های افسانه‌ای (هرکدام یک «صفحه» از دفتر + دستور پخت دست‌نویس!):

| صفحه | نوازنده | ایده تن |
|---|---|---|
| 1 | FLEA | اسلپ فانک، StingRay → GK 800RB |
| 2 | BOOTSY | اسپیس‌فانک با انولوپ باز |
| 3 | JACO | فرتلس با mwah و کرس دوبرابر |
| 4 | GEDDY | گرایند راک با ترکیب DI |
| 5 | CLIFF | فاز دایم + واه (Anesthesia) |
| 6 | WOOTEN | مدرن های‌فای با ساب |

---

## روش ۱: ساخت VST3 بدون نصب هیچ‌چیز (GitHub Actions) ⭐ پیشنهادی

اگر حوصله نصب Visual Studio را نداری:

1. همین پوشه را در یک ریپوی گیت‌هاب بریز و push کن.
2. برو به تب **Actions** → ورک‌فلو `Build Windows VST3` خودش اجرا می‌شود (۱۰ تا ۲۰ دقیقه).
3. از بخش **Artifacts** فایل `GrooveSketchbook-VST3` را دانلود و unzip کن.
4. پوشه `Groove Sketchbook.vst3` را کپی کن به:
   ```
   C:\Program Files\Common Files\VST3\
   ```
5. در DAW یک Rescan بزن و دنبال **Groove Sketchbook** از **Hoonaar Audio** بگرد.

> فایل Standalone (`GrooveSketchbook-Standalone`) هم ساخته می‌شود؛ یک `.exe` که بدون DAW اجرا می‌شود و برای تست سریع عالی است.

## روش ۲: بیلد روی خود ویندوز

پیش‌نیازها (فقط یک‌بار):

- **Visual Studio 2022** با ورک‌لود `Desktop development with C++` (رایگان: Community)
- **CMake** نسخه 3.22+ از [cmake.org](https://cmake.org/download/) (تیک Add to PATH را بزن)
- **Git** از [git-scm.com](https://git-scm.com/download/win)

بعد:

```bat
build-windows.bat
```

بار اول JUCE (حدود ۲۰۰ مگ) دانلود می‌شود؛ صبور باش. خروجی‌ها:

```
build\GrooveSketchbook_artefacts\Release\VST3\Groove Sketchbook.vst3
build\GrooveSketchbook_artefacts\Release\Standalone\Groove Sketchbook.exe
```

نصب VST3: روی `install-windows.bat` راست‌کلیک → **Run as administrator**.

---

## کار با پلاگین

- **نوار بالا:** انتخاب نوازنده (کل میز بازطراحی می‌شود).
- **زنجیره وسط:** کلیک روی هر پدال = روشن/خاموش + نمایش ناب‌های بزرگش پایین.
- **امپ:** ۵ ناب + سوییچ Tube/Solid و 800W/300W.
- **کب + میک:** انتخاب کبینت و میکروفن + نمودار زنده جای‌گذاری (فاصله/زاویه/اتاق).
- **یادداشت:** پایین سمت راست، recipe هر تن را بخوان!
- همه ناب‌ها با **دبل‌کلیک** به وسط برمی‌گردند.

## عیب‌یابی

| مشکل | راه‌حل |
|---|---|
| بیلد اول خیلی طول می‌کشد | طبیعی است؛ JUCE + کامپایل اولیه. بار دوم سریع است. |
| خطای `No CMAKE_CXX_COMPILER` | Visual Studio با ورک‌لود C++ نصب نیست، یا ترمینال قدیمی است. |
| DAW پلاگین را نمی‌بیند | مسیر VST3 را چک کن + Rescan. بعضی DAWها لیست سیاه دارند (مثلا مسیر Blocklist را پاک کن). |
| صدا با کمی تأخیر (latency) | به‌خاطر کانولوشن کبینت است (حدود ۵۰۰–۱۰۰۰ سمپل)؛ پلاگین مقدار دقیق را به DAW گزارش می‌دهد و DAW جبران می‌کند. |
| نویز/خرخر در گین بالا | سافت‌کلیپر خروجی از کلیپ دیجیتال جلوگیری می‌کند؛ Output را کم کن. |

## تست DSP (برای دولوپرها)

```bash
cmake -S . -B build -DGROOVE_BUILD_TESTS=ON
cmake --build build --target GrooveDSPTest
./build/GrooveDSPTest   # هر ۶ پریست را پردازش و NaN را چک می‌کند
```

## لایسنس ⚠️ مهم

- **سورس این پروژه:** MIT (فایل LICENSE).
- **فریم‌ورک JUCE:** دوال‌لایسنس GPLv3 / تجاری. باینری‌ای که می‌سازی و **منتشر** می‌کنی، تحت GPLv3 است مگر اینکه لایسنس تجاری JUCE داشته باشی. استفاده شخصی مشکلی ندارد.
- **فونت Caveat:** SIL Open Font License 1.1 (رایگان، قابل امبد).

## نقشه راه

- پدال Wah دستی (MIDI Expression) • سینک Delay با تمپو • ایمپالس‌های واقعی کبینت • مقایسه A/B • پریست کاربر + Undo کامل
