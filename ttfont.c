
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "pgf.h"

typedef struct TTF_GLYPH_T {
	int ucs;

	int width;
	int height;
	int left;
	int top;

	F26_PAIRS dimension;
	F26_PAIRS bearingX;
	F26_PAIRS bearingY;
	F26_PAIRS advance;

	u8 *bmp;
} TTF_GLYPH;


FT_Library ft_lib = NULL;

int ucs_list[65536];
int nucs;

FT_Face TTF_Init(char *name, int hsize, int vsize)
{
	int retv;
	FT_Face face;

	if(ft_lib==NULL){
		retv = FT_Init_FreeType(&ft_lib);
		if(retv){
			printf("FT_Init_Freetype failed!\n");
			return NULL;
		}
	}

	retv = FT_New_Face(ft_lib, name, 0, &face);
	if(retv){
		printf("FT_New_Face(%s): error=%08x\n", name, retv);
		return NULL;
	}

	retv = FT_Set_Pixel_Sizes(face, hsize, vsize);
	if(retv){
		printf("FT_Set_Pixel_Size: error=%08x\n", retv);
		return NULL;
	}

	return face;
}

TTF_GLYPH *TTF_Load_Glyph(FT_Face face, int ucs)
{
	FT_Bitmap bm;
	FT_Glyph_Metrics mt;
	TTF_GLYPH *pg;
	int index, retv;
	int bw, bh, bx, by, pitch, h, v;
	u8 *sbuf, *dbuf;

	index = FT_Get_Char_Index(face, ucs);
	if(index==0){
		return NULL;
	}

	retv = FT_Load_Glyph(face, index, FT_LOAD_DEFAULT|FT_LOAD_NO_HINTING|FT_LOAD_NO_BITMAP);
	if(retv){
		printf("FT_Load_Gluph(%04x): error=%08x\n", ucs, retv);
		return NULL;
	}

	if(face->glyph->format!=FT_GLYPH_FORMAT_BITMAP){
		retv = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
		if(retv){
			printf("FT_Render_Glyph(%04x): error=%08x\n", ucs, retv);
			return NULL;
		}
	}

	bm = face->glyph->bitmap;
	mt = face->glyph->metrics;

	pitch = bm.pitch;
	bw = bm.width;
	bh = bm.rows;
	bx = face->glyph->bitmap_left;
	by = face->glyph->bitmap_top;

	pg = (TTF_GLYPH*)malloc(sizeof(TTF_GLYPH));

	pg->ucs = ucs;
	pg->width = bw;
	pg->height = bh;
	pg->left = bx;
	pg->top = by;
	pg->dimension.h = mt.width;
	pg->dimension.v = mt.height;
	pg->bearingX.h  = mt.horiBearingX;
	pg->bearingX.v  = mt.vertBearingX;
	pg->bearingY.h  = mt.horiBearingY;
	pg->bearingY.v  = mt.vertBearingY;
	pg->advance.h   = mt.horiAdvance;
	pg->advance.v   = mt.vertAdvance;

	pg->bmp = malloc(bw*bh);
	dbuf = pg->bmp;
	sbuf = bm.buffer;
	for(v=0; v<bh; v++){
		for(h=0; h<bw; h++){
			dbuf[h] = sbuf[h]>>4;
#if 0
			dbuf[h] = (sbuf[h]>>4)*2;
			if(dbuf[h]>15)
				dbuf[h] = 15;
#endif
		}
		sbuf += bw;
		dbuf += pitch;
	}

	return pg;
}

void put_f26(int value)
{
    double dv;

    dv = value;
    dv /= 64;

    printf(" %9.6f", dv);
}

int face_to_pgf(FT_Face face, PGF_FONT *pgft)
{
	FT_Size_Metrics *mt;
	int h_size, h_res;

	mt = &face->size->metrics;

	strcpy(pgft->ph->font_name, face->family_name);
	strcpy(pgft->ph->font_type, face->style_name);

	h_size = mt->x_ppem;
	h_res = 128;
	pgft->ph->h_res = h_res<<6;
	pgft->ph->v_res = h_res<<6;
	pgft->ph->h_size = (h_size<<6)*72/h_res;
	pgft->ph->v_size = (h_size<<6)*72/h_res;

	//pgft->ph->ascender = mt->ascender;
	//pgft->ph->descender = mt->descender;
	pgft->ph->ascender = 0x000003f5;
	pgft->ph->descender = 0xffffff75;

	return 0;
}

int main(int argc, char *argv[])
{
	FT_Face face;
	PGF_FONT *pgft;
	PGF_GLYPH *glyph;
	TTF_GLYPH **glist;
	int i;

	if(argc<3){
		printf("Usage: ttf_pgf {xxx.ttf} {out.pgf} [unicode list]\n");
		return 0;
	}

	memset(ucs_list, 0, 65536*4);
	nucs = load_ucs_list(argv[3], ucs_list);
	if(nucs==0){
		memset(ucs_list, 1, 65536*4);
	}

	/* 18x18 TrueType */
	face = TTF_Init(argv[1], 18, 18);
	if(face==NULL){
		return -1;
	}

	glist = (TTF_GLYPH**)malloc(65536*sizeof(TTF_GLYPH*));
	memset(glist, 0, 65536*sizeof(TTF_GLYPH*));
	for(i=0; i<65536; i++){
		if(ucs_list[i])
			glist[i] = TTF_Load_Glyph(face, i);
	}

	/* create pgf */
	pgft = new_pgf_font();
	face_to_pgf(face, pgft);

	/* TTF to PGF */
	for(i=0; i<65536; i++){
		if(glist[i]==NULL)
			continue;
		glyph = (PGF_GLYPH*)malloc(sizeof(PGF_GLYPH));
		memset(glyph, 0, sizeof(PGF_GLYPH));
		glyph->ucs = glist[i]->ucs;
		glyph->width = glist[i]->width;
		glyph->height = glist[i]->height;
		glyph->left = glist[i]->left;
		glyph->top = glist[i]->top;
		glyph->dimension = glist[i]->dimension;
		glyph->bearingX = glist[i]->bearingX;
		glyph->bearingY = glist[i]->bearingY;
		glyph->advance = glist[i]->advance;
		glyph->bmp = glist[i]->bmp;
		pgft->char_glyph[i] = glyph;
	}

	/* save */
	save_pgf(pgft, argv[2]);

	FT_Done_Face(face);
	for(i=0; i<nucs; i++){
		if(glist[i]){
			if(glist[i]->bmp)
				free(glist[i]->bmp);
			free(glist[i]);
			glist[i] = 0;
		}
	}

	return 0;
}

