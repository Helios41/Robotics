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
   r32 height_above_baseline;
   r32 height_below_baseline;
};

enum RenderCommandType
{
	RenderCommandType_DrawRectangle,
	RenderCommandType_DrawBitmap,
   RenderCommandType_DrawRectangleOutline,
   RenderCommandType_DrawLine
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
   v2 size;
	LoadedBitmap *bitmap;
};

struct DrawRectangleOutlineCommand
{
	RenderCommandHeader header;
	v4 color;
	rect2 area;
   u32 thickness;
};

struct DrawLineCommand
{
	RenderCommandHeader header;
	v4 color;
	v2 a;
   v2 b;
   u32 thickness;
};

struct RenderContext
{
   u32 render_commands_size;
   u8 *render_commands_at;
   u8 *render_commands;
   
   struct
   {
      r32 baseline_from_height;
      r32 native_height;
      r32 space_width;
   } font;
   
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
   
   r32 heightest_above_baseline = -FLTMAX;
   r32 lowest_below_baseline = -FLTMAX;
   
   for(u32 char_code = 33;
       char_code <= 126;
       char_code++)
   {
      char curr_char = (char) char_code;
      CharacterBitmap *char_bitmap = result.characters + (char_code - 33);

      int width, height, xoffset, yoffset;
      u8 *mono_bitmap = stbtt_GetCodepointBitmap(&font, 0, stbtt_ScaleForPixelHeight(&font, 80.0f),
                                                 curr_char, &width, &height, &xoffset, &yoffset);

      char_bitmap->bitmap.width = (u32) width;
      char_bitmap->bitmap.height = (u32) height;
      char_bitmap->height_above_baseline = -yoffset;
      char_bitmap->height_below_baseline = height + yoffset;
      
      heightest_above_baseline = Max(heightest_above_baseline, char_bitmap->height_above_baseline);
      lowest_below_baseline = Max(lowest_below_baseline, char_bitmap->height_below_baseline);

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
   
   int space_width;
   stbtt_GetCodepointHMetrics(&font, ' ', &space_width, 0);
   
   result.font.native_height = heightest_above_baseline + lowest_below_baseline;
   result.font.baseline_from_height = heightest_above_baseline / result.font.native_height;
   result.font.space_width = result.font.native_height * 0.4f;
   
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
         draw_bitmap->size = V2(bitmap->width, bitmap->height);
         
         context->render_commands_at += sizeof(DrawBitmapCommand);
         return draw_bitmap;
      }
	}
   
   return NULL;
}

DrawBitmapCommand *Bitmap(RenderContext *context, LoadedBitmap *bitmap, v2 pos, v2 size)
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
         draw_bitmap->size = size;
         
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

DrawLineCommand *Line(RenderContext *context, v2 a, v2 b, v4 color, u32 thickness = 1)
{
	DrawLineCommand *draw_rectangle_outline = (DrawLineCommand *) context->render_commands_at;
   
	if((context->render_commands_at + sizeof(DrawLineCommand)) < 
      (context->render_commands + context->render_commands_size))
	{
		draw_rectangle_outline->header.size = sizeof(DrawLineCommand);
		draw_rectangle_outline->header.type = RenderCommandType_DrawLine;
		draw_rectangle_outline->a = a;
      draw_rectangle_outline->b = b;
		draw_rectangle_outline->color = color;
      draw_rectangle_outline->thickness = thickness;
      
		context->render_commands_at += sizeof(DrawLineCommand);
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
				rect2 area = RectMinSize(draw_bitmap->pos, draw_bitmap->size);

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
         
         case RenderCommandType_DrawLine:
         {
            DrawLineCommand *draw_line = (DrawLineCommand *) render_command;
				
            v2 a = draw_line->a;
            v2 b = draw_line->b;
            
            glLineWidth(draw_line->thickness);
            
				glBegin(GL_LINES);
				{
					glColor4fv(draw_line->color.vs);
					
               glVertex2f(a.x, a.y);
					glVertex2f(b.x, b.y);
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
   b32 key_left;
   b32 key_right;
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

struct interaction
{
   ui_id id;
   u32 ui_layer;
   u32 stack_layer;
};

#define NULL_INTERACTION Interaction(NULL_UI_ID, 0, 0)

interaction Interaction(ui_id id, u32 ui_layer, u32 stack_layer)
{
   interaction result = {};
   
   result.id = id;
   result.ui_layer = ui_layer;
   result.stack_layer = stack_layer;
   
   return result;
}

struct UIContext
{
   v2 window_size;
   
   //TODO: store start times for all of these interactions
   interaction hot_element;
   interaction active_element;
   interaction selected_element;
   
   RenderContext *render_context;
   InputState input_state;
   UIAssets *assets;
   
   u32 text_box_pointer;
   char temp_text_box[20]; //NOTE: for float/int/etc... text boxes
   
   u32 text_box_line;
   u32 text_box_column;
   
   u32 tooltip_ui_layer;
   u32 tooltip_stack_layer;
   string tooltip;
};

struct interaction_state
{
   b32 hot;
   b32 became_hot;
   
   b32 active;
   b32 became_active;
   
   b32 selected;
   b32 became_selected;
};

interaction_state ClickInteraction(UIContext *context, interaction intrct, b32 trigger_cond, b32 active_cond, b32 hot_cond)
{
   interaction_state result = {};
   result.hot = context->hot_element.id == intrct.id;
   result.active = context->active_element.id == intrct.id;
   result.selected = context->selected_element.id == intrct.id;
   
   if(context->active_element.id == intrct.id)
   {
      if(trigger_cond)
      {
         if(context->hot_element.id == intrct.id)
         {
            result.became_selected = true;
            context->selected_element = intrct;
         }
         
         context->active_element = NULL_INTERACTION;
      }
   }
   
   if(context->hot_element.id == intrct.id)
   {
      if(active_cond)
      {
         result.became_active = true;
         context->active_element = intrct;
      }
      
      if(!hot_cond)
         context->hot_element = NULL_INTERACTION;
   }

   b32 can_set_hot = ((context->active_element.id == NULL_UI_ID) ||
                     (context->active_element.id == intrct.id)) &&
                     ((context->hot_element.stack_layer < intrct.stack_layer) ||
                      ((context->hot_element.stack_layer == intrct.stack_layer) &&
                       (context->hot_element.ui_layer < intrct.ui_layer)));
                     
   if(can_set_hot && hot_cond) 
   {
      result.became_hot = true;
      context->hot_element = intrct;
   }
   
   return result;
}

struct layout
{
   u32 ui_layer;
   u32 stack_layer;
   
   UIContext *context;
   rect2 bounds;
   //r32 left_x; //NOTE: x coord of "at"'s origin, used for horizontal translations
   v2 at;
   
   r32 row_effective_height; // = size.y + topleft_margin.y, accumulate the max, not just the latest element
   r32 abs_row_bottom_margin;
   
   r32 last_row_effective_height;
   r32 last_row_bottom_margin;
   
   r32 last_element_right_margin;
   b32 has_elements;
   
   b32 new_line_issued;
};

layout Layout(rect2 bounds, UIContext *context, u32 stack_layer, r32 vertical_scroll = 0)
{
   layout result = {};
   
   result.context = context;
   result.bounds = bounds;
   result.at = bounds.min + V2(0, vertical_scroll);
   result.ui_layer = 0;
   result.stack_layer = stack_layer;
   
   return result;
}

interaction Interaction(ui_id id, layout *ui_layout)
{
   return Interaction(id, ui_layout->ui_layer, ui_layout->stack_layer);
}

v2 v2Max(v2 a, v2 b)
{
   return V2(Max(a.x, b.x), Max(a.y, b.y));
}

void NextLine(layout *ui_layout)
{
   ui_layout->last_row_effective_height = ui_layout->row_effective_height;
   ui_layout->last_row_bottom_margin = ui_layout->abs_row_bottom_margin - ui_layout->row_effective_height;
   
   ui_layout->at = V2(ui_layout->bounds.min.x /*->left_x*/, ui_layout->at.y + ui_layout->row_effective_height);
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
   rect2 padding_rect = RectMinSize(padding_min, (padding_size * 2.0f) + element_size);
   
   get_padding_rect_result result = {};
   result.padding_rect = padding_rect;
   result.padding_offset = padding_offset;
   return result;
}

struct element
{
   rect2 margin_bounds;
   rect2 bounds;
   b32 fit;
};

element Element(layout *ui_layout, v2 element_size, v2 padding_size, v2 margin_size, b32 no_fit_nextline = false)
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
         
         if(no_fit_nextline)
         {
            *ui_layout = temp_layout;
         
            padding_rect_result = GetPaddingRect(*ui_layout, element_size, padding_size, margin_size);
   
            padding_offset = padding_rect_result.padding_offset;
            padding_rect = padding_rect_result.padding_rect;
            padding_rect_size = GetSize(padding_rect);
         }
      }
   }
   
   result.bounds = RectMinSize(padding_rect.min + padding_size, element_size);
   result.margin_bounds = RectMinSize(ui_layout->at, (margin_size * 2.0f) + (padding_size * 2.0f) + element_size);
   
   ui_layout->at = V2(padding_rect.max.x, ui_layout->at.y);
   ui_layout->new_line_issued = false;
   ui_layout->has_elements = true;
   
   ui_layout->last_element_right_margin = margin_size.x;
   ui_layout->row_effective_height = Max(padding_offset.y + padding_rect_size.y, ui_layout->row_effective_height);
   ui_layout->abs_row_bottom_margin = Max(padding_offset.y + padding_rect_size.y + margin_size.y, ui_layout->abs_row_bottom_margin);
   
   return result;
}

struct panel
{
   layout lout;
   element elem;
};

panel Panel(layout *parent_layout, v2 element_size, v2 padding_size, v2 margin_size, r32 vertical_scroll = 0)
{
   panel result = {};
   
   result.elem = Element(parent_layout, element_size, padding_size, margin_size);
   result.lout = Layout(result.elem.bounds, parent_layout->context, parent_layout->stack_layer, vertical_scroll);
   result.lout.ui_layer = parent_layout->ui_layer + 1;
   
   return result;
}

r32 GetTextWidth(RenderContext *context, string text, r32 scale)
{
   r32 scale_coeff = scale / context->font.native_height;
   
	u32 width = 0;
	for(u32 i = 0;
       i < text.length;
       i++)
	{
      char c = text.text[i];
      
		if (c == ' ')
		{
			width += context->font.space_width * scale_coeff;
		}
		else
		{
			CharacterBitmap *char_bitmap = context->characters + ((u32)c - 33);
			width += char_bitmap->bitmap.width * scale_coeff + Min(char_bitmap->bitmap.width * scale_coeff / 2, scale_coeff * 10);
		}
	}
   
   return width;
}

v2 GetTextSize(RenderContext *context, string text, u32 scale)
{
   return V2(GetTextWidth(context, text, scale), scale);
}

void Text(RenderContext *context, rect2 bounds, string text)
{  
   r32 baseline = GetSize(bounds).y * context->font.baseline_from_height;
   v2 origin = bounds.min;
   r32 scale_coeff = GetSize(bounds).y / context->font.native_height;
   
   RectangleOutline(context, bounds, V4(0, 0, 0, 1));
   Line(context, V2(bounds.min.x, bounds.min.y + baseline), V2(bounds.max.x, bounds.min.y + baseline), V4(0, 0, 0, 1));
   
	u32 xoffset = 0;
	for(u32 i = 0;
       i < text.length;
       i++)
	{
      char c = text.text[i];
      
		if (c == ' ')
		{
			xoffset += context->font.space_width * scale_coeff;
		}
		else
		{
			CharacterBitmap *char_bitmap = context->characters + ((u32)c - 33);
			Bitmap(context, &char_bitmap->bitmap,
                V2(origin.x + xoffset, origin.y + (baseline - char_bitmap->height_above_baseline * scale_coeff)),
                V2(char_bitmap->bitmap.width, char_bitmap->bitmap.height) * scale_coeff);
			xoffset += char_bitmap->bitmap.width * scale_coeff + Min(char_bitmap->bitmap.width * scale_coeff / 2, scale_coeff * 10);
		}
	}
}

rect2 Text(RenderContext *context, v2 pos, string text, u32 scale)
{
   rect2 bounds = RectMinSize(pos, GetTextSize(context, text, scale));
   Text(context, bounds, text);
   return bounds;
}

rect2 GetCharBounds(RenderContext *context, rect2 bounds, string text, u32 char_index)
{  
   r32 baseline = GetSize(bounds).y * context->font.baseline_from_height;
   v2 origin = bounds.min;
   r32 scale_coeff = GetSize(bounds).y / context->font.native_height;
   
   rect2 result = RectMinSize(V2(0, 0), V2(0, 0));
   
   u32 xoffset = 0;
	for(u32 i = 0;
       i < text.length;
       i++)
	{
      char c = text.text[i];
      
      if(i == char_index)
      {
         if (c == ' ')
         {
            result = RectMinSize(V2(origin.x + xoffset, origin.y), V2(context->font.space_width * scale_coeff, GetSize(bounds).y));
         }
         else
         {
            CharacterBitmap *char_bitmap = context->characters + ((u32)c - 33);
            result = RectMinSize(V2(origin.x + xoffset,
                                    origin.y + (baseline - char_bitmap->height_above_baseline * scale_coeff)),
                                 V2(char_bitmap->bitmap.width, char_bitmap->bitmap.height) * scale_coeff);
         }
      }
      
		if (c == ' ')
		{
			xoffset += context->font.space_width * scale_coeff;
		}
		else
		{
			CharacterBitmap *char_bitmap = context->characters + ((u32)c - 33);
			xoffset += char_bitmap->bitmap.width * scale_coeff + Min(char_bitmap->bitmap.width * scale_coeff / 2, scale_coeff * 10);
		}
	}
   
   return result;
}

rect2 GetCharBounds(RenderContext *context, v2 pos, string text, u32 scale, u32 char_index)
{
   rect2 bounds = RectMinSize(pos, GetTextSize(context, text, scale));
   return GetCharBounds(context, bounds, text, char_index);
}

r32 TextLabelSuggestScale(RenderContext *render_context, rect2 bounds, string text)
{
   //TODO: write and start using this
   return 0.0f;
}

void TextLabel(RenderContext *render_context, rect2 bounds, string text, r32 scale)
{
   if(!IsEmpty(text))
   {
      u32 segment_count;
      string *segments = Split(text, ' ', &segment_count);
      v2 bounds_size = GetSize(bounds);
      
      if(segment_count > 1)
      {
         string curr_line = String(text.text, 0);
         v2 at = bounds.min;
         
         for(u32 i = 0;
             i < segment_count;
             i++)
         {
            string curr_segment = segments[i];
            string new_line = String(curr_line.text, curr_line.length + ((i == 0) ? 0 : 1) + curr_segment.length);
            v2 new_line_size = GetTextSize(render_context, new_line, scale);
            
            if(new_line_size.x > bounds_size.x)
            {
               r32 curr_line_height = GetTextSize(render_context, curr_line, scale).y;
               
               if((at.y + curr_line_height) > bounds.max.y)
               {
                  curr_line = EmptyString();
                  break;
               }
                  
               Text(render_context, at, curr_line, scale);
               curr_line = curr_segment;
               at = at + V2(0, curr_line_height);
            }
            else
            {
               curr_line = new_line;
            }
         }
         
         Text(render_context, at, curr_line, scale);
      }
      else
      {
         Text(render_context, bounds.min, text, scale);
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

element Text(layout *ui_layout, string text, u32 scale, v2 padding, v2 margin)
{
   RenderContext *render_context = ui_layout->context->render_context;

   element text_element = Element(ui_layout, GetTextSize(render_context, text, scale), padding, margin);
   Rectangle(render_context, text_element.bounds, V4(0, 0, 0, 0.2f));
   
   Text(render_context, text_element.bounds.min, text, scale);
   return text_element;
}

struct button
{
   b32 state;
   element elem;
   interaction_state intrct;
};

button _Button(ui_id id, layout *ui_layout, LoadedBitmap *icon, string text, v2 element_size, v2 padding_size, v2 margin_size)
{
   UIContext *context = ui_layout->context;
   RenderContext *render_context = context->render_context;
   InputState input = ui_layout->context->input_state;
   element button_element = Element(ui_layout, element_size, padding_size, margin_size);
   
   interaction_state button_interact = ClickInteraction(context, Interaction(id, ui_layout->ui_layer + 1, ui_layout->stack_layer), context->input_state.left_up,
                                                        context->input_state.left_down, Contains(button_element.bounds, context->input_state.pos));
   
   if(button_interact.active)
   {
      Rectangle(render_context, button_element.bounds, V4(1.0f, 0.0f, 0.75f, 1.0f));
   }
   else if(button_interact.hot)
   {
      Rectangle(render_context, button_element.bounds, V4(1.0f, 0.0f, 0.0f, 1.0f));
   }
   else
   {
      Rectangle(render_context, button_element.bounds, V4(0.5f, 0.0f, 0.0f, 1.0f));
   }
   
   if(button_interact.hot)
   {
      if((ui_layout->stack_layer > context->tooltip_stack_layer) ||
         ((ui_layout->stack_layer == context->tooltip_stack_layer) && (ui_layout->ui_layer > context->tooltip_ui_layer)))
      {
         context->tooltip = text;
      }
   }
   
   Bitmap(render_context, icon, button_element.bounds.min);
   TextLabel(render_context, button_element.bounds, text, 20);
   
   button result = {};
   result.state = button_interact.became_selected;
   result.elem = button_element;
   result.intrct = button_interact;
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

element _ToggleSlider(ui_id id, layout *ui_layout, b32 *option, string option1, string option2, v2 element_size, v2 padding_size, v2 margin_size)
{
   UIContext *context = ui_layout->context;
   RenderContext *render_context = context->render_context;
   InputState input = ui_layout->context->input_state;
   element toggle_element = Element(ui_layout, element_size, padding_size, margin_size);
   
   interaction_state toggle_interact = ClickInteraction(context, Interaction(id, ui_layout->ui_layer + 1, ui_layout->stack_layer), context->input_state.left_up,
                                                        context->input_state.left_down, Contains(toggle_element.bounds, context->input_state.pos));
   
   v2 option_bounds_size = V2(GetSize(toggle_element.bounds).x / 2, GetSize(toggle_element.bounds).y);
   rect2 option1_bounds = RectMinSize(toggle_element.bounds.min, option_bounds_size);
   rect2 option2_bounds = RectMinSize(toggle_element.bounds.min + V2(option_bounds_size.x, 0),
                                      option_bounds_size);   
   
   if(toggle_interact.became_selected)
   {
      *option = !(*option);
   }
   
   Rectangle(render_context, option1_bounds, (*option) ? V4(1, 0, 0, 1) : V4(0.5, 0, 0, 1));
   TextLabel(render_context, option1_bounds, option1, 20);
   
   Rectangle(render_context, option2_bounds, !(*option) ? V4(1, 0, 0, 1) : V4(0.5, 0, 0, 1));
   TextLabel(render_context, option2_bounds, option2, 20);
   
   if(toggle_interact.active)
   {
      RectangleOutline(render_context, toggle_element.bounds, V4(0.6f, 0.6f, 0.6f, 1.0f));
   }
   else if(toggle_interact.hot)
   {
      RectangleOutline(render_context, toggle_element.bounds, V4(0.3f, 0.3f, 0.3f, 1.0f));
   }
   else
   {
      RectangleOutline(render_context, toggle_element.bounds, V4(0.0f, 0.0f, 0.0f, 1.0f));
   }
   
   if(toggle_interact.hot)
   {
      string tooltip_text = EmptyString();
      
      if(Contains(option1_bounds, context->input_state.pos))
      {
         tooltip_text = option1;
      }
      else if(Contains(option2_bounds, context->input_state.pos))
      {
         tooltip_text = option2;
      }
      
      if(IsEmpty(context->tooltip) || (ui_layout->stack_layer > context->tooltip_stack_layer) ||
         ((ui_layout->stack_layer == context->tooltip_stack_layer) && (ui_layout->ui_layer > context->tooltip_ui_layer)))
      {
         context->tooltip = tooltip_text;
      }
   }
   
   return toggle_element;
}

#define ToggleSlider(...) _ToggleSlider(GEN_UI_ID, __VA_ARGS__)

void TextBoxInsert(string buffer, string curr_line, UIContext *context, char c)
{
   for(u32 i = buffer.length;
       i > (context->text_box_column + (u32)(curr_line.text - buffer.text));
       i--)
   {
      buffer.text[i] = buffer.text[i - 1];
   }
   
   curr_line.text[context->text_box_column] = c;
   if(context->text_box_column < curr_line.length)
   {
      context->text_box_column++;
   }
}

void CheckDrawCursor(interaction_state text_box_interact, rect2 textbox_bounds, string curr_draw_line,
                     string curr_line, UIContext *context, u32 cursor_width, r32 line_height, v2 at)
{
   RenderContext *render_context = context->render_context;
   if(text_box_interact.selected && (curr_draw_line.text == curr_line.text))
   {
      rect2 cursor_rect;
      
      if(context->text_box_column == curr_line.length)
      {
         rect2 selected_char = GetCharBounds(render_context, textbox_bounds.min,
                                             curr_line, 20, curr_line.length - 1);
         cursor_rect = RectMinSize(V2(selected_char.max.x, at.y), V2(cursor_width, line_height));
      }
      else
      {
         rect2 selected_char = GetCharBounds(render_context, textbox_bounds.min,
                                             curr_line, 20, context->text_box_column);
         cursor_rect = RectMinSize(V2(selected_char.min.x - cursor_width, at.y), V2(cursor_width, line_height));
      }
      
      Rectangle(render_context, cursor_rect, V4(0, 0, 0, 1));                                    
   }
}

void _TextBox(ui_id id, layout *ui_layout, string buffer, v2 element_size, v2 padding_size, v2 margin_size)
{
   UIContext *context = ui_layout->context;
   RenderContext *render_context = context->render_context;
   element textbox_element = Element(ui_layout, element_size, padding_size, margin_size);
   
   interaction_state text_box_interact =
      ClickInteraction(context, Interaction(id, ui_layout), context->input_state.left_up,
                       context->input_state.left_down, Contains(textbox_element.bounds, context->input_state.pos));
  
   if(text_box_interact.became_selected)
   {
      context->text_box_line = 0;
      context->text_box_column = 0;
   }
   
   string curr_line = EmptyString();
   
   if(text_box_interact.selected)
   {
      u32 on_line = 0;
      
      for(u32 i = 0;
          i < buffer.length;
          i++)
      {
         char c = buffer.text[i];
         
         if(on_line == context->text_box_line)
         {
            curr_line = String(buffer.text + i,
                               IndexOfFirst(String(buffer.text + i, buffer.length - i), '\n'));
            break;
         }
         
         if(c == '\n')
         {
            on_line++;
         }
      }
      
      u32 line_count = CountChar(buffer, '\n');
      
      Assert(curr_line.text != NULL);
      context->text_box_column = Clamp(0, curr_line.length, context->text_box_column);
   
      if(context->input_state.char_key_up &&
         ((context->text_box_column + (u32)(curr_line.text - buffer.text)) < buffer.length))
      {
         TextBoxInsert(buffer, curr_line, context, context->input_state.key_char);
      }
      
      if(context->input_state.key_enter &&
         ((context->text_box_column + (u32)(curr_line.text - buffer.text)) < buffer.length))
      {
         TextBoxInsert(buffer, curr_line, context, '\n');
      }
      
      if(context->input_state.key_backspace && (context->text_box_column > 0))
      {
         for(u32 i = context->text_box_column + (u32)(curr_line.text - buffer.text) - 1;
             i < (buffer.length - 1);
             i++)
         {
            buffer.text[i] = buffer.text[i + 1];
         }
         
         buffer.text[buffer.length - 1] = ' ';
         context->text_box_column--;
      }
      
      if(context->input_state.key_up && (context->text_box_line > 0))
      {
         context->text_box_line--;
      }
      if(context->input_state.key_down && (context->text_box_line < line_count))
      {
         context->text_box_line++;
      }
      
      if(context->input_state.key_right && (context->text_box_column < curr_line.length))
      {
         context->text_box_column++;
      }
      if(context->input_state.key_left && (context->text_box_column > 0))
      {
         context->text_box_column--;
      }
   }
   
   RectangleOutline(render_context, textbox_element.bounds, V4(0, 0, 0, 1), 2);
   
   u32 cursor_width = 2;
   
   string curr_draw_line = String(buffer.text, 0);
   v2 at = textbox_element.bounds.min;
   for(u32 i = 0;
      i < buffer.length;
      i++)
   {
      if(buffer.text[i] == '\n')
      {
         Text(render_context, at, curr_draw_line, 20);
         r32 line_height = GetTextSize(render_context, curr_draw_line, 20).y;
         
         CheckDrawCursor(text_box_interact, textbox_element.bounds, curr_draw_line,
                         curr_line, context, cursor_width, line_height, at);
            
         at = at + V2(0, line_height);
         curr_draw_line = String(curr_draw_line.text + curr_draw_line.length + 1, 0);
      }
      else
      {
         curr_draw_line.length++;
      }
   }
   
   Text(render_context, at, curr_draw_line, 20);
   r32 line_height = GetTextSize(render_context, curr_draw_line, 20).y;
   
   CheckDrawCursor(text_box_interact, textbox_element.bounds, curr_draw_line,
                     curr_line, context, cursor_width, line_height, at);
}

void _TextBox(ui_id id, layout *ui_layout, r32 *value, v2 element_size, v2 padding_size, v2 margin_size)
{
   UIContext *context = ui_layout->context;
   RenderContext *render_context = context->render_context;
   element textbox_element = Element(ui_layout, element_size, padding_size, margin_size);
   string temp_text_box = String(context->temp_text_box, ArrayCount(context->temp_text_box));
   
   interaction_state text_box_interact =
      ClickInteraction(context, Interaction(id, ui_layout), context->input_state.left_up,
                       context->input_state.left_down, Contains(textbox_element.bounds, context->input_state.pos));
  
   if(text_box_interact.became_selected)
   {
      context->text_box_pointer = 0;
   }
  
   if(text_box_interact.selected)
   {
      //TODO: a way to allocate strings safely in functions like ToString instead of passing one in as a buffer
      string buffer = String((char *) malloc(20 * sizeof(char)), 20);
      Clear(buffer);
      CopyTo(ToString(buffer, *value), temp_text_box);
      free(buffer.text);
   }
   
   if(text_box_interact.selected)
   {
      if(context->input_state.char_key_up)
      {
         temp_text_box.text[context->text_box_pointer] = context->input_state.key_char;
         if(context->text_box_pointer < (temp_text_box.length - 1))
         {
            context->text_box_pointer++;
         }
      }
      if(context->input_state.key_backspace)
      {
         temp_text_box.text[context->text_box_pointer] = ' ';
         if(context->text_box_pointer > 0)
         {
            context->text_box_pointer--;
         }
      }
      if(context->input_state.key_right && (context->text_box_pointer < (temp_text_box.length - 1)))
      {
         context->text_box_pointer++;
      }
      if(context->input_state.key_left && (context->text_box_pointer > 0))
      {
         context->text_box_pointer--;
      }
   }
   
   if(text_box_interact.selected)
   {
      *value = ToR32(temp_text_box);
   }
   
   Rectangle(render_context, textbox_element.bounds, V4(0.5f, 0.0f, 0.0f, 1.0f));
   RectangleOutline(render_context, textbox_element.bounds, V4(0.0f, 0.0f, 0.f, 1.0f), text_box_interact.selected ? 3 : 1);
   
   string buffer = String((char *) malloc(20 * sizeof(char)), 20);
   Clear(buffer);
   Text(render_context, textbox_element.bounds.min,
        text_box_interact.selected ? temp_text_box : ToString(buffer, *value), 20);
   free(buffer.text);
   
   if(text_box_interact.selected)
   {
      rect2 selected_char = GetCharBounds(render_context, textbox_element.bounds.min,
                                          temp_text_box, 20, context->text_box_pointer);
      Rectangle(render_context, selected_char, V4(0, 0, 0, 0.3f));
   }
}

void _TextBox(ui_id id, layout *ui_layout, r32 min, r32 max, r32 *value, v2 element_size, v2 padding_size, v2 margin_size)
{
   _TextBox(id, ui_layout, value, element_size, padding_size, margin_size);
   *value = Clamp(min, max, *value);
}

void _TextBox(ui_id id, layout *ui_layout, u32 *value, v2 element_size, v2 padding_size, v2 margin_size)
{
   r32 float_value = (r32) *value;
   _TextBox(id, ui_layout, &float_value, element_size, padding_size, margin_size);
   *value = (u32)(float_value + 0.5);
}

void _TextBox(ui_id id, layout *ui_layout, u32 min, u32 *value, v2 element_size, v2 padding_size, v2 margin_size)
{
   _TextBox(id, ui_layout, value, element_size, padding_size, margin_size);
   *value = Max(min, *value);
}

#define TextBox(...) _TextBox(GEN_UI_ID, __VA_ARGS__)

void _SliderBar(ui_id id, layout *ui_layout, r32 min, r32 max, r32 *value, v2 element_size, v2 padding_size, v2 margin_size)
{
   UIContext *context = ui_layout->context;
   RenderContext *render_context = context->render_context;
   element slider_element = Element(ui_layout, element_size, padding_size, margin_size);
   
   v2 tab_size;
   v2 tab_pos;
   
   r32 min_mouse;
   r32 max_mouse;
   r32 curr_mouse;
   
   Assert(GetSize(slider_element.bounds).y != GetSize(slider_element.bounds).x);
   
   if(GetSize(slider_element.bounds).x > GetSize(slider_element.bounds).y)
   {
      tab_size = V2(GetSize(slider_element.bounds).y, GetSize(slider_element.bounds).y);
      min_mouse = slider_element.bounds.min.x + tab_size.x * 0.5;
      max_mouse = slider_element.bounds.max.x - tab_size.x * 0.5;
      tab_pos = V2(min_mouse + (max_mouse - min_mouse) * ((*value - min) / (max - min)), GetCenter(slider_element.bounds).y);
      curr_mouse = context->input_state.pos.x;
   }
   else if(GetSize(slider_element.bounds).y > GetSize(slider_element.bounds).x)
   {
      tab_size = V2(GetSize(slider_element.bounds).x, GetSize(slider_element.bounds).x);
      min_mouse = slider_element.bounds.min.y + tab_size.y * 0.5;
      max_mouse = slider_element.bounds.max.y - tab_size.y * 0.5;
      tab_pos = V2(GetCenter(slider_element.bounds).x, min_mouse + (max_mouse - min_mouse) * ((*value - min) / (max - min)));
      curr_mouse = context->input_state.pos.y;
   }
   
   rect2 tab_bounds = RectPosSize(tab_pos, tab_size);
   
   interaction_state tab_interact =
      ClickInteraction(context, Interaction(id, ui_layout), context->input_state.left_up,
                       context->input_state.left_down, Contains(tab_bounds, context->input_state.pos));
   
   Rectangle(render_context, slider_element.bounds, V4(0.5, 0.5, 0.5, 1));
   
   if(tab_interact.active)
   {
      Rectangle(render_context, tab_bounds, V4(1.0f, 0.0f, 0.75f, 1.0f));
      if(min_mouse > curr_mouse)
      {
         *value = min;
      }
      else if(curr_mouse > max_mouse)
      {
         *value = max;
      }
      else
      {
         *value = min + (max - min) * ((curr_mouse - min_mouse) / (max_mouse - min_mouse));
      }
   }
   else if(tab_interact.hot)
   {
      Rectangle(render_context, tab_bounds, V4(1.0f, 0.0f, 0.0f, 1.0f));
   }
   else
   {
      Rectangle(render_context, tab_bounds, V4(0.5f, 0.0f, 0.0f, 1.0f));
   }
}

#define SliderBar(...) _SliderBar(GEN_UI_ID, __VA_ARGS__)