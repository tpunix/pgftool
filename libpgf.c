
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pgf.h"

int get_value(int bpe, u8 *buf, int *pos)
{
	int i, v;

	v = 0;
	for(i=0; i<bpe; i++){
		v += ( ( buf[(*pos)/8] >> ((*pos)%8) ) &1 ) << i;
		(*pos)++;
	}

	return v;
}

int read_table(FILE *fp, int *table, int num, int bpe)
{
	int i, p, len;
	u8 *raw;

	len = ((num*bpe+31)/32)*4;
	raw = malloc(len);
	fread(raw, len, 1, fp);

	p = 0;
	for(i=0; i<num; i++){
		table[i] = get_value(bpe, raw, &p);
	}

	free(raw);
	return 0;
}

int ptr2ucs(PGF_FONT *pgft, int ptr)
{
	int i, n_charmap;

	n_charmap = pgft->ph->charmap_len;
	for(i=0; i<n_charmap; i++){
		if(pgft->charmap[i]==ptr){
			return i+pgft->ph->charmap_min;
		}
	}

	return 0xffff;
}

int have_shadow(PGF_FONT *pgft, int ucs)
{
	int i, n_shadowmap;

	n_shadowmap = pgft->ph->shadowmap_len;
	for(i=0; i<n_shadowmap; i++){
		if(pgft->shadowmap[i]==ucs){
			return 1;
		}
	}

	return 0;
}

int get_bitmap(PGF_GLYPH *glyph)
{
	int i, j, p, nb, data, len;
	u8 *bmp;

	i = 0;
	p = 0;
	len = glyph->width * glyph->height;
	if((glyph->flag&3)==2){
		bmp = (u8*)malloc(len);
	}else{
		bmp = glyph->bmp;
	}

	while(i<len){
		nb = get_value(4, glyph->data, &p);
		if(nb<8){
			data = get_value(4, glyph->data, &p);
			for(j=0; j<nb+1; j++){
				bmp[i] = data;
				i++;
			}
		}else{
			for(j=0; j<(16-nb); j++){
				data = get_value(4, glyph->data, &p);
				bmp[i] = data;
				i++;
			}
		}
	}

	if((glyph->flag&3)==2){
		int h, v;

		i = 0;
		for(h=0; h<glyph->width; h++){
			for(v=0; v<glyph->height; v++){
				glyph->bmp[v*glyph->width+h] = bmp[i];
				i++;
			}
		}
		free(bmp);
	}

	return 0;
}

int load_shadow_glyph(u8 *ptr, PGF_GLYPH *glyph)
{
	int pos;

	pos = 0;

	glyph->size   = get_value(14, ptr, &pos);
	glyph->width  = get_value(7, ptr, &pos);
	glyph->height = get_value(7, ptr, &pos);
	glyph->left   = get_value(7, ptr, &pos);
	glyph->top    = get_value(7, ptr, &pos);
	glyph->flag   = get_value(6, ptr, &pos);

	if(glyph->left>63) glyph->left |= 0xffffff80;
	if(glyph->top >63) glyph->top  |= 0xffffff80;

	glyph->data = ptr+(pos/8);
	glyph->bmp = malloc(glyph->width*glyph->height);
	get_bitmap(glyph);

	return 0;
}

int load_char_glyph(PGF_FONT *pgft, int index, PGF_GLYPH *glyph)
{
	int id, pos;
	u8 *ptr;

	ptr = pgft->glyphdata + pgft->charptr[index];
	pos = 0;

	glyph->index = index;
	glyph->have_shadow = have_shadow(pgft, glyph->ucs);

	glyph->size   = get_value(14, ptr, &pos);
	glyph->width  = get_value( 7, ptr, &pos);
	glyph->height = get_value( 7, ptr, &pos);
	glyph->left   = get_value( 7, ptr, &pos);
	glyph->top    = get_value( 7, ptr, &pos);
	glyph->flag   = get_value( 6, ptr, &pos);

	if(glyph->left>63) glyph->left |= 0xffffff80;
	if(glyph->top >63) glyph->top  |= 0xffffff80;

	/* read extension info */
	glyph->shadow_flag = get_value(7, ptr, &pos);
	glyph->shadow_id   = get_value(9, ptr, &pos);
	if(glyph->flag&0x04){
		id = get_value(8, ptr, &pos);
		glyph->dimension.h = pgft->dimension[id].h;
		glyph->dimension.v = pgft->dimension[id].v;
	}else{
		glyph->dimension.h = get_value(32, ptr, &pos);
		glyph->dimension.v = get_value(32, ptr, &pos);
	}
	if(glyph->flag&0x08){
		id = get_value(8, ptr, &pos);
		glyph->bearingX.h = pgft->bearingX[id].h;
		glyph->bearingX.v = pgft->bearingX[id].v;
	}else{
		glyph->bearingX.h = get_value(32, ptr, &pos);
		glyph->bearingX.v = get_value(32, ptr, &pos);
	}
	if(glyph->flag&0x10){
		id = get_value(8, ptr, &pos);
		glyph->bearingY.h = pgft->bearingY[id].h;
		glyph->bearingY.v = pgft->bearingY[id].v;
	}else{
		glyph->bearingY.h = get_value(32, ptr, &pos);
		glyph->bearingY.v = get_value(32, ptr, &pos);
	}
	if(glyph->flag&0x20){
		id = get_value(8, ptr, &pos);
		glyph->advance.h = pgft->advance[id].h;
		glyph->advance.v = pgft->advance[id].v;
	}else{
		glyph->advance.h = get_value(32, ptr, &pos);
		glyph->advance.v = get_value(32, ptr, &pos);
	}

	glyph->data = ptr+(pos/8);
	glyph->bmp = malloc(glyph->width*glyph->height);
	get_bitmap(glyph);

	if(glyph->have_shadow){
		id = glyph->shadow_id;
		pgft->shadow_glyph[id] = (PGF_GLYPH*)malloc(sizeof(PGF_GLYPH));
		memset(pgft->shadow_glyph[id], 0, sizeof(PGF_GLYPH));
		load_shadow_glyph(ptr+glyph->size, pgft->shadow_glyph[id]);
	}

	return 0;
}


int load_all_glyph(PGF_FONT *pgft, int *list)
{
	PGF_GLYPH *glyph;
	int i, n_chars, ucs;

	n_chars = pgft->ph->charptr_len;
	for(i=0; i<n_chars; i++){
		glyph = (PGF_GLYPH*)malloc(sizeof(PGF_GLYPH));
		memset(glyph, 0, sizeof(PGF_GLYPH));
		ucs = ptr2ucs(pgft, i);
		if(list[ucs]==0){
			continue;
		}
		glyph->ucs = ucs;
		pgft->char_glyph[ucs] = glyph;
		load_char_glyph(pgft, i, glyph);
	}

	return 0;
}


int load_ucs_list(char *list_name, int *list)
{
	FILE *lfp;
	char lbuf[128];
	int i, ucs;

	lfp = fopen(list_name, "rb");
	if(lfp==NULL){
		//fprintf(stderr, "Open file %s failed!\n", list_name);
		for(i=0; i<65536; i++){
			list[i] = i;
		}
		return 0;
	}

	i = 0;
	while(fgets(lbuf, 64, lfp)){
		if(lbuf[0]=='#' || lbuf[0]==0 || lbuf[0]=='\n' || lbuf[0]=='\r')
			continue;
		ucs = 0;
		sscanf(lbuf, "%x", &ucs);
		list[ucs] = ucs;
		i++;
	}

	fprintf(stderr, "Load list from %s : %d\n", list_name, i);
	return i;
}


PGF_FONT *load_pgf_font(char *font_name)
{
	PGF_FONT *pgft;
	FILE *fp;
	int i, pos, fsize, len;
	int ucs_list[65536];
	char list_name[128];

	pgft = (PGF_FONT*)malloc(sizeof(PGF_FONT));
	memset(pgft, 0, sizeof(PGF_FONT));

	fp = fopen(font_name, "rb");
	if(fp==NULL){
		printf("Open font %s error!\n", font_name);
		return NULL;
	}

	/* read pgf header */
	pgft->ph = (PGF_HEADER*)malloc(sizeof(PGF_HEADER));
	fread(pgft->ph, sizeof(PGF_HEADER), 1, fp);

	len = pgft->ph->header_len;
	fseek(fp, len, SEEK_SET);

	/* read dimension table */
	len = pgft->ph->dimension_len;
	pgft->dimension = (F26_PAIRS*)malloc(len*8);
	fread(pgft->dimension, len*8, 1, fp);

	/* read left bearing table */
	len = pgft->ph->bearingX_len;
	pgft->bearingX = (F26_PAIRS*)malloc(len*8);
	fread(pgft->bearingX, len*8, 1, fp);

	/* read top bearing table */
	len = pgft->ph->bearingY_len;
	pgft->bearingY = (F26_PAIRS*)malloc(len*8);
	fread(pgft->bearingY, len*8, 1, fp);

	/* read advance table */
	len = pgft->ph->advance_len;
	pgft->advance = (F26_PAIRS*)malloc(len*8);
	fread(pgft->advance, len*8, 1, fp);

	/* read shadowmap table */
	len = pgft->ph->shadowmap_len;
	if(len){
		pgft->shadowmap = (int*)malloc(len*4);
		read_table(fp, pgft->shadowmap, len, pgft->ph->shadowmap_bpe);
	}

	/* read charmap table */
	len = pgft->ph->charmap_len;
	pgft->charmap = (int*)malloc(len*4);
	read_table(fp, pgft->charmap, len, pgft->ph->charmap_bpe);

	/* read charptr table */
	len = pgft->ph->charptr_len;
	pgft->charptr = (int*)malloc(len*4);
	read_table(fp, pgft->charptr, len, pgft->ph->charptr_bpe);
	for(i=0; i<len; i++){
		pgft->charptr[i] *= pgft->ph->charptr_scale;
	}

	/* read font glyph data */
	pos = ftell(fp);
	fseek(fp, 0, SEEK_END);
	fsize = ftell(fp);
	fseek(fp, pos, SEEK_SET);
	len = fsize-pos;
	pgft->glyphdata = (u8*)malloc(len+4096);
	fread(pgft->glyphdata, len, 1, fp);
	fclose(fp);

	memset(ucs_list, 0, 65536*4);
	sprintf(list_name, "%s.txt", font_name);
	load_ucs_list(list_name, ucs_list);
	load_all_glyph(pgft, ucs_list);

	free(pgft->glyphdata);
	pgft->glyphdata = 0;

	return pgft;
}

PGF_GLYPH *get_glyph(PGF_FONT *pgft, int ucs)
{
	return pgft->char_glyph[ucs];
}

void free_glyph(PGF_FONT *pgft, PGF_GLYPH *glyph)
{
	int ucs;

	ucs = glyph->ucs;
	free(glyph->bmp);
	free(glyph);
	pgft->char_glyph[ucs] = 0;
}

PGF_FONT *new_pgf_font(void)
{
	PGF_FONT *pgft;
	PGF_HEADER *ph;

	pgft = (PGF_FONT*)malloc(sizeof(PGF_FONT));
	memset(pgft, 0, sizeof(PGF_FONT));

	ph = (PGF_HEADER*)malloc(sizeof(PGF_HEADER));
	memset(ph, 0, sizeof(PGF_HEADER));
	pgft->ph = ph;

	ph->header_start = 0;
	ph->header_len = 0x0188;
	strncpy((char*)ph->pgf_id, "PGF0", 4);
	ph->revision = 0x00000002;
	ph->version = 0x00000006;

	ph->unk_20[0] = 0x04;
	ph->unk_20[1] = 0x04;
	ph->bpp = 0x04;

	ph->unk_BC = 0x00010000;
	ph->unk_C8 = 0x00010000;

	ph->charptr_scale = 0x0004;
	ph->shadowmap_bpe = 0x00000010;

	ph->unk_174 = 0x00000604;
	ph->shadowscale_x = 0x00000020;
	ph->shadowscale_y = 0x00000020;

	return pgft;
}

