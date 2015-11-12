#ifndef DASHBOARD_H_
#define DASHBOARD_H_

/**
TODO:
   -clean up
   -memory arenas
   -platform agnostic
   -
   
*/

typedef union
{
   struct
   {
      r32 r;
      r32 g;
      r32 b;
      r32 a;
   }r;
   
   struct
   {
      r32 x;
      r32 y;
      r32 z;
      r32 w;
   }p;
   
   r32 v[4];
}v4;

inline v4 V4(r32 x, r32 y, r32 z, r32 w)
{
   v4 result = {0};
   
   result.p.x = x;
   result.p.y = y;
   result.p.z = z;
   result.p.w = w;
   
   return result;
}

inline s32 RoundR32ToS32(r32 real)
{
   s32 result = (s32)(real + 0.5f);
   return result;
}

struct LoadedBitmap
{
   u32 width;
   u32 height;
   u32 *pixels;
};

#endif