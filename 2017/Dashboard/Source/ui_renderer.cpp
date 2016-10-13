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

//TODO: remove this, just revert to a root_layout
struct UIContext
{
   v2 window_size;
   
   r32 top_bar_height;
   r32 left_bar_width;
   r32 right_bar_width;
   
   RenderContext *render_context;
   InputState input_state;
   UIAssets *assets;
};

struct layout
{
   UIContext *context;
   rect2 bounds; 
   v2 at;
   
   r32 row_effective_height; // = size.y + topleft_margin.y, accumulate the max, not just the latest element
   r32 abs_row_bottom_margin;
   
   r32 last_row_effective_height;
   r32 last_row_bottom_margin;
   
   r32 last_element_right_margin;
   b32 has_elements;
   
   b32 new_line_issued;
};

layout Layout(rect2 bounds, UIContext *context)
{
   layout result = {};
   
   result.context = context;
   result.bounds = bounds;
   result.at = bounds.min;
   
   return result;
}

//TODO: remove these
layout TopBar(UIContext *context, r32 height)
{
   layout result = Layout(RectMinMax(0, 0, context->window_size.x, height), context);
   context->top_bar_height = height;
   return result;
}

layout LeftBar(UIContext *context, r32 width)
{
   layout result = Layout(RectMinMax(0, context->top_bar_height,
                                     width, context->window_size.y),
                                     context);
   context->left_bar_width = width;
   return result;
}

layout RightBar(UIContext *context, r32 width)
{
   layout result = Layout(RectMinMax(context->window_size.x - width, context->top_bar_height,
                                     context->window_size.x, context->window_size.y),
                                     context);
   context->right_bar_width = width;
   return result;
}

layout CenterArea(UIContext *context)
{
   layout result = Layout(RectMinMax(context->left_bar_width, context->top_bar_height,
                                     context->window_size.x - context->right_bar_width, context->window_size.y),
                                     context);
   return result;
}
//end remove

v2 v2Max(v2 a, v2 b)
{
   return V2(Max(a.x, b.x), Max(a.y, b.y));
}

//TODO: non uniform margins, one side bigger than the other
//TODO: minimum clamps, not just maximum
struct element_size
{
   v2 clamp; 
   v2 percent;
};

element_size ElementSizePixel(v2 pixel_size)
{
   element_size result = {};
   result.percent = V2(1.0f, 1.0f);
   result.clamp = pixel_size;
   return result;
}

element_size ElementSizePercent(v2 percent_size)
{
   element_size result = {};
   result.percent = percent_size / 100.0f;
   result.clamp = V2(FLTMAX, FLTMAX);
   return result;
}

element_size ElementSize(v2 percent_size, v2 pixel_clamp)
{
   element_size result = {};
   result.percent = percent_size / 100.0f;
   result.clamp = pixel_clamp;
   return result;
}

v2 ToSize(v2 layout_size, element_size spec)
{
   v2 size = V2(layout_size.x * spec.percent.x, layout_size.y * spec.percent.y);
   size = V2((size.x > spec.clamp.x) ? spec.clamp.x : size.x,
             (size.y > spec.clamp.y) ? spec.clamp.y : size.y);
   return size;
}

struct element_definition
{
   element_size element_spec;
   element_size padding_spec;
   element_size margin_spec;
};

element_definition ElementDefPixel(v2 margin, v2 padding, v2 element)
{
   element_definition result = {};
   
   result.margin_spec = ElementSizePixel((margin * 2.0f) + (padding * 2.0f) + element);
   result.padding_spec = ElementSizePixel((padding * 2.0f) + element);
   result.element_spec = ElementSizePixel(element);
   
   return result;
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

//TODO: scroll bars for layouts that are too big (vertical scroll bars only!!!)
rect2 Element(layout *ui_layout, element_size element_spec, element_size padding_spec, element_size margin_spec)
{  
   b32 is_first_element = !ui_layout->has_elements;
   
   rect2 margin_bounds = RectMinSize(ui_layout->at, ToSize(GetSize(ui_layout->bounds), margin_spec));
   v2 padding_size = ToSize(GetSize(margin_bounds), padding_spec);
   rect2 padding_bounds = RectMinSize(margin_bounds.min + (GetSize(margin_bounds) - padding_size) * 0.5f, padding_size);
   v2 element_size = ToSize(GetSize(padding_bounds), element_spec);
   rect2 element_bounds = RectMinSize(padding_bounds.min + (padding_size - element_size) * 0.5f, element_size);
   
   v2 margin = (GetSize(margin_bounds) - padding_size) / 2.0f;
   v2 padding_offset = V2(margin.x, Max(margin.y, ui_layout->last_row_bottom_margin));
   
   if(!is_first_element && !ui_layout->new_line_issued)
   {
      padding_offset.x = Max(margin.x, ui_layout->last_element_right_margin);
   }
   
   v2 padding_min = ui_layout->at + padding_offset;
   rect2 padding_rect = RectMinSize(padding_min, padding_size.x, padding_size.y);
   
   if((padding_size.x > GetSize(ui_layout->bounds).x) ||
      (padding_size.y > GetSize(ui_layout->bounds).y))
   {
      Assert(false);
      //TODO: do something
      
      //NOTE: this could break if the element is just too big to possibly fit
      //      we should check for that case & do something about it, like 
      //      maybe shrink the element
   }
   
   if(IsInside(padding_rect, ui_layout->bounds))
   {
      ui_layout->at = ui_layout->at + V2(padding_offset.x, 0) + V2(padding_size.x, 0);
      ui_layout->new_line_issued = false;
      ui_layout->has_elements = true;
      
      ui_layout->last_element_right_margin = margin.x;
      ui_layout->row_effective_height = Max(padding_offset.y + padding_size.y, ui_layout->row_effective_height);
      ui_layout->abs_row_bottom_margin = Max(padding_offset.y + padding_size.y + margin.x, ui_layout->abs_row_bottom_margin);
      
      return RectMinSize(padding_rect.min + (padding_size - element_size) * 0.5f, element_size);
   }
   else
   {
      NextLine(ui_layout);
      return Element(ui_layout, element_spec, padding_spec, margin_spec);
   }
}

layout Panel(layout *parent_layout, element_size element_spec, element_size padding_spec, element_size margin_spec)
{
   rect2 bounds = Element(parent_layout, element_spec, padding_spec, margin_spec);
   layout result = Layout(bounds, parent_layout->context);
   return result;
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

v2 TextSize(RenderContext *context, char *text, u32 scale)
{
	if (!text) return V2(0, 0);
   
	r32 width = 0.0f;
   r32 highest = -FLTMAX;
   r32 lowest = FLTMAX;
	while (*text)
	{
		if (*text == ' ')
		{
			width += 10;
		}
		else
		{
			CharacterBitmap *char_bitmap = context->characters + ((u32)(*text) - 33);
			width += char_bitmap->bitmap.width;
         highest = Max(highest, char_bitmap->bitmap.height + char_bitmap->offset.y);
         lowest = Min(lowest, char_bitmap->offset.y);
		}
      
		text++;
	}
   
   return V2(width, highest - lowest);
}

void Rectangle(layout *ui_layout, v4 color, element_size element_spec, element_size padding_spec, element_size margin_spec)
{
   RenderContext *render_context = ui_layout->context->render_context;
   rect2 bounds = Element(ui_layout, element_spec, padding_spec, margin_spec);
   
   Rectangle(render_context, bounds, color);
}

void Bitmap(layout *ui_layout, LoadedBitmap *bitmap, element_size element_spec, element_size padding_spec, element_size margin_spec)
{
   /*
   RenderContext *render_context = ui_layout->context->render_context;
   rect2 bounds = Element(ui_layout, size, margin, ui_flags);
   
   Bitmap(render_context, bitmap, bounds.min);
   */
}

void Text(layout *ui_layout, char *text, u32 scale)
{
   /*
   RenderContext *render_context = ui_layout->context->render_context;
   rect2 bounds = Element(ui_layout, TextSize(render_context, text, scale), margin, ui_flags);
   Rectangle(render_context, bounds, V4(0, 0, 0, 0.2f));
   
   Text(render_context, V2(bounds.min.x, bounds.min.y + 5), text, scale);
   */
}

//TODO: text wrap
b32 Button(layout *ui_layout, LoadedBitmap *icon, char *text, element_size element_spec, element_size padding_spec, element_size margin_spec)
{
   RenderContext *render_context = ui_layout->context->render_context;
   InputState input = ui_layout->context->input_state;
   rect2 bounds = Element(ui_layout, element_spec, padding_spec, margin_spec);
   
   b32 hot = Contains(bounds, input.pos);
   
   if(hot && (input.left_down || input.right_down))
   {
	  Rectangle(render_context, bounds, V4(1.0f, 0.0f, 0.75f, 1.0f));
   }
   else if(hot)
   {
	  Rectangle(render_context, bounds, V4(1.0f, 0.0f, 0.0f, 1.0f));
   }
   else
   {
	  Rectangle(render_context, bounds, V4(0.5f, 0.0f, 0.0f, 1.0f));
   }
   
   v2 bounds_size = GetSize(bounds);
   
   Bitmap(render_context, icon, bounds.min);
   
   if(text && (TextSize(render_context, text, 20).x > GetSize(bounds).x))
   {
      //TODO: finish this
      /*
      char *text_at = text;
      char *last_pass = NULL;
      
      while(*text_at)
      {
         if(*text_at == ' ')
         {
            *text_at = '\0';
            
            if(TextSize(render_context, text, 20).x < GetSize(bounds).x)
            {
               last_pass = text_at;
            }
            else
            {
               if(last_pass)
               {
                  
               }
               //TODO: if the word is just too big this will fail
            }
            
            *text_at = ' ';
         }
         
         text_at++;
      }
      */
   }
   else
   {
      Text(render_context, V2(bounds.min.x + 10, bounds.min.y + 10), text, 20);
   }
   
   return hot && (input.left_up || input.right_up);
}

b32 Button(layout *ui_layout, LoadedBitmap *icon, char *text, b32 triggered, element_size element_spec, element_size padding_spec, element_size margin_spec)
{
   b32 result = Button(ui_layout, icon, text, element_spec, padding_spec, margin_spec);
   
   //TODO: reimplement this
   /*
   v2 bounds_size = RectGetSize(bounds);
   Rectangle(context->render_context, RectPosSize(bounds.min.x, bounds.min.y, 5,
	         (bounds_size.x == bounds_size.y) ? 5 : bounds_size.y),
	         triggered ? V4(0.0f, 0.0f, 0.0f, 1.0f) : V4(0.0f, 0.0f, 0.0f, 0.0f));
   */

   return result;
}
