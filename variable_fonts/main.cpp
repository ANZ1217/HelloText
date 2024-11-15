#include <iostream>
#include <cstdio>
#include <cstring>
#include <string>
#include <cmath>
#include <vector>
#include <cassert>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_MULTIPLE_MASTERS_H
// for freetype

#include <hb.h>
#include <hb-ft.h>
#include <hb-ot.h>
// for harfbuzz

#include <cairo.h>
#include <cairo-ft.h>
// for cairo

const char* FONT_FILE = "./NotoSans-VariableFont_wdth,wght.ttf";
const int FONT_SIZE = 72;
const double MARGIN = FONT_SIZE * .5;
const int MAX_STR_LEN = 10000;
const int AXIS_CNT = 2;

const char* WEIGHT_NAME = "Weight";
const char* WIDTH_NAME = "Width";

int weight = 0;
int width = 0;
std::string str = "Hello, World!";

namespace
{ // global variables
    FT_Library library;
    FT_Face face;

    hb_font_t *hb_font;
    hb_buffer_t *hb_buffer;

    hb_glyph_info_t *info;
    hb_glyph_position_t *pos;

    cairo_surface_t *cairo_surface;
    cairo_t *cr;
    cairo_font_face_t *cairo_face;
}

void ShapeText()
{
    hb_font = hb_ft_font_create(face, NULL);
    hb_buffer = hb_buffer_create();

    hb_buffer_add_utf8(hb_buffer, str.c_str(), -1, 0, -1);
    hb_buffer_guess_segment_properties(hb_buffer);

    hb_variation_t variations[AXIS_CNT];
    variations[0].tag = HB_OT_TAG_VAR_AXIS_WEIGHT;
    variations[0].value = weight;
    variations[1].tag = HB_OT_TAG_VAR_AXIS_WIDTH;
    variations[1].value = width;
    hb_font_set_variations(hb_font, variations, AXIS_CNT);

    hb_shape(hb_font, hb_buffer, NULL, 0);

    unsigned int len = hb_buffer_get_length(hb_buffer);
    info = hb_buffer_get_glyph_infos(hb_buffer, NULL);
    pos = hb_buffer_get_glyph_positions(hb_buffer, NULL);
}

void RenderText()
{
    double width = 2 * MARGIN;
    double height = 2 * MARGIN;

    unsigned int len = hb_buffer_get_length(hb_buffer);
    for (unsigned int i = 0; i < len; i++)
    {
        width += pos[i].x_advance / 64.;
        height -= pos[i].y_advance / 64.; // HarfBuzz는 y가 커지는 방향이 위쪽을 뜻한다. 세로쓰기를 하면 글자가 아래로 내려가므로, y_advance는 음수가 된다.
    }

    if (HB_DIRECTION_IS_HORIZONTAL(hb_buffer_get_direction(hb_buffer)))
        height += FONT_SIZE;
    else
        width += FONT_SIZE;

    // Cairo surface create
    cairo_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
                                               ceil(width),
                                               ceil(height));

    // Cairo create
    cr = cairo_create(cairo_surface);
    cairo_set_source_rgba(cr, 1., 1., 1., 1.);
    cairo_paint(cr);
    cairo_set_source_rgba(cr, 0., 0., 0., 1.);
    cairo_translate(cr, MARGIN, MARGIN); // 현재 변환 행렬 수정 (Margin, Margin)으로

    // Cairo Font Face 설정
    cairo_face = cairo_ft_font_face_create_for_ft_face(face, 0);
    cairo_set_font_face(cr, cairo_face);
    cairo_set_font_size(cr, FONT_SIZE);

    // baseLine 설정
    // baseLine은 기준이 되는 아래 줄을 뜻한다. gpqy등의 문자들은 이 아래로 내려가기도 한다.
    if (HB_DIRECTION_IS_HORIZONTAL(hb_buffer_get_direction(hb_buffer)))
    {
        cairo_font_extents_t font_extents;
        cairo_font_extents(cr, &font_extents);
        double baseline = (FONT_SIZE - font_extents.height) * .5 + font_extents.ascent;
        cairo_translate(cr, 0, baseline);
    }
    else
    {
        cairo_translate(cr, FONT_SIZE * .5, 0);
    }

    cairo_glyph_t *cairo_glyphs = cairo_glyph_allocate(len);
    double current_x = 0;
    double current_y = 0;
    for (unsigned int i = 0; i < len; i++)
    {
        cairo_glyphs[i].index = info[i].codepoint;
        cairo_glyphs[i].x = current_x + pos[i].x_offset / 64.;
        cairo_glyphs[i].y = -(current_y + pos[i].y_offset / 64.);
        current_x += pos[i].x_advance / 64.;
        current_y += pos[i].y_advance / 64.;
    }

    cairo_show_glyphs(cr, cairo_glyphs, len);
    cairo_glyph_free(cairo_glyphs);

    cairo_surface_write_to_png(cairo_surface, "out.png");
}

void Destroy()
{
    cairo_font_face_destroy(cairo_face);
    cairo_destroy(cr);
    cairo_surface_destroy(cairo_surface);

    hb_buffer_destroy(hb_buffer);
    hb_font_destroy(hb_font);

    FT_Done_Face(face);
    FT_Done_FreeType(library);
}

int main(int argc, char* argv[])
{
    printf("How to use: ./variable_fonts [weight] [width] [text].\n");

    if(argc > 1) weight = atoi(argv[1]);
    if(argc > 2) width = atoi(argv[2]);
    if(argc > 3) str = argv[3];

    // freetype
    if (FT_Init_FreeType(&library))
    {
        printf("Freetype library init error.\n");
        return 1;
    }

    if(FT_New_Face(library, FONT_FILE, 0, &face))
    {
        printf("Font load error.\n");
        return 1;
    }

    FT_Set_Pixel_Sizes(face, 0, FONT_SIZE);

    FT_MM_Var* mm_var;
    if(FT_Get_MM_Var(face, &mm_var) == 0)
    {
        printf("Variable Font detected.\n");
        FT_Fixed coordinates[mm_var->num_axis];

        printf("Variable Count: %d\n", mm_var->num_axis);
        for(int i=0;i<mm_var->num_axis;i++)
        {
            printf("Variable: %s\n", mm_var->axis[i].name);
            if(strcmp(mm_var->axis[i].name, WEIGHT_NAME) == 0)
            {
                printf("Weight detected.\n");
                coordinates[i] = weight << 16;
            }
            else if(strcmp(mm_var->axis[i].name, WIDTH_NAME) == 0)
            {
                printf("WIDTH detected.\n");
                coordinates[i] = width << 16;
            }
        }

        FT_Set_Var_Design_Coordinates(face, mm_var->num_axis, coordinates);
    }

    ShapeText();
    RenderText();
    Destroy();
}