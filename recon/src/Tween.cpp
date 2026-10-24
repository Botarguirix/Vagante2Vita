// Tween::val @007df280: formulas recovered from SSE instructions, not lost x87 pseudocode.
#include "recon_all.h"
#include <algorithm>
#include <cmath>
#define VLOG(level) if ((level) > Log::reportingLevel()) ; else Log().get(level)
Tween::Tween(TweenType kind,float begin,float change,float duration)
    : type(kind),time(0),b(begin),c(change),d(duration),endWaitTime(0),pauseTime(0) {}
float Tween::step(float delta) {
    if (pauseTime<=0.f) time+=delta;
    else { pauseTime-=delta;if(pauseTime<0.f) time-=pauseTime; }
    return val();
}
float Tween::val() {
    const float t=std::min(time,d);
    const float q=(1.f/d)*t;
    switch(type) {
    case TWEEN_LINEAR: return q*c+b;
    case TWEEN_QUADRATIC_IN: return (q*q)*c+b;
    case TWEEN_QUADRATIC_IN_OUT: {
        const float twice=q+q;
        if(twice<1.f) return (twice*q)*c+b;
        const float u=((twice-1.f)*(twice-3.f)-1.f)*.5f;
        return b-u*c;
    }
    case TWEEN_CUBIC_IN: return (q*c)*(q*q)+b;
    case TWEEN_CUBIC_OUT: { const float u=q-1.f;return ((u*u)*u+1.f)*c+b; }
    case TWEEN_CUBIC_IN_OUT: {
        const float twice=q+q;
        if(twice<1.f) return ((q*twice)*(twice*c))+b;
        const float u=twice-2.f;return ((u*u)*(u*.5f)+1.f)*c+b;
    }
    case TWEEN_QUARTIC_IN: { const float square=q*q;return (square*square)*c+b; }
    case TWEEN_CIRCULAR_IN: return (1.f-std::sqrt(1.f-q*q))*c+b;
    case TWEEN_CIRCULAR_OUT: { const float u=q-1.f;return std::sqrt(1.f-u*u)*c+b; }
    case TWEEN_EASE_IN_OUT: return b-((std::cos((t*3.1415927410125732f)*(1.f/d))-1.f)*c)*.5f;
    case TWEEN_EASE_BOUNCE:
        if(q<0.3636363744735718f) return (c*q)*(7.5625f*q)+b;
        if(q<0.7272727489471436f) return (std::pow(q-0.5454545617103577f,2.f)*7.5625f+.75f)*c+b;
        if(q<0.9090909361839294f) return (std::pow(q-0.8181818127632141f,2.f)*7.5625f+.9375f)*c+b;
        return (std::pow(q-0.9545454382896423f,2.f)*7.5625f+.984375f)*c+b;
    case TWEEN_EXP_IN_OUT:
        if(q<=0.f) return b;
        if(q>=1.f) return c+b;
        if(q+q<1.f) return (std::pow(1024.f,(q+q)-1.f)*.5f)*c+b;
        return c*(1.f-std::pow(2.f,((q+q)-1.f)*-10.f)*.5f)+b;
    default: VLOG(LOG_ERROR)<<"Undefined tween type: "<<type;return 0.f;
    }
}

void Tween::setEndWaitTime(float value) { endWaitTime=value; }
