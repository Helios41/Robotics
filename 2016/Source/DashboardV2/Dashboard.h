#ifndef DASHBOARD_H_
#define DASHBOARD_H_

/**
TODO:
   -memory arenas
   -platform agnostic
   -properly clamp drawing area
   -transforms
   -font
   -open file dialog
   -fullscreen/boarderless
   -only trigger click on release
   -
   
*/

/**
NOTE:
   -Chrome locks C:/Windows/Fonts/arial.ttf while open, blocking programs from using it
   -
   
*/

union v4
{
   struct
   {
      r32 r;
      r32 g;
      r32 b;
      r32 a;
   };
   
   struct
   {
      r32 x;
      r32 y;
      r32 z;
      r32 w;
   };
   
   r32 vs[4];
};

inline v4 V4(r32 x, r32 y, r32 z, r32 w)
{
   v4 result = {};
   
   result.x = x;
   result.y = y;
   result.z = z;
   result.w = w;
   
   return result;
}

union v2
{
   struct
   {
      r32 u;
      r32 v;
   };
   
   struct
   {
      r32 x;
      r32 y;
   };
   
   r32 vs[2];
};

inline v2 V2(r32 x, r32 y)
{
   v2 result = {};
   
   result.x = x;
   result.y = y;
   
   return result;
}

struct rect2
{
   v2 min;
   v2 max;
};

inline rect2 RectPosSize(r32 x, r32 y, r32 width, r32 height)
{
   rect2 result = {};
   
   result.min.x = x;
   result.min.y = y;
   result.max.x = x + width;
   result.max.y = y + height;
   
   return result;
}

inline rect2 RectMinMax(r32 minx, r32 miny, r32 maxx, r32 maxy)
{
   rect2 result = {};
   
   result.min.x = minx;
   result.min.y = miny;
   result.max.x = maxx;
   result.max.y = maxy;
   
   return result;
}

inline b32 Contains(rect2 rect, v2 vec)
{
   b32 result = (vec.x > rect.min.x) && 
                (vec.x < rect.max.x) &&
                (vec.y > rect.min.y) &&
                (vec.y < rect.max.y);
                
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

struct LoadedFont
{
   LoadedBitmap bitmap;
   v2 offset;
};

enum PageType
{
   PageType_Home,
   PageType_Config,
   PageType_Auto,
   PageType_Fun
};

struct EntireFile
{
   void *contents;
   u64 length;
};

#endif