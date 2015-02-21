
#ifndef COLOR_H
#define COLOR_H

#include <SDL.h>
#include "tools.h"

class rgb_ui8 {
    public :
        rgb_ui8(Uint8 r=0, Uint8 g=0, Uint8 b=0) : r(r), g(g), b(b){}
        Uint8 r,g,b;
};
class rgb_f {
    public :
        rgb_f() : r(0), g(0), b(0), s(0) {}
        rgb_f(float r, float g, float b) : r(r), g(g), b(b), s(0) {}
        void add(rgb_f const& c, float a) {
            r += (float)c.r * a;
            g += (float)c.g * a;
            b += (float)c.b * a;
            s+=a;
        }
        void clamp() {
            r = clampIn01(r);
            g = clampIn01(g);
            b = clampIn01(b);
        }
        rgb_f getRGB() {
            if(s == 0)
                return rgb_f(0,0,0);
            else
                return rgb_f(r/s, g/s, b/s);
        };
        rgb_f getRGB(rgb_f const& defaultColor) {
            if(s < 1.0f)
                return rgb_f(r + defaultColor.r*(1.0f-s), g + defaultColor.g*(1.0f-s), b + defaultColor.b*(1.0f-s));
            else
                return rgb_f(r/s, g/s, b/s);
        };
        rgb_f & operator+=(rgb_f const& c) { r+=c.r; g+=c.g; b+=c.b; return *this; }
        rgb_f & operator*=(rgb_f const& c) { r*=c.r; g*=c.g; b*=c.b; return *this; }
        rgb_f & operator*=(float k) { r*=k; g*=k; b*=k; return *this; }
        rgb_f operator*(float const& k) const { return rgb_f(r*k, g*k, b*k); }
        rgb_f operator/(float const& k) const { return rgb_f(r/k, g/k, b/k); }
        rgb_f operator+(float const& k) const { return rgb_f(r+k, g+k, b+k); }
        rgb_f operator-(float const& k) const { return rgb_f(r-k, g-k, b-k); }
        rgb_f operator+(rgb_f const& d) const { return rgb_f(r+d.r, g+d.g, b+d.b); }

        float r,g,b, s;
};

#endif
