//----------------------------------------------------------------------------
// Doki Theme - color primitives implementation.
//----------------------------------------------------------------------------
#include "doki/color.h"

#include <cctype>
#include <algorithm>
#include <cstdio>
#include <cmath>

namespace doki
{

static bool hex_nibble(char c, int *out)
{
  if ( c >= '0' && c <= '9' ) { *out = c - '0';        return true; }
  c = (char)std::tolower((unsigned char)c);
  if ( c >= 'a' && c <= 'f' ) { *out = 10 + (c - 'a'); return true; }
  return false;
}

static bool hex_byte(const char *p, uint8_t *out)
{
  int hi, lo;
  if ( !hex_nibble(p[0], &hi) || !hex_nibble(p[1], &lo) )
    return false;
  *out = (uint8_t)((hi << 4) | lo);
  return true;
}

bool parse_hex_color(const std::string &in, Rgba *out)
{
  std::string s = in;
  if ( !s.empty() && s[0] == '#' )
    s.erase(s.begin());

  Rgba c;
  if ( s.size() == 3 ) // #rgb -> #rrggbb
  {
    int r, g, b;
    if ( !hex_nibble(s[0], &r) || !hex_nibble(s[1], &g) || !hex_nibble(s[2], &b) )
      return false;
    c.r = (uint8_t)(r * 17); // 0xF -> 0xFF
    c.g = (uint8_t)(g * 17);
    c.b = (uint8_t)(b * 17);
    c.a = 255;
  }
  else if ( s.size() == 6 || s.size() == 8 )
  {
    if ( !hex_byte(&s[0], &c.r)
      || !hex_byte(&s[2], &c.g)
      || !hex_byte(&s[4], &c.b) )
      return false;
    c.a = 255;
    if ( s.size() == 8 && !hex_byte(&s[6], &c.a) )
      return false;
  }
  else
  {
    return false;
  }

  *out = c;
  return true;
}

uint32_t to_bgr24(const Rgba &c)
{
  // red in the least-significant byte (0x00BBGGRR)
  return (uint32_t)c.r
       | ((uint32_t)c.g << 8)
       | ((uint32_t)c.b << 16);
}

uint32_t to_bg_with_alpha(const Rgba &c)
{
  uint8_t a = c.a == 0 ? 0xFF : c.a; // never 0: that would mean "color key"
  return to_bgr24(c) | ((uint32_t)a << 24);
}

std::string to_css_hex(const Rgba &c)
{
  char buf[8];
  std::snprintf(buf, sizeof(buf), "#%02x%02x%02x", c.r, c.g, c.b);
  return buf;
}

std::string to_css_rgba(const Rgba &c)
{
  char buf[40];
  std::snprintf(buf, sizeof(buf), "rgba(%u, %u, %u, %.3f)",
                c.r, c.g, c.b, c.a / 255.0);
  return buf;
}

static double srgb_to_linear(uint8_t channel)
{
  const double c = channel / 255.0;
  return c <= 0.04045 ? c / 12.92 : std::pow((c + 0.055) / 1.055, 2.4);
}

double relative_luminance(const Rgba &c)
{
  return 0.2126 * srgb_to_linear(c.r)
       + 0.7152 * srgb_to_linear(c.g)
       + 0.0722 * srgb_to_linear(c.b);
}

double contrast_ratio(const Rgba &a, const Rgba &b)
{
  const double la = relative_luminance(a);
  const double lb = relative_luminance(b);
  const double hi = std::max(la, lb);
  const double lo = std::min(la, lb);
  return (hi + 0.05) / (lo + 0.05);
}

static uint8_t mix_channel(uint8_t a, uint8_t b, double t)
{
  t = std::max(0.0, std::min(1.0, t));
  return (uint8_t)std::lround(a + (b - a) * t);
}

Rgba blend(const Rgba &a, const Rgba &b, double t)
{
  return Rgba{mix_channel(a.r, b.r, t),
              mix_channel(a.g, b.g, t),
              mix_channel(a.b, b.b, t),
              mix_channel(a.a, b.a, t)};
}

Rgba lighten(const Rgba &c, double amount)
{
  return blend(c, Rgba{255,255,255,c.a}, amount);
}

Rgba darken(const Rgba &c, double amount)
{
  return blend(c, Rgba{0,0,0,c.a}, amount);
}

Rgba with_alpha(const Rgba &c, uint8_t alpha)
{
  Rgba out = c;
  out.a = alpha;
  return out;
}

Rgba ensure_contrast(const Rgba &fg,
                     const Rgba &bg,
                     double min_ratio,
                     ContrastPreference preference)
{
  if ( contrast_ratio(fg, bg) >= min_ratio )
    return fg;

  auto best_toward = [&](const Rgba &target) -> Rgba
  {
    Rgba best = fg;
    double best_ratio = contrast_ratio(fg, bg);
    for ( int i = 1; i <= 20; ++i )
    {
      Rgba candidate = blend(fg, target, i / 20.0);
      const double ratio = contrast_ratio(candidate, bg);
      if ( ratio > best_ratio )
      {
        best = candidate;
        best_ratio = ratio;
      }
      if ( ratio >= min_ratio )
        return candidate;
    }
    return best;
  };

  if ( preference == ContrastPreference::Lighten )
    return best_toward(Rgba{255,255,255,fg.a});
  if ( preference == ContrastPreference::Darken )
    return best_toward(Rgba{0,0,0,fg.a});

  Rgba light = best_toward(Rgba{255,255,255,fg.a});
  Rgba dark = best_toward(Rgba{0,0,0,fg.a});
  return contrast_ratio(light, bg) >= contrast_ratio(dark, bg) ? light : dark;
}

} // namespace doki
