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
	RenderCommandType_DrawBitmap,
   RenderCommandType_DrawRectangleOutline
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

struct DrawRectangleOutlineCommand
{
	RenderCommandHeader header;
	v4 color;
	rect2 area;
   u32 thickness;
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

DrawRectangleCommand *Rectangle(RenderContext *context, rect2 area, v4 color)
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
      return draw_rectangle;
	}
   
   return NULL;
}

DrawBitmapCommand *Bitmap(RenderContext *context, LoadedBitmap *bitmap, v2 pos)
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
         return draw_bitmap;
      }
	}
   
   return NULL;
}

DrawRectangleOutlineCommand *RectangleOutline(RenderContext *context, rect2 area, v4 color, u32 thickness = 1)
{
	DrawRectangleOutlineCommand *draw_rectangle_outline = (DrawRectangleOutlineCommand *) context->render_commands_at;
   
	if((context->render_commands_at + sizeof(DrawRectangleOutlineCommand)) < 
      (context->render_commands + context->render_commands_size))
	{
		draw_rectangle_outline->header.size = sizeof(DrawRectangleOutlineCommand);
		draw_rectangle_outline->header.type = RenderCommandType_DrawRectangleOutline;
		draw_rectangle_outline->area = area;
		draw_rectangle_outline->color = color;
      draw_rectangle_outline->thickness = thickness;
      
		context->render_commands_at += sizeof(DrawRectangleOutlineCommand);
      return draw_rectangle_outline;
	}
   
   return NULL;
}

//TODO: shader support (eg. guassian blured tiles), Opengl 3 & a layer based sort (radix sort)
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

   //TODO: radix sort 
   
	RenderCommandHeader *render_command = (RenderCommandHeader *) context->render_commands;
	while((u8 *)render_command != context->render_commands_at)
	{  
      //TODO: setup glScissor region

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
				rect2 area = RectMinSize(draw_bitmap->pos, V2(draw_bitmap->bitmap->width, draw_bitmap->bitmap->height));

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
         
         case RenderCommandType_DrawRectangleOutline:
         {
            DrawRectangleOutlineCommand *draw_rectangle_outline = (DrawRectangleOutlineCommand *) render_command;
				
            v2 min = draw_rectangle_outline->area.min;
            v2 max = draw_rectangle_outline->area.max;
            
            glLineWidth(draw_rectangle_outline->thickness);
            
				glBegin(GL_LINES);
				{
					glColor4fv(draw_rectangle_outline->color.vs);
					
               glVertex2f(min.x, min.y);
					glVertex2f(max.x, min.y);
					
               glVertex2f(max.x, min.y);
					glVertex2f(max.x, max.y);
               
               glVertex2f(max.x, max.y);
					glVertex2f(min.x, max.y);
               
               glVertex2f(min.x, max.y);
					glVertex2f(min.x, min.y);
				}
				glEnd();
            
            glLineWidth(1);
         }
         break;
		}

		render_command = (RenderCommandHeader *) ((u8 *) render_command + render_command->size);
	}

	context->render_commands_at = context->render_commands;
}

//TODO: clean this up
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

//TODO: make this more dynamic
struct UIAssets
{
   LoadedBitmap logo;
   LoadedBitmap home;
   LoadedBitmap gear;
   LoadedBitmap eraser;
   LoadedBitmap competition;
};

struct ui_id
{
   u64 a;
   u64 b;
};

b32 operator== (ui_id a, ui_id b)
{
   return (a.a == b.a) && (a.b == b.b);
}

ui_id UIID(u64 a, u64 b)
{
   ui_id result = {a, b};
   return result;
}

#define GEN_UI_ID UIID((u64) __FILE__, (u64) __LINE__)
#define POINTER_UI_ID(pointer) UIID((u64) pointer, (u64) __LINE__)
#define NULL_UI_ID UIID((u64) 0, (u64) 0)

struct UIContext
{
   v2 window_size;
   
   ui_id hot_element;
   ui_id active_element;
   //ui_interaction curr_interaction;
   
   RenderContext *render_context;
   InputState input_state;
   UIAssets *assets;
};

b32 ClickInteraction(UIContext *context, ui_id id, b32 trigger_cond, b32 active_cond, b32 hot_cond)
{
   b32 result = false;
   
   if(context->active_element == id)
   {
      if(trigger_cond)
      {
         if(context->hot_element == id)
         {
            result = true;
         }
         
         context->active_element = NULL_UI_ID;
      }
   }
   
   if(context->hot_element == id)
   {
      if(active_cond)
      {
         context->active_element = id;
      }
      
      if(!hot_cond)
         context->hot_element = NULL_UI_ID;
   }

   if(((context->active_element == NULL_UI_ID) || (context->active_element == id)) && hot_cond) 
      context->hot_element = id;
   
   return result;
}

struct layout
{
   u32 ui_layer;
   u32 stack_layer;
   
   UIContext *context;
   rect2 bounds; 
   //v2 offset; //NOTE: for use in scroll bars
   v2 at;
   
   r32 row_effective_height; // = size.y + topleft_margin.y, accumulate the max, not just the latest element
   r32 abs_row_bottom_margin;
   
   r32 last_row_effective_height;
   r32 last_row_bottom_margin;
   
   r32 last_element_right_margin;
   b32 has_elements;
   
   b32 new_line_issued;
};

layout Layout(rect2 bounds, UIContext *context, u32 stack_layer)
{
   layout result = {};
   
   result.context = context;
   result.bounds = bounds;
   result.at = bounds.min;
   result.ui_layer = 0;
   result.stack_layer = stack_layer;
   
   return result;
}

v2 v2Max(v2 a, v2 b)
{
   return V2(Max(a.x, b.x), Max(a.y, b.y));
}

void NextLine(layout *ui_layout)
{
   ui_layout->last_row_effective_height = ui_layout->row_effective_height;
   ui_layout->last_row_bottom_margin = ui_layout->abs_row_bottom_margin - ui_layout->row_effective_height;
   
   ui_layout->at = V2(ui_layout->bounds.min.x, ui_layout->at.y + ui_layout->row_effective_height);
   ui_layout->new_line_issued = true;
   
   ui_layout->row_effective_height = 0.0f;
   ui_layout->abs_row_bottom_margin = 0.0f;
}

struct get_padding_rect_result
{
   rect2 padding_rect;
   v2 padding_offset;
};

get_padding_rect_result GetPaddingRect(layout ui_layout, v2 element_size, v2 padding_size, v2 margin_size)
{
   b32 is_first_element = !ui_layout.has_elements;
   rect2 margin_bounds = RectMinSize(ui_layout.at, (margin_size * 2.0f) + (padding_size * 2.0f) + element_size);
   v2 padding_offset = V2(margin_size.x, Max(margin_size.y, ui_layout.last_row_bottom_margin));
   
   if(!is_first_element && !ui_layout.new_line_issued)
   {
      padding_offset.x = Max(margin_size.x, ui_layout.last_element_right_margin);
   }
   
   v2 padding_min = ui_layout.at + padding_offset;
   rect2 padding_rect = RectMinSize(padding_min, padding_size.x * 2.0f + element_size.x,
                                                 padding_size.y * 2.0f + element_size.y);
   
   get_padding_rect_result result = {};
   result.padding_rect = padding_rect;
   result.padding_offset = padding_offset;
   return result;
}

struct element
{
   rect2 bounds;
   b32 fit;
};

element Element(layout *ui_layout, v2 element_size, v2 padding_size, v2 margin_size)
{
   get_padding_rect_result padding_rect_result = GetPaddingRect(*ui_layout, element_size, padding_size, margin_size);
   
   v2 padding_offset = padding_rect_result.padding_offset;
   rect2 padding_rect = padding_rect_result.padding_rect;
   v2 padding_rect_size = GetSize(padding_rect);
   
   element result = {};
   result.fit = true;
   
   if(!IsInside(padding_rect, ui_layout->bounds))
   {
      layout temp_layout = *ui_layout;
      NextLine(&temp_layout);
      
      rect2 temp_rect = GetPaddingRect(temp_layout, element_size, padding_size, margin_size).padding_rect;
      
      if(IsInside(temp_rect, ui_layout->bounds))
      {
         *ui_layout = temp_layout;
         
         padding_rect_result = GetPaddingRect(*ui_layout, element_size, padding_size, margin_size);
   
         padding_offset = padding_rect_result.padding_offset;
         padding_rect = padding_rect_result.padding_rect;
         padding_rect_size = GetSize(padding_rect);
      }
      else
      {
         result.fit = false;
      }
   }
   
   ui_layout->at = V2(padding_rect.max.x, ui_layout->at.y);
   ui_layout->new_line_issued = false;
   ui_layout->has_elements = true;
   
   ui_layout->last_element_right_margin = margin_size.x;
   ui_layout->row_effective_height = Max(padding_offset.y + padding_rect_size.y, ui_layout->row_effective_height);
   ui_layout->abs_row_bottom_margin = Max(padding_offset.y + padding_rect_size.y + margin_size.y, ui_layout->abs_row_bottom_margin);
   
   result.bounds = RectMinSize(padding_rect.min + padding_size, element_size);
   
   return result;
}

struct panel
{
   layout lout;
   element elem;
};

panel Panel(layout *parent_layout, v2 element_size, v2 padding_size, v2 margin_size)
{
   panel result = {};
   
   result.elem = Element(parent_layout, element_size, padding_size, margin_size);
   result.lout = Layout(result.elem.bounds, parent_layout->context, parent_layout->stack_layer);
   result.lout.ui_layer = parent_layout->ui_layer + 1;
   
   return result;
}

void Text(RenderContext *context, v2 pos, string text, u32 scale)
{
	if (IsEmpty(text)) return;
   
	u32 xoffset = 0;
	for(u32 i = 0;
       i < text.length;
       i++)
	{
      char c = text.text[i];
      
		if (c == ' ')
		{
			xoffset += 10;
		}
		else
		{
			CharacterBitmap *char_bitmap = context->characters + ((u32)c - 33);
			Bitmap(context, &char_bitmap->bitmap, V2(pos.x + xoffset + char_bitmap->offset.x, pos.y + char_bitmap->offset.y + 10));
			xoffset += char_bitmap->bitmap.width;
		}
	}
}

v2 TextSize(RenderContext *context, string text, u32 scale)
{
	if (IsEmpty(text)) return V2(0, 0);
   
	r32 width = 0.0f;
   r32 highest = -FLTMAX;
   r32 lowest = FLTMAX;
	for(u32 i = 0;
       i < text.length;
       i++)
	{
      char c = text.text[i];
      
		if (c == ' ')
		{
			width += 10;
		}
		else
		{
			CharacterBitmap *char_bitmap = context->characters + ((u32)c - 33);
			width += char_bitmap->bitmap.width;
         highest = Max(highest, char_bitmap->bitmap.height + char_bitmap->offset.y);
         lowest = Min(lowest, char_bitmap->offset.y);
		}
	}
   
   return V2(width, highest - lowest);
}

//TODO: optimize this by caching the text wrapping & segments?
void TextWrapRect(RenderContext *render_context, rect2 bounds, string text)
{
   if(!IsEmpty(text))
   {
      u32 segment_count;
      string *segments = Split(text, ' ', &segment_count);
      
      v2 at = V2(0.0f, 0.0f);
      r32 row_height = 0.0f;
      
      for(u32 i = 0;
          i < segment_count;
          i++)
      {
         string curr_segment = segments[i];
         v2 word_size = TextSize(render_context, curr_segment, 20);
         
         if((bounds.min.x + at.x + word_size.x) > bounds.max.x)
         {
            if((at.y + row_height + word_size.y) > GetSize(bounds).y)
            {
               Rectangle(render_context, RectPosSize(bounds.min + at + V2(5, 5), V2(3, 3)),
                         V4(0.0f, 0.0f, 0.0f, 1.0f));
               Text(render_context, bounds.min + at + V2(5, 5),
                    Literal("..."), 20);
            }
            else
            {
               if(word_size.x > GetSize(bounds).x)
               {
                  Rectangle(render_context, RectPosSize(bounds.min + at + V2(5, 5), V2(3, 3)),
                         V4(0.0f, 0.0f, 0.0f, 1.0f));
                  Text(render_context, bounds.min + at + V2(5, 5),
                       Literal("..."), 20);
               }
               else
               {
                  i--;
               }
               
               at.x = 0.0f;
               at.y += row_height;
               row_height = 0.0f;
            }
         }
         else
         {
            Rectangle(render_context, RectPosSize(bounds.min + at + V2(5, 5), V2(3, 3)),
                      V4(0.0f, 0.0f, 0.0f, 1.0f));
            Text(render_context, bounds.min + at + V2(5, 5),
                 curr_segment, 20);
                 
            row_height = Max(row_height, word_size.y);
            at.x += word_size.x + 10;
         }
      }
      
      free(segments);
   }
}

element Rectangle(layout *ui_layout, v4 color, v2 element_size, v2 padding_size, v2 margin_size)
{
   RenderContext *render_context = ui_layout->context->render_context;
   element rect_element = Element(ui_layout, element_size, padding_size, margin_size);
   
   Rectangle(render_context, rect_element.bounds, color);
   return rect_element;
}

element Bitmap(layout *ui_layout, LoadedBitmap *bitmap, v2 element_size, v2 padding_size, v2 margin_size)
{
   RenderContext *render_context = ui_layout->context->render_context;
   element bmp_element = Element(ui_layout, element_size, padding_size, margin_size);
   
   Bitmap(render_context, bitmap, bmp_element.bounds.min);
   return bmp_element;
}

element Text(layout *ui_layout, string text, u32 scale, v2 margin)
{
   RenderContext *render_context = ui_layout->context->render_context;

   element text_element = Element(ui_layout, TextSize(render_context, text, scale), V2(0, 0), margin);
   Rectangle(render_context, text_element.bounds, V4(0, 0, 0, 0.2f));
   
   Text(render_context, V2(text_element.bounds.min.x, text_element.bounds.min.y + 5), text, scale);
   return text_element;
}

struct button
{
   b32 state;
   element elem;
};

//TODO: clean up text wrap
button _Button(ui_id id, layout *ui_layout, LoadedBitmap *icon, string text, v2 element_size, v2 padding_size, v2 margin_size)
{
   UIContext *context = ui_layout->context;
   RenderContext *render_context = context->render_context;
   InputState input = ui_layout->context->input_state;
   element button_element = Element(ui_layout, element_size, padding_size, margin_size);
   
   b32 state = ClickInteraction(context, id, context->input_state.left_up,
                                context->input_state.left_down, Contains(button_element.bounds, context->input_state.pos));
   
   if(context->active_element == id)
   {
      Rectangle(render_context, button_element.bounds, V4(1.0f, 0.0f, 0.75f, 1.0f));
   }
   else if(context->hot_element == id)
   {
      Rectangle(render_context, button_element.bounds, V4(1.0f, 0.0f, 0.0f, 1.0f));
      //TODO: set tooltip to text
   }
   else
   {
      Rectangle(render_context, button_element.bounds, V4(0.5f, 0.0f, 0.0f, 1.0f));
   }
   
   Bitmap(render_context, icon, button_element.bounds.min);
   TextWrapRect(render_context, button_element.bounds, text);
   
   button result = {};
   result.state = state;
   result.elem = button_element;
   return result;
}

button _Button(ui_id id, layout *ui_layout, LoadedBitmap *icon, string text, b32 triggered, v2 element_size, v2 padding_size, v2 margin_size)
{
   button result = _Button(id, ui_layout, icon, text, element_size, padding_size, margin_size);
   
   v2 bounds_size = GetSize(result.elem.bounds);
   Rectangle(ui_layout->context->render_context, RectMinSize(result.elem.bounds.min, V2(5,
	         (bounds_size.x == bounds_size.y) ? 5 : bounds_size.y)),
	         triggered ? V4(0.0f, 0.0f, 0.0f, 1.0f) : V4(0.0f, 0.0f, 0.0f, 0.0f));

   return result;
}

#define Button(...) _Button(GEN_UI_ID, __VA_ARGS__)

void _TextBox(ui_id id, layout *ui_layout, string *buffer, v2 element_size, v2 padding_size, v2 margin_size)
{
   UIContext *context = ui_layout->context;
   RenderContext *render_context = context->render_context;
   element textbox_element = Element(ui_layout, element_size, padding_size, margin_size);
  
   Rectangle(render_context, textbox_element.bounds, V4(0.5f, 0.0f, 0.0f, 1.0f));
   RectangleOutline(render_context, textbox_element.bounds, V4(0.0f, 0.0f, 0.f, 1.0f));
   
   TextWrapRect(render_context, textbox_element.bounds, *buffer);
}

#define TextBox(...) _TextBox(GEN_UI_ID, __VA_ARGS__)