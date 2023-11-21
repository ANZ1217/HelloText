#include <ft2build.h>
#include FT_FREETYPE_H

#include <iostream>
#include <string>

int main()
{
    FT_Library library;
    FT_Error error;

    // library 초기화
    if (error = FT_Init_FreeType(&library)) abort();

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
    if(error = FT_Set_Char_Size(
       face,
       0, // char_width 0일시 height와 동기화됨
       16*64, // char_height, 16pt
       300, // horizontal device 해상도
       300 // vertical device 해상도
    )) abort();

    /*
    fontsize를 pixel로 설정
    error = FT_Set_Pixel_Sizes(face, 0, 16);
    */

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
}
