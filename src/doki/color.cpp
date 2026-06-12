//----------------------------------------------------------------------------
// Doki Theme - color primitives implementation.
//----------------------------------------------------------------------------
#include "doki/color.h"

#include <cctype>
#include <cstdio>

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

} // namespace doki
