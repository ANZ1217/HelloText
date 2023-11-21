#include <ft2build.h>
#include FT_FREETYPE_H
// for freetype

#include <hb.h>
#include <hb-ft.h>
// for harfbuzz

#include <iostream>
#include <string>

int main()
{
    FT_Library library;
    FT_Error error;

    // library 초기화
    if (error = FT_Init_FreeType(&library))
        abort();

    // font file로 부터 font face 로딩
    FT_Face face;
    if (error = FT_New_Face(library,
                            "../NotoSans-Regular.ttf",
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
            0,       // char_width 0일시 height와 동기화됨
            16 * 64, // char_height, 16pt
            300,     // horizontal device 해상도
            300      // vertical device 해상도
            ))
        abort();

    /*
    fontsize를 pixel로 설정
    error = FT_Set_Pixel_Sizes(face, 0, 16);
    */

    /*
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

    // Size 조정 직접 하지 않고 HarfBuzz 사용해보기

    // hb create
    hb_font_t *hb_font;
    hb_font = hb_ft_font_create(face, NULL);

    // hb buffer create
    hb_buffer_t *hb_buffer;
    hb_buffer = hb_buffer_create();

    std::string str = "Hello, Text!";

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
    hb_glyph_info_t *info = hb_buffer_get_glyph_infos(hb_buffer, NULL);
    hb_glyph_position_t *pos = hb_buffer_get_glyph_positions(hb_buffer, NULL);

    // Raw data 출력
    std::cout << "Raw buffer: \n";
    for (unsigned int i = 0; i < len; i++)
    {
        hb_codepoint_t gid = info[i].codepoint; // Shaping 전 Unicode code point, Shaping 후 Glyph index
        unsigned int cluster = info[i].cluster; // 현재 이 글자가 original text의 몇번째 index에 매칭되는지?
        double x_advance = pos[i].x_advance / 64.; // 앞뒤 간격 포함 총 너비
        double y_advance = pos[i].y_advance / 64.; // 위아래 총 너비
        double x_offset = pos[i].x_offset / 64.;
        double y_offset = pos[i].y_offset / 64.;

        char glyphname[32];
        hb_font_get_glyph_name(hb_font, gid, glyphname, sizeof(glyphname));

        std::cout << "Glyph: " << glyphname << '\n';
        std::cout << "Gid: " << gid << '\n';
        std::cout << "Cluster: " << cluster << '\n';
        std::cout << "Advance: (" << x_advance << ", " << y_advance << ")\n";
        std::cout << "Offset: (" << x_offset << ", " << y_offset << ")\n";
        std::cout << '\n';
    }

    printf("Converted to absolute positions:\n");
    /* And converted to absolute positions. */
    {
        double current_x = 300;
        double current_y = 200;
        for (unsigned int i = 0; i < len; i++)
        {
            hb_codepoint_t gid = info[i].codepoint;
            unsigned int cluster = info[i].cluster;
            double x_position = current_x + pos[i].x_offset / 64.;
            double y_position = current_y + pos[i].y_offset / 64.;

            char glyphname[32];
            hb_font_get_glyph_name(hb_font, gid, glyphname, sizeof(glyphname));

            std::cout << "Glyph: " << glyphname << '\n';
            std::cout << "Gid: " << gid << '\n';
            std::cout << "Cluster: " << cluster << '\n';
            std::cout << "Position: (" << x_position << ", " << y_position << ")\n";
            std::cout << '\n';

            current_x += pos[i].x_advance / 64.;
            current_y += pos[i].y_advance / 64.;
        }
    }
}
