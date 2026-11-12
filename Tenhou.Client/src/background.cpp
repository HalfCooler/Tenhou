#include "background.hpp"
#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
#include <algorithm>
#include <cmath>
#include <memory>
#include <stdexcept>

namespace tenhou {
namespace {
struct Vec3 { float x, y, z; };
Vec3 Cross(Vec3 a, Vec3 b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
Vec3 Scale(Vec3 a, float s) { return {a.x * s, a.y * s, a.z * s}; }
Vec3 Normalize(Vec3 a) {
    const double length = std::sqrt(double(a.x) * a.x + double(a.y) * a.y + double(a.z) * a.z);
    return {float(a.x / length), float(a.y / length), float(a.z / length)};
}
void Check(Gdiplus::Status status, const char* operation) {
    if (status != Gdiplus::Ok) throw std::runtime_error(std::string("Background: ") + operation);
}
Image Pixels(Gdiplus::Bitmap& bitmap, int width, int height) {
    Image result;
    result.width = width;
    result.height = height;
    result.bgra.resize(std::size_t(width) * height * 4);
    Gdiplus::Rect area(0, 0, width, height);
    Gdiplus::BitmapData locked{};
    Check(bitmap.LockBits(&area, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &locked), "lock pixels");
    const auto* origin = static_cast<const std::uint8_t*>(locked.Scan0);
    for (int y = 0; y < height; ++y) {
        std::copy_n(origin + std::ptrdiff_t(y) * locked.Stride, std::size_t(width) * 4,
                    result.bgra.data() + std::size_t(y) * width * 4);
    }
    Check(bitmap.UnlockBits(&locked), "unlock pixels");
    return result;
}
}  // namespace
std::uint32_t AdjustOriginalHsv(std::uint32_t colorref,float hue,float saturation,float value) {
    // FUN_00467d60/00467db0/00467f40. COLORREF channels, x87 intermediates,
    // float stores and truncation (not rounding) are deliberate.
    const float r=float((colorref&255)*double(1.0f/255.0f));
    const float g=float(((colorref>>8)&255)*double(1.0f/255.0f));
    const float b=float(((colorref>>16)&255)*double(1.0f/255.0f));
    const float high=std::max({r,g,b}), delta=float(double(high)-std::min({r,g,b}));
    const float sat=high==0?0:float(double(delta)/high);
    double h=0;
    if (delta!=0) {
        if (r==high) h=(double(g)-b)/delta;
        else if (g==high) h=(double(b)-r)/delta+2;
        else h=(double(r)-g)/delta+4;
        h*=60; if (h<0) h+=360;
    }
    const float shifted=float(double(float(h))+hue);
    const float s=float(double(sat)*saturation), v=float(double(high)*value);
    h=shifted; while(h>=360) h-=360; while(h<0) h+=360;
    const double sector_value=h*double(1.0f/60.0f);
    const int sector=static_cast<int>(sector_value);
    double fraction=sector_value-sector;
    if (!(sector&1)) fraction=1-fraction;
    const float low=std::min(1.0f,float((1.0-double(s))*v));
    const double middle=std::min(1.0,(1.0-fraction*s)*v);
    const double top=std::min(1.0,double(v));
    double red=0,green=0,blue=0;
    switch(sector) {
    case 0: red=top; green=middle; blue=low; break;
    case 1: red=middle; green=top; blue=low; break;
    case 2: red=low; green=top; blue=middle; break;
    case 3: red=low; green=middle; blue=top; break;
    case 4: red=middle; green=low; blue=top; break;
    case 5: red=top; green=low; blue=middle; break;
    default: return 0xffffffff;
    }
    const auto byte=[](double x) { return static_cast<unsigned>(static_cast<int>(x*255))&255; };
    return byte(red)|(byte(green)<<8)|(byte(blue)<<16);
}
Image BuildPreferredBackground(const std::filesystem::path& directory,const std::filesystem::path& config,
    int width,int height,const ClientPreferences& preferences) {
    const auto number=[&](unsigned index,int fallback) {
        const auto& text=preferences.background_lhsv[index]; if (text.empty()) return fallback;
        wchar_t* end=nullptr; const auto value=wcstol(text.c_str(),&end,10);
        return end==text.c_str()?fallback:static_cast<int>(value);
    };
    std::array<std::array<float,3>,3> hsv{};
    for (unsigned band=0;band<3;++band) hsv[band]={float(std::clamp(number(1+band*3,0),0,359)),
        float(std::clamp(number(2+band*3,255),0,256)*double(1.0f/255.0f)),
        float(std::clamp(number(3+band*3,255),0,256)*double(1.0f/255.0f))};
    auto source=preferences.background_source==L"./BG1"?directory/L"_bg1.bmp":std::filesystem::path(preferences.background_source);
    if (source.empty()) source=directory/L"_bg.bmp";
    else if (source.is_relative() && preferences.background_source!=L"./BG1") source=config.parent_path()/source;
    // The original #FF0000 preset uses a red solid source, then adjusts HSV.
    const float strength=float(std::sqrt(float(width>>10))*(10-std::clamp(number(0,2),0,10)));
    if (preferences.background_source==L"#FF0000") {
        Image image; image.width=width; image.height=height; image.bgra.resize(std::size_t(width)*height*4);
        const auto rgb=AdjustOriginalHsv(0xff,hsv[0][0],hsv[0][1],hsv[0][2]);
        for (std::size_t i=0;i<image.bgra.size();i+=4) { image.bgra[i]=static_cast<std::uint8_t>(rgb>>16); image.bgra[i+1]=static_cast<std::uint8_t>(rgb>>8); image.bgra[i+2]=static_cast<std::uint8_t>(rgb); image.bgra[i+3]=255; }
        ApplyBackgroundLighting(image,strength,600); return image;
    }
    return BuildBackground(source,width,height,strength,600,&hsv);
}

void ApplyBackgroundLighting(Image& image, float strength, float light_y) {
    if (image.width <= 0 || image.height <= 0 ||
        image.bgra.size() != std::size_t(image.width) * image.height * 4 ||
        !std::isfinite(strength) || !std::isfinite(light_y) || strength < 0 || strength > 100) {
        throw std::runtime_error("Background: invalid lighting input");
    }
    if (strength == 0) return;
    // FUN_004083e0 camera: eye(0,480,420), target(0,0,20), up(0,1,0).
    // Original x87 computations round at these float-vector assignments.
    const Vec3 backward_unit = Normalize({0, 480, 400});
    const Vec3 right = Normalize(Cross({0, 1, 0}, backward_unit));
    const Vec3 vertical_basis = Cross(backward_unit, right);
    const Vec3 up{vertical_basis.x / 1.33f, vertical_basis.y / 1.33f, vertical_basis.z / 1.33f};
    const float focal = float(0.5 / std::tan(0.2617993298918009));
    const Vec3 backward = Scale(backward_unit, focal);
    for (int row = 0; row < image.height; ++row) {
        // FUN_00408000 processes a positive-height, bottom-up 24bpp DIB.
        const int original_row = image.height - 1 - row;
        const float sy = -(float(original_row - image.height / 2) / float(image.height));
        for (int x = 0; x < image.width; ++x) {
            const float sx = float(x - image.width / 2) / float(image.width);
            const Vec3 horizontal = Scale(right, sx);
            const Vec3 vertical = Scale(up, sy);
            const Vec3 ray{horizontal.x + vertical.x - backward.x,
                           horizontal.y + vertical.y - backward.y,
                           horizontal.z + vertical.z - backward.z};
            const double t = -480.0 / double(ray.y);
            if (!(t > 0)) continue;
            // Original stores x/z after x87 multiply+add, and fixes intersection y=0.
            const Vec3 hit{float(double(ray.x) * t), 0, float(double(ray.z) * t + 420.0)};
            const Vec3 light{-hit.x, light_y, -75.0f - hit.z};
            const Vec3 direction = Normalize(light);
            const double factor = std::max(0.0, 1.0 - (1.0 - double(direction.y)) * strength);
            if (factor >= 1.0) continue;
            auto* p = image.bgra.data() + (std::size_t(row) * image.width + x) * 4;
            for (int channel = 0; channel < 3; ++channel) p[channel] = std::uint8_t(p[channel] * factor);
        }
    }
}

Image BuildBackground(const std::filesystem::path& source, int width, int height,
                       float light_strength, float light_y,const std::array<std::array<float,3>,3>* hsv) {
    if (width <= 0 || height <= 0 || width > 4096 || height > 4096) {
        throw std::runtime_error("Background: dimensions outside 1..4096");
    }
    std::unique_ptr<Gdiplus::Bitmap> input(Gdiplus::Bitmap::FromFile(source.c_str()));
    Check(input->GetLastStatus(), "open source bitmap");
    const int input_width = static_cast<int>(input->GetWidth());
    const int input_height = static_cast<int>(input->GetHeight());
    const bool stacked = (input_width == 1024 && input_height == 3840) ||
                         (input_width == width && input_height == height * 5);
    Gdiplus::Bitmap strips(width, height * 5, PixelFormat32bppARGB);
    Check(strips.GetLastStatus(), "create five-strip bitmap");
    {
        Gdiplus::Graphics graphics(&strips);
        Check(graphics.Clear(Gdiplus::Color(0, 0, 0, 0)), "clear five strips");
        Check(graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic), "interpolation");
        Check(graphics.DrawImage(input.get(), Gdiplus::Rect(0, 0, width, stacked ? height * 5 : height),
                                  0, 0, input_width, input_height, Gdiplus::UnitPixel), "scale source");
    }
    Image strip_pixels = Pixels(strips, width, height * 5);
    const std::size_t band_bytes = std::size_t(width) * height * 4;
    // Strip pairs (1,2), (3,4): RGB image plus red-channel alpha mask.
    for (std::size_t p = 0; p < band_bytes; p += 4) {
        strip_pixels.bgra[band_bytes + p + 3] = strip_pixels.bgra[2 * band_bytes + p + 2];
        strip_pixels.bgra[3 * band_bytes + p + 3] = strip_pixels.bgra[4 * band_bytes + p + 2];
    }
    if (hsv) for (unsigned layer=0;layer<3;++layer) {
        const auto& adjust=(*hsv)[layer]; if (adjust==std::array<float,3>{0,1,1}) continue;
        const auto start=(layer==0?0:layer==1?1:3)*band_bytes;
        for (std::size_t p=start;p<start+band_bytes;p+=4) {
            auto* pixel=strip_pixels.bgra.data()+p;
            const auto rgb=AdjustOriginalHsv(pixel[2]|(unsigned(pixel[1])<<8)|(unsigned(pixel[0])<<16),adjust[0],adjust[1],adjust[2]);
            pixel[0]=static_cast<std::uint8_t>(rgb>>16); pixel[1]=static_cast<std::uint8_t>(rgb>>8); pixel[2]=static_cast<std::uint8_t>(rgb);
        }
    }
    Gdiplus::Bitmap masked(width, height * 5, width * 4, PixelFormat32bppARGB, strip_pixels.bgra.data());
    Gdiplus::Bitmap output(width, height, PixelFormat32bppARGB);
    Check(output.GetLastStatus(), "create composed bitmap");
    {
        Gdiplus::Graphics graphics(&output);
        Check(graphics.Clear(Gdiplus::Color(255, 0, 0, 0)), "clear composed bitmap");
        for (int band : {0, 1, 3}) {
            Check(graphics.DrawImage(&masked, Gdiplus::Rect(0, 0, width, height),
                                      0, band * height, width, height, Gdiplus::UnitPixel), "composite strip");
        }
    }
    Image result = Pixels(output, width, height);
    for (std::size_t p = 3; p < result.bgra.size(); p += 4) result.bgra[p] = 255;
    ApplyBackgroundLighting(result, light_strength, light_y);
    return result;
}
}  // namespace tenhou
