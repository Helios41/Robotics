#ifndef DASHBOARD_H_
#define DASHBOARD_H_

#include <stdint.h>

typedef int8_t s8;
typedef uint8_t u8;
typedef int16_t s16;
typedef uint16_t u16;
typedef int32_t s32;
typedef uint32_t u32;
typedef int64_t s64;
typedef uint64_t u64;
typedef float r32;
typedef double r64;
typedef uint32_t b32;

#define Kilobyte(BYTE) BYTE * 1024
#define Megabyte(BYTE) Kilobyte(BYTE) * 1024
#define Gigabyte(BYTE) Megabyte(BYTE) * 1024
#define Terabyte(BYTE) Gigabyte(BYTE) * 1024
#define Assert(condition) if(!(condition)){*(u8 *)0 = 0;}
#define ArrayCount(array) (sizeof(array) / sizeof(array[0]))
#define MIN(a, b) (((a) > (b)) ? (b) : (a))
#define MAX(a, b) (((a) < (b)) ? (b) : (a))

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

v2 operator+ (v2 a, v2 b)
{
	v2 output = {};
	output.x = a.x + b.x;
	output.y = a.y + b.y;
	return output;
}

v2 operator- (v2 a, v2 b)
{
	v2 output = {};
	output.x = a.x - b.x;
	output.y = a.y - b.y;
	return output;
}

v2 operator* (v2 v, r32 s)
{
	v2 output = {};
	output.x = v.x * s;
	output.y = v.y * s;
	return output;
}

v2 operator/ (v2 v, r32 s)
{
	v2 output = {};
	output.x = v.x / s;
	output.y = v.y / s;
	return output;
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

inline rect2 RectMinSize(v2 min, r32 width, r32 height)
{
   rect2 result = {};
   
   result.min = min;
   result.max = min + V2(width, height);
   
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

inline v2 RectGetSize(rect2 rect)
{
   return V2(rect.max.x - rect.min.x, rect.max.y - rect.min.y);
}

inline s32 RoundR32ToS32(r32 real)
{
   s32 result = (s32)(real + 0.5f);
   return result;
}

struct MemoryArena
{
   size_t size;
   size_t used;
   void *memory;
};

void InitMemoryArena(MemoryArena *arena, void *memory, size_t size)
{
   arena->size = size;
   arena->used = 0;
   arena->memory = memory;
}

void *PushSize(MemoryArena *arena, size_t size)
{
   Assert((arena->used + size) < arena->size);
   void *result = (u8 *)arena->memory + arena->used;
   arena->used += size;
   return result;
}

//TODO: this needs to be removed ASAP, its super prone to error
void PopSize(MemoryArena *arena, size_t size)
{
   arena->used -= size;
}

struct EntireFile
{
   void *contents;
   u64 length;
};

EntireFile LoadEntireFile(const char* path);

u32 StringLength(char *str)
{
   u32 len = 0;
   while(*str)
   {
      len++;
      str++;
   }
   return len;
}

#include <stdio.h>
#include <stdlib.h>

char *R32ToString(r32 value, char *str)
{
   sprintf(str, "%f", value);
   return str;
}

char *R64ToString(r64 value, char *str)
{
   sprintf(str, "%lf", value);
   return str;
}

char *U32ToString(u32 value, char *str)
{
   sprintf(str, "%u", value);
   return str;
}

r32 StringToR32(char *str)
{
	return atof(str);
}

void StringCopy(char *src, char *dest)
{
	while (*src)
	{
		*dest = *src;
		dest++;
		src++;
	}
	*dest = '\0';
}

char *ConcatStrings(char *str1, char *str2, char *str)
{
   u32 i = 0;
   while(*str1)
   {
      str[i++] = *str1;
      str1++;
   }
   
   while(*str2)
   {
      str[i++] = *str2;
      str2++;
   }

   str[i] = '\0';
   return str;
}

char *ConcatStrings(char *str1, char *str2, char *str3, char *str)
{
   u32 i = 0;
   while(*str1)
   {
      str[i++] = *str1;
      str1++;
   }
   
   while(*str2)
   {
      str[i++] = *str2;
      str2++;
   }

   while(*str3)
   {
      str[i++] = *str3;
      str3++;
   }
   
   str[i] = '\0';
   return str;
}

char *ConcatStrings(char *str1, char *str2, char *str3, char *str4, char *str)
{
	u32 i = 0;
	while (*str1)
	{
		str[i++] = *str1;
		str1++;
	}

	while (*str2)
	{
		str[i++] = *str2;
		str2++;
	}

	while (*str3)
	{
		str[i++] = *str3;
		str3++;
	}

	while (*str4)
	{
		str[i++] = *str4;
		str4++;
	}

	str[i] = '\0';
	return str;
}

char *ConcatStrings(char *str1, char *str2, char *str3, char *str4, char *str5, char *str)
{
	u32 i = 0;
	while (*str1)
	{
		str[i++] = *str1;
		str1++;
	}

	while (*str2)
	{
		str[i++] = *str2;
		str2++;
	}

	while (*str3)
	{
		str[i++] = *str3;
		str3++;
	}

	while (*str4)
	{
		str[i++] = *str4;
		str4++;
	}

	while (*str5)
	{
		str[i++] = *str5;
		str5++;
	}

	str[i] = '\0';
	return str;
}

#endif