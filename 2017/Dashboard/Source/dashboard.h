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
#define Min(a, b) (((a) > (b)) ? (b) : (a))
#define Max(a, b) (((a) < (b)) ? (b) : (a))
#define FLTMAX 3.402823e+38

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
//      ATM its only used to deallocate the temp memory when loading the fonts
//      the arena system needs a proper temp_memory solution
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

b32 IsEmpty(string text)
{
   return (text.length == 0) || (text.text == NULL);
}

string EmptyString()
{
   string result = {NULL, 0};
   return result;
}

void CopyTo(string source, string dest)
{
   for(u32 i = 0;
       i < Min(source.length, dest.length);
       i++)
   {
      dest.text[i] = source.text[i];
   }
}

void Clear(string str)
{
   for(u32 i = 0;
       i < str.length;
       i++)
   {
      str.text[i] = ' ';
   }
}

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

/*
string Concat(string *strings, u32 count, char concat_char)
{
   u32 result_size = count - 1;
   
   for(u32 i = 0;
       i < count;
       i++)
   {
      result_size += strings[i].length;
   }
   
   string result = String((char *) malloc(result_size * sizeof(char)), result_size);
   
   u32 index = 0;
   for(u32 i = 0;
       i < count;
       i++)
   {
      string *curr_string = strings + i;
      
      for(u32 c = 0;
          c < curr_string->length;
          c++)
      {
         result.text[index] = curr_string->text[c];
         index++;
      }
      
      if(i != (count - 1))
      {
         result.text[index] = concat_char;
         index++;
      }
   }
   
   return result;
}
*/

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

r32 ToR32(string str)
{
   string buffer = String((char *) malloc(21 * sizeof(char)), 20);
   CopyTo(str, buffer);
   buffer.text[buffer.length] = '\0';
   r32 result = atof(buffer.text);
   free(buffer.text);
   return result;
}

r32 Clamp(r32 min, r32 max, r32 in)
{
   return Min(Max(min, in), max);
}

struct ticket_mutex
{
   s32 handout;
   s32 serving;
};

void BeginTicketMutex(ticket_mutex *mutex)
{
   s32 ticket = InterlockedIncrement((long volatile *) &mutex->handout) - 1;
   while(ticket != mutex->serving);
}

void EndTicketMutex(ticket_mutex *mutex)
{
   InterlockedIncrement((long volatile *) &mutex->serving);
}

#endif