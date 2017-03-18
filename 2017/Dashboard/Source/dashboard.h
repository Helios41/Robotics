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
#define InvalidCodePath Assert(false)
#define ArrayCount(array) (sizeof(array) / sizeof(array[0]))
#define Min(a, b) (((a) > (b)) ? (b) : (a))
#define Max(a, b) (((a) < (b)) ? (b) : (a))
#define Clamp(min, max, in) Min(Max(min, in), max)
#define FLTMAX 3.402823e+38
#define MIN_U64 0
#define MAX_U64 0xFFFFFFFFFFFFFFFF

//TODO: use 3 font sizes for entire ui
//		calculate default sizes using monitor's ppi

//TODO: make the notification system cache its messages so we dont have
//      to worry about the lifetime of the strings we pass   

/**
TODO:
	-camera
	-connect over field
	-autonomous recording times
	-vision tracking
   -opengl 3 & shaders
   -animations for ui
   -only draw ui frame when nessisary (animation, input, other event, etc...) & only
    redraw the required section of the screen
   -port backend to SDL
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

union v3
{
   struct
   {
      r32 x;
      r32 y;
      r32 z;
   };
   
   struct
   {
      r32 r;
      r32 g;
      r32 b;
   };
   
   struct
   {
      r32 h;
      r32 s;
      r32 v;
   };
   
   r32 vs[3];
};

inline v3 V3(r32 x, r32 y, r32 z)
{
   v3 result = {};
   
   result.x = x;
   result.y = y;
   result.z = z;
   
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

#include "math.h"

r32 Length(v2 a)
{
   return sqrtf(a.x * a.x + a.y * a.y);
}

r32 Distance(v2 a, v2 b)
{
   return Length(a - b);
}

struct rect2
{
   v2 min;
   v2 max;
};

inline rect2 RectPosSize(v2 pos, v2 size)
{
   rect2 result = {};
   
   result.min = pos - size * 0.5f;
   result.max = pos + size * 0.5f;
   
   return result;
}

inline rect2 RectPosSize(r32 x, r32 y, r32 width, r32 height)
{
   return RectPosSize(V2(x, y), V2(width, height));
}

inline rect2 RectMinSize(v2 min, r32 width, r32 height)
{
   rect2 result = {};
   
   result.min = min;
   result.max = min + V2(width, height);
   
   return result;
}

inline rect2 RectMinSize(v2 min, v2 size)
{
   return RectMinSize(min, size.x, size.y);
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

inline b32 Equal(r32 a, r32 b)
{
   r32 difference = (a - b);
   difference = (difference < 0) ? -difference : difference;
   return difference < 0.1f;
}

inline r32 Abs(r32 real)
{
   return (real < 0) ? -real : real;
}

inline b32 GreaterOrEqual(r32 a, r32 b)
{
   return (a > b) || Equal(a, b);
}

inline b32 LessOrEqual(r32 a, r32 b)
{
   return (a < b) || Equal(a, b);
}

inline b32 Contains(rect2 rect, v2 vec)
{
   b32 result = GreaterOrEqual(vec.x, rect.min.x) && 
                LessOrEqual(vec.x, rect.max.x) &&
                GreaterOrEqual(vec.y, rect.min.y) &&
                LessOrEqual(vec.y, rect.max.y);
                
   return result;
}

inline b32 IsInside(rect2 a, rect2 b)
{
   return Contains(b, a.min) && Contains(b, a.max);
}

inline v2 GetSize(rect2 rect)
{
   return V2(rect.max.x - rect.min.x, rect.max.y - rect.min.y);
}

inline v2 GetCenter(rect2 rect)
{
   return V2(rect.min.x + GetSize(rect).x / 2,
             rect.min.y + GetSize(rect).y / 2);
}

inline b32 Intersects(rect2 a, rect2 b)
{
   return Contains(RectPosSize(GetCenter(a), GetSize(a) + GetSize(b)), GetCenter(b));
}

inline r32 GetArea(rect2 a)
{
   return GetSize(a).x * GetSize(a).y;
}

inline s32 RoundR32ToS32(r32 real)
{
   s32 result = (s32)(real + 0.5f);
   return result;
}

void Clear(void *memory, size_t size)
{
   for(u32 i = 0; i < size; i++) ((u8 *)memory)[i] = 0;
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

struct TemporaryMemoryArena
{
   MemoryArena *source;
   size_t used;
};

TemporaryMemoryArena BeginTemporaryMemory(MemoryArena *arena)
{
   TemporaryMemoryArena result = {};
   result.source = arena;
   return result;
}

void EndTemporaryMemory(TemporaryMemoryArena temp_arena)
{
   temp_arena.source->used -= temp_arena.used;
}

enum ArenaFlags
{
   Arena_Clear = (1 << 0)
};

void *PushSize(MemoryArena *arena, size_t size, u32 flags = 0)
{
   Assert((arena->used + size) < arena->size);
   void *result = (u8 *)arena->memory + arena->used;
   arena->used += size;
   
   if(flags & Arena_Clear)
   {
      Clear(result, size);
   }
   
   return result;
}

void *PushSize(TemporaryMemoryArena *arena, size_t size, u32 flags = 0)
{
   arena->used += size;
   return PushSize(arena->source, size, flags);
}

#define PushStruct(arena, struct, ...) (struct *) PushSize(arena, sizeof(struct), ##__VA_ARGS__)
#define PushArray(arena, count, struct, ...) (struct *) PushSize(arena, count * sizeof(struct), ##__VA_ARGS__)

struct EntireFile
{
   void *contents;
   u64 length;
};

EntireFile LoadEntireFile(const char* path);
void SetFullscreen(b32 state);

u32 StringLength(char *str)
{
   if(!str) return 0;
   
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

struct string
{
   char *text;
   u32 length;
};

string Literal(char *text)
{
   string result = {text, StringLength(text)};
   return result;
}

string String(char *text, u32 length)
{
   string result = {text, length};
   return result;
}

string ToString(string buffer, r32 flt)
{
   _snprintf(buffer.text, buffer.length, "%f", flt);
   for(u32 i = 0; i < buffer.length; i++) if(buffer.text[i] == '\0') buffer.text[i] = ' ';
   return buffer;
}

string ToString(r32 flt, TemporaryMemoryArena *temp_memory)
{
   string result = String(PushArray(temp_memory, 12, char, Arena_Clear), 12);
   _snprintf(result.text, result.length, "%f", flt);
   //for(u32 i = 0; i < result.length; i++) if(result.text[i] == '\0') result.text[i] = ' ';
   
   for(u32 i = 0; i < result.length; i++)
   {
      if(result.text[i] == '\0')
      {
         result.length = i;
         break;
      }
   }
   
   return result;
}

string ToString(u32 flt, TemporaryMemoryArena *temp_memory)
{
   string result = String(PushArray(temp_memory, 12, char, Arena_Clear), 12);
   _snprintf(result.text, result.length, "%u", flt);
   //for(u32 i = 0; i < result.length; i++) if(result.text[i] == '\0') result.text[i] = ' ';
   
   for(u32 i = 0; i < result.length; i++)
   {
      if(result.text[i] == '\0')
      {
         result.length = i;
         break;
      }
   }
   
   return result;
}

b32 IsEmpty(string text)
{
   return (text.length == 0) || (text.text == NULL);
}

string EmptyString()
{
   string result = {NULL, 0};
   return result;
}

string CopyTo(string source, string dest)
{
   for(u32 i = 0;
       i < Min(source.length, dest.length);
       i++)
   {
      dest.text[i] = source.text[i];
   }
   
   return dest;
}

string Clear(string str)
{
   for(u32 i = 0;
       i < str.length;
       i++)
   {
      str.text[i] = ' ';
   }
   
   return str;
}

#if 0
//TODO: make this more robust
string *Split(string input, char split_char, u32 *array_length)
{
   {
      u32 i = 0;
      while((i < input.length) && 
            (input.text[i] == split_char))
      {
         input.text++;
         input.length--;
         i++;
      }
   }
   
   {
      u32 i = input.length - 1;
      while((i > 0) && 
            (input.text[i] == split_char))
      {
         input.length--;
         i--;
      }
   }
   
   *array_length = 1;
   
   for(u32 i = 0;
       i < input.length;
       i++)
   {
      if((input.text[i] == split_char) &&
         ((input.text[i - 1] != split_char) || (input.text[i + 1] != split_char)))
         (*array_length)++;
   }
   
   string *result = (string *) malloc(sizeof(string) * (*array_length));
   
   u32 index = 0;
   for(u32 i = 0;
       i < input.length;
       i++)
   {
      if(input.text[i] == split_char)
      {
         if(index == 0)
         {
            result[index] = String(input.text, i);
         }
         else
         {
            string last_segment = result[index - 1];
            result[index] = String(last_segment.text + last_segment.length + 1,
                                   i - (u32)((last_segment.text + last_segment.length) - input.text) - 1);
         }
         
         index++;
      }
   }
   
   if((index == 0) && (*array_length == 1))
   {
      result[0] = input;
   }
   else
   {
      string last_segment = result[index - 1];
      result[index] = String(last_segment.text + last_segment.length + 1, (u32)((input.text + input.length - 1) - (last_segment.text + last_segment.length)));
   }
   
   return result;
}
#endif

string Concat(string s0, string s1, TemporaryMemoryArena *temp_memory)
{
   u32 result_length = s0.length + s1.length;
   string result = String(PushArray(temp_memory, result_length, char, Arena_Clear), result_length);
   
   u32 i = 0;
   for(u32 j = 0; j < s0.length; j++) result.text[i++] = s0.text[j];
   for(u32 j = 0; j < s1.length; j++) result.text[i++] = s1.text[j];
   
   return result;
}

string Concat(string s0, string s1, string s2, TemporaryMemoryArena *temp_memory)
{
   u32 result_length = s0.length + s1.length + s2.length;
   string result = String(PushArray(temp_memory, result_length, char, Arena_Clear), result_length);
   
   u32 i = 0;
   for(u32 j = 0; j < s0.length; j++) result.text[i++] = s0.text[j];
   for(u32 j = 0; j < s1.length; j++) result.text[i++] = s1.text[j];
   for(u32 j = 0; j < s2.length; j++) result.text[i++] = s2.text[j];
   
   return result;
}

string Concat(string s0, string s1, string s2, string s3, TemporaryMemoryArena *temp_memory)
{
   u32 result_length = s0.length + s1.length + s2.length + s3.length;
   string result = String(PushArray(temp_memory, result_length, char, Arena_Clear), result_length);
   
   u32 i = 0;
   for(u32 j = 0; j < s0.length; j++) result.text[i++] = s0.text[j];
   for(u32 j = 0; j < s1.length; j++) result.text[i++] = s1.text[j];
   for(u32 j = 0; j < s2.length; j++) result.text[i++] = s2.text[j];
   for(u32 j = 0; j < s3.length; j++) result.text[i++] = s3.text[j];
   
   return result;
}

string Concat(string s0, string s1, string s2, string s3, string s4, TemporaryMemoryArena *temp_memory)
{
   u32 result_length = s0.length + s1.length + s2.length + s3.length + s4.length;
   string result = String(PushArray(temp_memory, result_length, char, Arena_Clear), result_length);
   
   u32 i = 0;
   for(u32 j = 0; j < s0.length; j++) result.text[i++] = s0.text[j];
   for(u32 j = 0; j < s1.length; j++) result.text[i++] = s1.text[j];
   for(u32 j = 0; j < s2.length; j++) result.text[i++] = s2.text[j];
   for(u32 j = 0; j < s3.length; j++) result.text[i++] = s3.text[j];
   for(u32 j = 0; j < s4.length; j++) result.text[i++] = s4.text[j];
   
   return result;
}

string Concat(string s0, string s1, string s2, string s3, string s4, string s5, string s6,
              TemporaryMemoryArena *temp_memory)
{
   u32 result_length = s0.length + s1.length + s2.length + s3.length + s4.length +
                       s5.length + s6.length;
   string result = String(PushArray(temp_memory, result_length, char, Arena_Clear), result_length);
   
   u32 i = 0;
   for(u32 j = 0; j < s0.length; j++) result.text[i++] = s0.text[j];
   for(u32 j = 0; j < s1.length; j++) result.text[i++] = s1.text[j];
   for(u32 j = 0; j < s2.length; j++) result.text[i++] = s2.text[j];
   for(u32 j = 0; j < s3.length; j++) result.text[i++] = s3.text[j];
   for(u32 j = 0; j < s4.length; j++) result.text[i++] = s4.text[j];
   for(u32 j = 0; j < s5.length; j++) result.text[i++] = s5.text[j];
   for(u32 j = 0; j < s6.length; j++) result.text[i++] = s6.text[j];
   
   return result;
}

u32 CountChar(string str, char c)
{
   u32 result = 0;
   for(u32 i = 0;
       i < str.length;
       i++)
   {
      if(str.text[i] == c)
      {
         result++;
      }
   }
   return result;
}

u32 IndexOfNth(string str, u32 count, char c)
{
   u32 result = 0;
   for(u32 i = 0;
       i < str.length;
       i++)
   {
      if(str.text[i] == c)
      {
         if(count == 0)
         {
            result = i;
            break;
         }
         
         count--;
      }
   }
   return result;
}

u32 IndexOfFirst(string str, char c)
{
   //NOTE: for some reason the c param was being passed as 'c'
   //      guess i was having a bad day when i wrote this ¯\_(ツ)_/¯
   return IndexOfNth(str, 0, c);
}

void InsertAt(string buffer, string str, u32 at)
{
   for(u32 i = buffer.length - 1;
       i > (at + str.length - 1);
       i--)
   {
      buffer.text[i] = buffer.text[i - str.length];
   }
   
   for(u32 i = 0;
       i < str.length;
       i++)
   {
      buffer.text[at + i] = str.text[i];
   }
}

r32 ToR32(string str)
{
   string buffer = String((char *) malloc(21 * sizeof(char)), 20);
   CopyTo(str, buffer);
   buffer.text[buffer.length] = '\0';
   r32 result = atof(buffer.text);
   free(buffer.text);
   return result;
}

#endif