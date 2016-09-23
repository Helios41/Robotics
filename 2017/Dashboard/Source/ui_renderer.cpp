#include <gl/gl.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

struct LoadedBitmap
{
   u32 width;
   u32 height;
   u32 gl_texture;
};

struct CharacterBitmap
{
	LoadedBitmap bitmap;
	v2 offset;
};

enum RenderCommandType
{
	RenderCommandType_DrawRectangle,
	RenderCommandType_DrawBitmap
};

struct RenderCommandHeader
{
	RenderCommandType type;
	u32 size;
};

struct DrawRectangleCommand
{
	RenderCommandHeader header;
	v4 color;
	rect2 area;
};

struct DrawBitmapCommand
{
	RenderCommandHeader header;
	v2 pos;
	LoadedBitmap *bitmap;
};

struct RenderContext
{
   u32 render_commands_size;
   u8 *render_commands_at;
   u8 *render_commands;
   
   CharacterBitmap characters[94];
};

LoadedBitmap LoadImage(const char* path)
{
   LoadedBitmap result = {};
   int w, h, comp;
   u8* image = stbi_load(path, &w, &h, &comp, STBI_rgb_alpha);

   if(image)
   {
      result.width = w;
      result.height = h;
      
      glEnable(GL_TEXTURE_2D);
      glGenTextures(1, &result.gl_texture);
		glBindTexture(GL_TEXTURE_2D, result.gl_texture);

      if(comp == 3)
      {
          glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
      }
      else if(comp == 4)
      {
          glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
      }
      
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_ENV_MODE, GL_MODULATE);
      
      glBindTexture(GL_TEXTURE_2D, 0);
      glDisable(GL_TEXTURE_2D);
      stbi_image_free(image);
   }
       
   return result;
}

RenderContext InitRenderContext(MemoryArena *memory, u32 render_commands_size)
{
   RenderContext result = {};
   result.render_commands_size = render_commands_size;
   result.render_commands = (u8 *) PushSize(memory, render_commands_size);
   result.render_commands_at = result.render_commands;
   
   EntireFile font_file = LoadEntireFile("font.ttf");
   stbtt_fontinfo font;
   stbtt_InitFont(&font, (u8 *) font_file.contents, stbtt_GetFontOffsetForIndex((u8 *) font_file.contents, 0));
   
   glEnable(GL_TEXTURE_2D);
   
   for(u32 char_code = 33;
       char_code <= 126;
       char_code++)
   {
      char curr_char = (char) char_code;
      CharacterBitmap *char_bitmap = result.characters + (char_code - 33);

      int width, height, xoffset, yoffset;
      u8 *mono_bitmap = stbtt_GetCodepointBitmap(&font, 0, stbtt_ScaleForPixelHeight(&font, 20.0f),
                                                 curr_char, &width, &height, &xoffset, &yoffset);

      char_bitmap->bitmap.width = (u32) width;
      char_bitmap->bitmap.height = (u32) height;
      char_bitmap->offset.x = (r32) xoffset;
      char_bitmap->offset.y = (r32) yoffset;

      u32 *temp_texture = (u32 *) PushSize(memory, sizeof(u32) * width * height);
      
      for(u32 y = 0; y < height; ++y)
      {
         for(u32 x = 0; x < width; ++x)
         {
            u8 *source = mono_bitmap + (y * width) + x;
            u32 *dest = temp_texture + (y * width) + x;
            
            *dest = (*source << 24) |
                    (*source << 16) |
                    (*source << 8) |
                    (*source << 0);
         }
      }
      
      stbtt_FreeBitmap(mono_bitmap, 0);
      
      glGenTextures(1, &char_bitmap->bitmap.gl_texture);
		glBindTexture(GL_TEXTURE_2D, char_bitmap->bitmap.gl_texture);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height,
                   0, GL_RGBA, GL_UNSIGNED_BYTE, temp_texture);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_ENV_MODE, GL_MODULATE);

      glBindTexture(GL_TEXTURE_2D, 0);
      PopSize(memory, sizeof(u32) * width * height);
   }
   
   glDisable(GL_TEXTURE_2D);
   return result;
}

void Rectangle(RenderContext *context, rect2 area, v4 color)
{
	DrawRectangleCommand *draw_rectangle = (DrawRectangleCommand *) context->render_commands_at;
   
	if((context->render_commands_at + sizeof(DrawRectangleCommand)) < 
      (context->render_commands + context->render_commands_size))
	{
		draw_rectangle->header.size = sizeof(DrawRectangleCommand);
		draw_rectangle->header.type = RenderCommandType_DrawRectangle;
		draw_rectangle->area = area;
		draw_rectangle->color = color;
      
		context->render_commands_at += sizeof(DrawRectangleCommand);
	}
}

void Bitmap(RenderContext *context, LoadedBitmap *bitmap, v2 pos)
{
	if(bitmap)
	{
      DrawBitmapCommand *draw_bitmap = (DrawBitmapCommand *) context->render_commands_at;
   
      if((context->render_commands_at + sizeof(DrawBitmapCommand)) < 
         (context->render_commands + context->render_commands_size))
      {
         draw_bitmap->header.size = sizeof(DrawBitmapCommand);
         draw_bitmap->header.type = RenderCommandType_DrawBitmap;
         draw_bitmap->bitmap = bitmap;
			draw_bitmap->pos = pos;
         
         context->render_commands_at += sizeof(DrawBitmapCommand);
      }
	}
}

void RenderUI(RenderContext *context, v2 window_size)
{
	r32 window_width = window_size.x;
	r32 window_height = window_size.y;
	glViewport(0.0f, 0.0f, window_width, window_height);

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0f, window_width, window_height, 0.0f, -10.0f, 10.0f);

	RenderCommandHeader *render_command = (RenderCommandHeader *) context->render_commands;
	while((u8 *)render_command != context->render_commands_at)
	{  
		switch (render_command->type)
		{
			case RenderCommandType_DrawRectangle:
			{
				DrawRectangleCommand *draw_rectangle = (DrawRectangleCommand *) render_command;
				
				glBegin(GL_QUADS);
				{
					glColor4f(draw_rectangle->color.r, draw_rectangle->color.g, draw_rectangle->color.b, draw_rectangle->color.a);
					glVertex2f(draw_rectangle->area.min.x, draw_rectangle->area.min.y);
					glVertex2f(draw_rectangle->area.max.x, draw_rectangle->area.min.y);
					glVertex2f(draw_rectangle->area.max.x, draw_rectangle->area.max.y);
					glVertex2f(draw_rectangle->area.min.x, draw_rectangle->area.max.y);
				}
				glEnd();
			}
			break;

			case RenderCommandType_DrawBitmap:
			{
				DrawBitmapCommand *draw_bitmap = (DrawBitmapCommand *) render_command;
				rect2 area = RectPosSize(draw_bitmap->pos.x, draw_bitmap->pos.y, draw_bitmap->bitmap->width, draw_bitmap->bitmap->height);

				glBindTexture(GL_TEXTURE_2D, draw_bitmap->bitmap->gl_texture);

				glBegin(GL_QUADS);
				{
					glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	
					glTexCoord2f(0.0f, 0.0f);
					glVertex2f(area.min.x, area.min.y);

					glTexCoord2f(1.0f, 0.0f);
					glVertex2f(area.max.x, area.min.y);

					glTexCoord2f(1.0f, 1.0f);
					glVertex2f(area.max.x, area.max.y);

					glTexCoord2f(0.0f, 1.0f);
					glVertex2f(area.min.x, area.max.y);
				}
				glEnd();

				glBindTexture(GL_TEXTURE_2D, 0);
			}
			break;
		}

		render_command = (RenderCommandHeader *) ((u8 *) render_command + render_command->size);
	}

	context->render_commands_at = context->render_commands;
}

struct InputState
{
   b32 left_down;
   b32 left_up;
   b32 right_down;
   b32 right_up;
   v2 pos;
   b32 char_key_up;
   char key_char;
   b32 key_backspace;
   b32 key_enter;
   b32 key_up;
   b32 key_down;
};

struct layout
{
   v2 origin;
   v2 at;
   b32 row; //NOTE: true for row, false for column
   v2 max;
};

layout Layout(v2 origin, b32 row)
{
   layout result = {};
   
   result.origin = origin;
   result.at = result.origin;
   result.max = result.origin;
   result.row = row;
   
   return result;
}

rect2 Element(layout *ui_layout, v2 size, v2 padding = V2(0, 0))
{
   v2 min = ui_layout->at + padding;
   ui_layout->at = ui_layout->at + (ui_layout->row ? V2(size.x, 0) : V2(0, size.y)) + (ui_layout->row ? V2(padding.x, 0) : V2(0, padding.y)) * 2.0f;
   ui_layout->max = V2(MAX(ui_layout->max.x, (min + size + padding).x), MAX(ui_layout->max.y, (min + size + padding).y));
   
   return RectMinSize(min, size.x, size.y);
}

void Text(RenderContext *context, v2 pos, char *text, u32 scale)
{
	if (!text) return;

	u32 xoffset = 0;
	while (*text)
	{
		if (*text == ' ')
		{
			xoffset += 10;
		}
		else
		{
			CharacterBitmap *char_bitmap = context->characters + ((u32)(*text) - 33);
			Bitmap(context, &char_bitmap->bitmap, V2(pos.x + xoffset + char_bitmap->offset.x, pos.y + char_bitmap->offset.y + 10));
			xoffset += char_bitmap->bitmap.width;
		}
		text++;
	}
}

b32 Button(RenderContext *context, InputState input, rect2 bounds, LoadedBitmap *icon, char *text)
{
   b32 hot = Contains(bounds, input.pos);
   
   if(hot && (input.left_down || input.right_down))
   {
	  Rectangle(context, bounds, V4(1.0f, 0.0f, 0.75f, 1.0f));
   }
   else if(hot)
   {
	  Rectangle(context, bounds, V4(1.0f, 0.0f, 0.0f, 1.0f));
   }
   else
   {
	  Rectangle(context, bounds, V4(0.5f, 0.0f, 0.0f, 1.0f));
   }
   
   v2 bounds_size = RectGetSize(bounds);
   
   if(bounds_size.x == bounds_size.y)
   {
      Bitmap(context, icon, bounds.min);
      Text(context,V2(bounds.min.x + 10, bounds.min.y + 10), text, 20);
   }
   else
   {
      Bitmap(context, icon, V2(bounds.min.x + 5, bounds.min.y));
      Text(context, V2(bounds.min.x + 10, bounds.min.y + 10), text, 20);
   }
   
   return hot && (input.left_up || input.right_up);
}