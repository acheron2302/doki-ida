//----------------------------------------------------------------------------
// Doki Theme - color primitives and the RGB <-> IDA BGR conversion.
//
// Kept free of any IDA SDK dependency so the conversion logic is trivially
// testable. IDA's bgcolor_t is 0xAABBGGRR (COLORREF order): the red channel
// is the least-significant byte. For plain syntax colors AA is 0.
//----------------------------------------------------------------------------
#pragma once

#include <cstdint>
#include <string>

namespace doki
{

struct Rgba
{
  uint8_t r = 0, g = 0, b = 0, a = 255;
  bool operator==(const Rgba &o) const
  {
    return r == o.r && g == o.g && b == o.b && a == o.a;
  }
};

// Parse "#rgb", "#rrggbb" or "#rrggbbaa" (leading '#' optional, case
// insensitive). Returns false if the string is not a valid hex color.
bool parse_hex_color(const std::string &s, Rgba *out);

// 0x00BBGGRR : IDA bgcolor_t for a plain (syntax) color, alpha byte = 0.
uint32_t to_bgr24(const Rgba &c);

// 0xAABBGGRR : background color carrying the alpha byte (forced non-zero so
// IDA treats it as a real background rather than a color key).
uint32_t to_bg_with_alpha(const Rgba &c);

// "#rrggbb" (lowercase) - for CSS that wants an opaque hex color.
std::string to_css_hex(const Rgba &c);

// "rgba(r, g, b, a.aaa)" - for CSS that wants alpha (e.g. translucent panes).
std::string to_css_rgba(const Rgba &c);

// Relative luminance per WCAG 2.x, using sRGB channels and ignoring alpha.
double relative_luminance(const Rgba &c);

// WCAG contrast ratio between two opaque colors. Alpha is ignored; callers
// should composite translucent colors first if that distinction matters.
double contrast_ratio(const Rgba &a, const Rgba &b);

// Linear RGB blend. t=0 returns a, t=1 returns b. Alpha is blended too.
Rgba blend(const Rgba &a, const Rgba &b, double t);
Rgba lighten(const Rgba &c, double amount);
Rgba darken(const Rgba &c, double amount);
Rgba with_alpha(const Rgba &c, uint8_t alpha);

enum class ContrastPreference
{
  Auto,
  Lighten,
  Darken,
};

// Return fg adjusted toward white or black until it reaches min_ratio against
// bg, or the closest attempted value if the threshold cannot be met.
Rgba ensure_contrast(const Rgba &fg,
                     const Rgba &bg,
                     double min_ratio,
                     ContrastPreference preference = ContrastPreference::Auto);

} // namespace doki
