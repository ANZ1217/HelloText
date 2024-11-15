#include <iostream>
#include <string>
#include <cmath>
#include <vector>
#include <cassert>

#include <fontconfig/fontconfig.h>
// for fontconfig

#include <ft2build.h>
#include FT_FREETYPE_H
// for freetype

#include <hb.h>
#include <hb-ft.h>
// for harfbuzz

#include <cairo.h>
#include <cairo-ft.h>
// for cairo

#include <fribidi/fribidi.h>
// for fribidi

const char *FONT_NAME = "Arial";
const int FONT_SIZE = 36;
const double MARGIN = FONT_SIZE * .5;
const int MAX_STR_LEN = 10000;

const std::string str = "Ленивый рыжий кот شَدَّة latin العَرَبِية";

namespace
{ // global variables

    FcConfig *config;
    FcPattern *pat;
    FcPattern *font;

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

std::string my_fontconfig()
{
    // FontConfig 사용하기

    FcInit(); // initializes Fontconfig

    // 현재 기기의 FontConfig, Font 로드
    config = FcInitLoadConfigAndFonts();

    // Font Name으로 패턴 만듬
    pat = FcNameParse((const FcChar8 *)FONT_NAME);

    // 매치 가능한 범위를 늘려줌
    FcConfigSubstitute(config, pat, FcMatchPattern);
    FcDefaultSubstitute(pat);

    std::string fontFile;

    // 폰트를 찾는다
    FcResult result;
    font = FcFontMatch(config, pat, &result);
    if (font)
    {
        FcChar8 *file = NULL;
        if (FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch)
        {
            // fontFile = (char*)file;
            // 이 file FcPattern* font에 binding 되어 있다.
            // 따라서 font가 destroy되면, file 자동으로 없어진다.
            // 이 에러를 없애기 위해서는 fileName을 deepcopy하면 된다.

            fontFile = (char *)file;

            std::cout << "fontFile: " << fontFile << '\n';
        }
    }

    std::cout << "\n";

    return fontFile;
}

void my_freetype(std::string &fontFile)
{
    FT_Error error;

    // library 초기화
    if (error = FT_Init_FreeType(&library))
        abort();

    // font file로 부터 font face 로딩
    if (error = FT_New_Face(library,
                            fontFile.c_str(),
                            0,
                            &face))
        abort();

    /*
    // memory로부터 font file을 이미 로드했을 때
    error = FT_New_Memory_Face(library, buffer, size, 0, &face);
    */

    // font size 설정
    if (error = FT_Set_Char_Size(
            face,
            0,              // char_width 0일시 height와 동기화됨
            FONT_SIZE * 64, // char_height, FONT_SIZE pt
            0,              // horizontal device resolution
            0               // vertical device resolution, 둘다 0일시 자동으로 72 dpi로 설정됨.
            ))
        abort();

    /*
    fontsize를 pixel로 설정
    error = FT_Set_Pixel_Sizes(face, 0, 16);
    */

    /*
        // 아래와 같이 직접 shaping 해볼 수 있다.

        // Glyph Image 로드
        std::string str = "Hello, Text!";
        FT_GlyphSlot slot = face->glyph;
        FT_UInt glyph_index;

        int cur_x = 300;
        int cur_y = 200;

        std::cout << str << '\n';
        for(char c : str)
        {
            // character code를 glyph index로 변환
            glyph_index = FT_Get_Char_Index(face, c);

            // glyph index를 이용해 glyph 로드
            if(error = FT_Load_Glyph(face, glyph_index, FT_LOAD_RENDER)) abort();

            // 그린다
            std::cout << "char: " << c << "(" << cur_x + slot->bitmap_left << ", " << cur_y - slot->bitmap_top << ")\n";

            // x, y 좌표 갱신
            cur_x += slot->advance.x / 64;
            cur_y += slot->advance.y / 64; // 지금은 필요 없음
        }
    */
}

std::string my_fribidi()
{
    // Fribidi 사용하기
    std::vector<FriBidiChar> fribidi_in_char(MAX_STR_LEN);

    // charset을 unicode로 바꿈
    FriBidiStrIndex fribidi_len = fribidi_charset_to_unicode(
        FRIBIDI_CHAR_SET_UTF8,
        str.c_str(),
        str.size(),
        fribidi_in_char.data());

    FriBidiCharType fribidi_pbase_dir = FRIBIDI_TYPE_LTR;
    std::vector<FriBidiChar> fribidi_visual_char(MAX_STR_LEN + 1);

    std::cout << "Before Fribidi: " << str << '\n';

    // RTL은 반대로 뒤집어줌
    fribidi_boolean stat = fribidi_log2vis(
        /* input */
        fribidi_in_char.data(), // unicode input string
        fribidi_len,            // length
        &fribidi_pbase_dir,     // input and outpu base direction
        /* output */
        fribidi_visual_char.data(), // visual string
        NULL,                       // logical string to visual string position mapping
        NULL,                       // visual string to logical string position mapping
        NULL                        // 각 character의 분류
    );

    if (!stat)
        abort();

    std::string str_after_fribidi(MAX_STR_LEN, 0);

    // unicode를 charset으로 바꿈
    const FriBidiStrIndex new_len = fribidi_unicode_to_charset(FRIBIDI_CHAR_SET_UTF8,
                                                               fribidi_visual_char.data(),
                                                               fribidi_len,
                                                               str_after_fribidi.data());

    assert(new_len < MAX_STR_LEN);
    str_after_fribidi.resize(new_len);

    std::cout << "After Fribidi: " << str_after_fribidi << '\n';
    std::cout << '\n';

    return str_after_fribidi;
}

void my_harfbuzz(std::string str)
{
    // HarfBuzz 사용해보기

    // hb create
    hb_font = hb_ft_font_create(face, NULL);

    // hb buffer create
    hb_buffer = hb_buffer_create();

    hb_buffer_add_utf8(hb_buffer,
                       str.c_str(),
                       -1,  // str의 length, NULL이 terminator면 -1
                       0,   // buffer의 offset
                       -1); // 몇 글자 넣을건지, 전부면 -1
    hb_buffer_guess_segment_properties(hb_buffer);

    // 만든다
    hb_shape(hb_font, hb_buffer, NULL, 0);

    // information 획득
    unsigned int len = hb_buffer_get_length(hb_buffer);
    info = hb_buffer_get_glyph_infos(hb_buffer, NULL);
    pos = hb_buffer_get_glyph_positions(hb_buffer, NULL);

    // Raw data 출력
    std::cout << "Raw buffer: \n";
    for (unsigned int i = 0; i < len; i++)
    {
        hb_codepoint_t gid = info[i].codepoint;    // Shaping 전 Unicode code point, Shaping 후 Glyph index
        unsigned int cluster = info[i].cluster;    // 현재 이 글자가 original text의 몇번째 index에 매칭되는지?
        double x_advance = pos[i].x_advance / 64.; // Advance: 다음 glyph를 놓아햐는 상대 좌표
        double y_advance = pos[i].y_advance / 64.;
        double x_offset = pos[i].x_offset / 64.;
        double y_offset = pos[i].y_offset / 64.;

        char glyphname[32];
        hb_font_get_glyph_name(hb_font, gid, glyphname, sizeof(glyphname));

        //        std::cout << "Glyph: " << glyphname << "\t\t";
        //        std::cout << "Gid: " << gid << "\t\t";
        //        std::cout << "Cluster: " << cluster << '\n';
        //        std::cout << "Advance: (" << x_advance << ", " << y_advance << ")\t";
        //        std::cout << "Offset: (" << x_offset << ", " << y_offset << ")\t";
        //        std::cout << '\n';
    }

    // 절대 위치 출력해보기
    std::cout << "Converted to absolute positions:\n";
    {
        double current_x = 0.;
        double current_y = 0.;
        for (unsigned int i = 0; i < len; i++)
        {
            hb_codepoint_t gid = info[i].codepoint;
            unsigned int cluster = info[i].cluster;
            double x_position = current_x + pos[i].x_offset / 64.;
            double y_position = current_y + pos[i].y_offset / 64.;

            char glyphname[32];
            hb_font_get_glyph_name(hb_font, gid, glyphname, sizeof(glyphname));

            std::cout << "Glyph: " << glyphname << "\t\t";
            std::cout << "Gid: " << gid << "\t\t";
            std::cout << "Cluster: " << cluster << '\n';
            std::cout << "Position: (" << x_position << ", " << y_position << ")\t";
            std::cout << '\n';

            current_x += pos[i].x_advance / 64.;
            current_y += pos[i].y_advance / 64.;
        }

        std::cout << '\n';
    }
}

void my_cairo()
{
    // Cairo를 이용해 그리기
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

        std::cout << "Font Size: " << FONT_SIZE << '\n';
        std::cout << "font_extents.height: " << font_extents.height << '\n';
        std::cout << "font_extents.ascent: " << font_extents.ascent << '\n';
        std::cout << "font_extents.descent: " << font_extents.descent << '\n';
        std::cout << "font_extents.max_x_advance: " << font_extents.max_x_advance << '\n';
        std::cout << "font_extents.max_y_advance: " << font_extents.max_y_advance << '\n';
        std::cout << "baseline: " << baseline << '\n';
    }
    else
    {
        cairo_translate(cr, FONT_SIZE * .5, 0);
    }

    // 절대 위치로 변경했던것과 같이 그림
    cairo_glyph_t *cairo_glyphs = cairo_glyph_allocate(len);
    double current_x = 0;
    double current_y = 0;
    for (unsigned int i = 0; i < len; i++)
    {
        cairo_glyphs[i].index = info[i].codepoint;
        cairo_glyphs[i].x = current_x + pos[i].x_offset / 64.;
        cairo_glyphs[i].y = -(current_y + pos[i].y_offset / 64.);
        // cairo_glyphs[i].x = current_x;
        // cairo_glyphs[i].y = current_y;
        current_x += pos[i].x_advance / 64.;
        current_y += pos[i].y_advance / 64.;
    }

    cairo_show_glyphs(cr, cairo_glyphs, len);
    cairo_glyph_free(cairo_glyphs);

    cairo_surface_write_to_png(cairo_surface, "out.png");
}

void destroy()
{
    cairo_font_face_destroy(cairo_face);
    cairo_destroy(cr);
    cairo_surface_destroy(cairo_surface);

    hb_buffer_destroy(hb_buffer);
    hb_font_destroy(hb_font);

    FT_Done_Face(face);
    FT_Done_FreeType(library);

    FcPatternDestroy(font);  // needs to be called for every pattern created; in this case, 'fontFile' / 'file' is also freed
    FcPatternDestroy(pat);   // needs to be called for every pattern created
    FcConfigDestroy(config); // needs to be called for every config created
    FcFini();                // uninitializes Fontconfig
}

int main()
{
    std::string fontFile = my_fontconfig();
    my_freetype(fontFile);
    std::string newString = my_fribidi();
    my_harfbuzz(newString);
    my_cairo();

    destroy();
}
