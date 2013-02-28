
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

#include "pgf.h"
#include "bmp.h"

void put_f26(int value)
{
	double dv;

	dv = value;
	dv /= 64;

	printf(" %9.6f", dv);
}

void print_header(PGF_FONT *pgft)
{
	PGF_HEADER *h = pgft->ph;

	printf("# PGF header:\n");
	printf("# -----------------------------\n");
	printf("header_len: %04x\n", h->header_len);
	printf("version   : %d.%d\n", h->version, h->revision);
	printf("font name : %s\n", h->font_name);
	printf("font type : %s\n", h->font_type);
	printf("h_size    :"); put_f26(h->h_size); printf("\n");
	printf("v_size    :"); put_f26(h->v_size); printf("\n");
	printf("ascender  :"); put_f26(h->ascender); printf("\n");
	printf("descender :"); put_f26(h->descender); printf("\n");
	printf("h_res     :"); put_f26(h->h_res); printf("\n");
	printf("v_res     :"); put_f26(h->v_res); printf("\n");
	printf("bpp       : %d\n", h->bpp);
	printf("\n");

	printf("charmap   : len=%d bpe=%d\n", h->charmap_len, h->charmap_bpe);
	printf("charptr   : len=%d bpe=%d scale=%d\n", h->charptr_len, h->charptr_bpe, h->charptr_scale);
	printf("shadowmap : len=%d bpe=%d\n", h->shadowmap_len, h->shadowmap_bpe);
	printf("\n");

	printf("min charmap : %04x\n", h->charmap_min);
	printf("max charmap : %04x\n", h->charmap_max);
	printf("x shadowscale :"); put_f26(h->shadowscale_x); printf("\n");
	printf("y shadowscale :"); put_f26(h->shadowscale_y); printf("\n");
	printf("\n");

	printf("max hori_bearingX  :"); put_f26(h->max_h_bearingX);  printf("\n");
	printf("max hori_bearingY  :"); put_f26(h->max_h_bearingY);  printf("\n");
	printf("min vert_bearingX  :"); put_f26(h->min_v_bearingX);  printf("\n");
	printf("max vert_bearingY  :"); put_f26(h->max_v_bearingY);  printf("\n");
	printf("max hori_advance   :"); put_f26(h->max_h_advance);   printf("\n");
	printf("max vert_advance   :"); put_f26(h->max_v_advance);   printf("\n");
	printf("max hori_dimension :"); put_f26(h->max_h_dimension); printf("\n");
	printf("max vert_dimension :"); put_f26(h->max_v_dimension); printf("\n");
	printf("max glyph width  : %d\n", h->max_glyph_w);
	printf("max glyph height : %d\n", h->max_glyph_h);
	printf("\n");

	printf("dimension table len : %d\n", h->dimension_len);
	printf("bearingX  table len : %d\n", h->bearingX_len);
	printf("bearingY  table len : %d\n", h->bearingY_len);
	printf("advance   table len : %d\n", h->advance_len);
	printf("\n");
	printf("\n");

}

void print_dim_table(PGF_FONT *pgft)
{
	int i, n_dim;

	n_dim = pgft->ph->dimension_len;

	printf("# Dimension table: %d\n", n_dim);
	printf("# -----------------------------\n");

	for(i=0; i<n_dim; i++){
		printf("%3d: h =", i);
		put_f26(pgft->dimension[i].h);
		printf("    v =");
		put_f26(pgft->dimension[i].v);
		printf("\n");
	}
	printf("\n");
}

void print_bearingX_table(PGF_FONT *pgft)
{
	int i, n_bx;

	n_bx = pgft->ph->bearingX_len;

	printf("# Left bearing table: %d\n", n_bx);
	printf("# -----------------------------\n");

	for(i=0; i<n_bx; i++){
		printf("%3d: h =", i); put_f26(pgft->bearingX[i].h); 
		printf("    v ="); put_f26(pgft->bearingX[i].v);
		printf("\n");
	}
	printf("\n");
}

void print_bearingY_table(PGF_FONT *pgft)
{
	int i, n_by;

	n_by = pgft->ph->bearingY_len;

	printf("# Top bearing table: %d\n", n_by);
	printf("# -----------------------------\n");

	for(i=0; i<n_by; i++){
		printf("%3d: h =", i); put_f26(pgft->bearingY[i].h);
		printf("    v ="); put_f26(pgft->bearingY[i].v);
		printf("\n");
	}
	printf("\n");
}

void print_adv_table(PGF_FONT *pgft)
{
	int i, n_adv;

	n_adv = pgft->ph->advance_len;

	printf("# Advance table: %d\n", n_adv);
	printf("# -----------------------------\n");

	for(i=0; i<n_adv; i++){
		printf("%3d: h =", i); put_f26(pgft->advance[i].h);
		printf("    v ="); put_f26(pgft->advance[i].v);
		printf("\n");
	}
	printf("\n");
}

void print_shadowmap(PGF_FONT *pgft)
{
	int i, n_shadowmap;

	n_shadowmap = pgft->ph->shadowmap_len;

	printf("# Shadow map table: %d\n", n_shadowmap);
	printf("# -----------------------------\n");
	for(i=0; i<n_shadowmap; i++){
		printf("%04x\n", pgft->shadowmap[i]);
	}
	printf("\n");
}

void print_charmap(PGF_FONT *pgft)
{
	int i, n_charmap;

	n_charmap = pgft->ph->charptr_len;

	printf("# charmap table: %d\n", n_charmap);
	printf("# -----------------------------\n");
	for(i=0; i<65536; i++){
		if(pgft->char_glyph[i])
			printf("%04x\n", i);
	}
	printf("\n");
}

void print_charptr(PGF_FONT *pgft)
{
	int i, n_charptr;

	n_charptr = pgft->ph->charptr_len;

	printf("# charptr table: %d\n", n_charptr);
	printf("# -----------------------------\n");
	for(i=0; i<n_charptr; i++){
		printf("%4x : %08x\n", i, pgft->charptr[i]);
	}
	printf("\n");
}

void print_charinfo(PGF_FONT *pgft)
{
	PGF_GLYPH *glyph;
	int i, p;

	p = 0;
	for(i=0; i<65536; i++){
		glyph = pgft->char_glyph[i];
		if(glyph==NULL)
			continue;

		printf("\n---- %5d : U_%04x ----\n", p, i);
		printf("    dimension: h="); put_f26(glyph->dimension.h);
		printf(" v="); put_f26(glyph->dimension.v);
		printf("\n");
		printf("    bearingX : h="); put_f26(glyph->bearingX.h);
		printf(" v="); put_f26(glyph->bearingX.v);
		printf("\n");
		printf("    bearingY : h="); put_f26(glyph->bearingY.h);
		printf(" v="); put_f26(glyph->bearingY.v);
		printf("\n");
		printf("    advance  : h="); put_f26(glyph->advance.h);
		printf(" v="); put_f26(glyph->advance.v);
		printf("\n");
		printf("    bitmap: width=%d height=%d left=%d top=%d\n",
			glyph->width, glyph->height, glyph->left, glyph->top);
		printf("    shadow_id: U_%04x  shadow_flag: %d:%d:%d\n",
			pgft->shadowmap[glyph->shadow_id],
			(glyph->shadow_flag>>5)&3,
			(glyph->shadow_flag>>3)&3,
			(glyph->shadow_flag>>0)&7);

		p++;
	}
}

void save_bitmap(char *bmp_name, u8 *buf, int bw, int bh)
{
	BITMAPFILEHEADER bm_header;
	BITMAPINFOHEADER bm_info;
	FILE *fp;
	int i, fsize, dsize, psize;
	int bpp = 8;
	char *palbuf;

	dsize = bw*bh;
	psize = (1<<bpp)*4;
	fsize = 14+sizeof(BITMAPINFOHEADER)+psize+dsize;

	bm_header.bfType = 0x4D42;
	bm_header.bfSize = fsize;
	bm_header.bfReserved1 = 0;
	bm_header.bfReserved2 = 0;
	bm_header.bfOffBits = 14+sizeof(BITMAPINFOHEADER)+psize;

	bm_info.biSize = sizeof(BITMAPINFOHEADER);
	bm_info.biWidth = bw;
	bm_info.biHeight = bh;
	bm_info.biPlanes = 1;
	bm_info.biBitCount = bpp;
	bm_info.biCompression = BI_RGB;
	bm_info.biSizeImage = dsize;
	bm_info.biXPelsPerMeter = 0;
	bm_info.biYPelsPerMeter = 0;
	bm_info.biClrUsed = psize/4;
	bm_info.biClrImportant = 0;

	palbuf = (char*)malloc(psize);
	memset(palbuf, 0, psize);
	for(i=0; i<16*4; i+=4){
		palbuf[i+0] = i<<2;
		palbuf[i+1] = i<<2;
		palbuf[i+2] = i<<2;
		palbuf[i+3] = 0;
	}

	fp = fopen(bmp_name, "wb");
	fwrite(&bm_header, 14, 1, fp);
	fwrite(&bm_info, sizeof(BITMAPINFOHEADER), 1, fp);
	fwrite(palbuf, psize, 1, fp);
	for(i=bh-1; i>=0; i--){
		memset(palbuf, 0, bw+4);
		memcpy(palbuf, buf+i*bw, bw);
		fwrite(palbuf, (bw+3)&(~3), 1, fp);
	}

	fclose(fp);
	free(palbuf);
	printf("Save BMP : %s\n", bmp_name);
}

void render_glyph(PGF_GLYPH *glyph, u8 *buf, int ox, int oy, int pitch)
{
	int v;
	u8 *sbuf, *dbuf;

	ox += glyph->left;
	oy -= glyph->top;
	dbuf = buf+oy*pitch+ox;
	sbuf = glyph->bmp;

	for(v=0; v<glyph->height; v++){
		memcpy(dbuf, sbuf, glyph->width);
		dbuf += pitch;
		sbuf += glyph->width;
	}
}

void print_bitmap(PGF_FONT *pgft, char *basename)
{
	PGF_GLYPH *glyph;
	int p, h, v, empty_page;
	int cx, cy, xstep, ystep;
	char pagename[64];
	u8 *gbuf;
	int gw, gh, gsize;

	p = strlen(basename);
	while(p>=0){
		if(basename[p]=='.'){
			basename[p] = 0;
			break;
		}
		p--;
	}
	while(p>=0){
		if(basename[p]=='/' || basename[p]=='\\'){
			basename += p+1;
			break;
		}
		p--;
	}
	sprintf(pagename, "%s_bmp", basename);
#ifdef __MINGW32__
	mkdir(pagename);
#else
	mkdir(pagename, 0755);
#endif

	xstep = pgft->ph->max_glyph_w+1;
	ystep = pgft->ph->max_glyph_h+2;

	gw = xstep*17;
	gh = ystep*17;
	gsize = gw*gh;

	gbuf = (u8*)malloc(gsize);

	for(p=0; p<256; p++){
		cy = ystep;
		empty_page = 1;
		memset(gbuf, 0, gsize);

		for(v=0; v<16; v++){
			cx = 0;
			for(h=0; h<16; h++){
				int ucs = p*256+v*16+h;
				glyph = get_glyph(pgft, ucs);
				if(glyph==NULL){
					cx += xstep;
					continue;
				}
				empty_page = 0;
				int bx = (xstep-glyph->width)/2;
				render_glyph(glyph, gbuf, cx+bx, cy, gw);
				cx += xstep;
			}
			cy += ystep;
		}

		if(empty_page)
			continue;
		sprintf(pagename, "%s_bmp/%s_%02x.bmp", basename, basename, p);
		save_bitmap(pagename, gbuf, gw, gh);
	}

	free(gbuf);
}

int main(int argc, char *argv[])
{
	PGF_FONT *pgft;
	char *args, *font_name;
	int i;

	int dump_header = 0;
	int dump_metrics = 0;
	int dump_charmap = 0;
	int dump_charinfo = 0;
	int dump_shadow = 0;
	int dump_charptr = 0;
	int dump_bitmap = 0;

	if(argc==1){
		printf("PGF font info dumper V0.2\n");
		printf("-------------------------\n");
		printf("Usage:\n");
		printf("  %s [-h] [-m] [-c] [-i] [-s] [-p] [-b] {pgf file}\n", argv[0]);
		printf("\n");
		printf("      -h: dump font header\n");
		printf("      -m: dump metrics table\n");
		printf("      -c: dump chars map\n");
		printf("      -i: dump chars info\n");
		printf("      -s: dump shadow map\n");
		printf("      -p: dump chars pointer\n");
		printf("      -b: dump chars bitmap\n");
		return 0;
	}

	font_name = NULL;
	i = 0;
	while(i<argc){
		args = argv[i];
		if(args[0]=='-'){
			if(args[1]=='h')
				dump_header = 1;
			if(args[1]=='m')
				dump_metrics = 1;
			if(args[1]=='c')
				dump_charmap = 1;
			if(args[1]=='i')
				dump_charinfo = 1;
			if(args[1]=='s')
				dump_shadow = 1;
			if(args[1]=='p')
				dump_charptr = 1;
			if(args[1]=='b')
				dump_bitmap = 1;
		}else{
			font_name = args;
		}
		i++;
	}

	pgft = load_pgf_font(font_name);
	if(pgft==NULL){
		return -1;
	}

	if(dump_header){
		print_header(pgft);
	}

	if(dump_metrics){
		print_dim_table(pgft);
		print_bearingX_table(pgft);
		print_bearingY_table(pgft);
		print_adv_table(pgft);
	}

	if(dump_charmap){
		print_charmap(pgft);
	}

	if(dump_charptr){
		print_charptr(pgft);
	}

	if(dump_shadow){
		print_shadowmap(pgft);
	}

	if(dump_charinfo){
		print_charinfo(pgft);
	}

	if(dump_bitmap){
		print_bitmap(pgft, font_name);
	}

	return 0;
}

