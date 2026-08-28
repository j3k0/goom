#ifndef _RGBA_H_
#define _RGBA_H_

#include <stdint.h>

namespace gametools {

struct RGBA {
  uint8_t red;
  uint8_t green;
  uint8_t blue;
  uint8_t alpha;
};

struct RGBAf {
  float red;
  float green;
  float blue;
  float alpha;
  RGBAf() : red(1.0f), green(1.0f), blue(1.0f), alpha(1.0f) {}
  RGBAf(float r, float g, float b, float a) : red(r), green(g), blue(b), alpha(a) {}

  bool operator ==(const RGBAf &c) const {
	  return (c.alpha == alpha) && (c.red == red) && (c.green == green) && (c.blue == blue);
  }
  bool operator !=(const RGBAf &c) const {
	  return (c.alpha != alpha) || (c.red != red) || (c.green != green) || (c.blue != blue);
  }
    
    RGBAf withAlpha(float alpha) const {
        RGBAf c = *this;
        c.alpha = alpha;
        return c;
    }
};

struct HSVA {
  float hue;
  float saturation;
  float value;
  uint8_t alpha;
};

}

#endif /* _RGBA_H_ */

