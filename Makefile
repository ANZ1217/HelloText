HB_PKGS = harfbuzz
FT_PKGS = harfbuzz cairo-ft freetype2

HB_CFLAGS = `pkg-config --cflags $(HB_PKGS)`
HB_LDFLAGS = `pkg-config --libs $(HB_PKGS)` -lm

FT_CFLAGS = `pkg-config --cflags $(FT_PKGS)`
FT_LDFLAGS = `pkg-config --libs $(FT_PKGS)` -lm

all: hello_text

hello_text: main.c
	$(CC) -std=c99 -o $@ $^ $(FT_CFLAGS) $(FT_LDFLAGS)
