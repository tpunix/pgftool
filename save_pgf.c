
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pgf.h"

typedef struct metrics_table_t {
	F26_PAIRS table[65536];
	int freq[65536];
	int total;
}METRICS_TABLE;

/********************************************************************/

void put_value(int nbit, u8 *buf, int data, int *ptr)
{
	int i;
	unsigned char mask, bit;

	for(i=0; i<nbit; i++){
		mask = 1<<((*ptr)%8);
		bit = ((data>>i)<<((*ptr)%8))&mask;
		buf[(*ptr)/8] &= ~mask;
		buf[(*ptr)/8] |= bit;
		(*ptr)++;
	}
	
}

/********************************************************************/

int rle_data(u8 *out, u8 *bmp, int bmp_len)
{
	int i, j, k, size, rcnt, scnt, rlen, slen;

	size = 0;
	i = 0;
	while(i<bmp_len){
		k = i;
		rcnt = 0;
		scnt = 0;
		while(k<bmp_len){
			rlen = 0;
			slen = 0;

			for(j=k+1; (j<k+9 && j<bmp_len); j++){
				if(bmp[j-1]==bmp[j]){
					j--;
					break;
				}
			}
			rlen = j-k;

			for(j=k+1; (j<k+8 && j<bmp_len); j++){
				if(bmp[j-1]!=bmp[j]){
					break;
				}
			}
			slen = j-k;

			if(slen>2){
				scnt = slen;
				break;
			}else if(slen==2){
				scnt += 2;
				k += slen;
				if(scnt>6 || rcnt==0)
					break;
			}else{
				rcnt += rlen+scnt;
				k += rlen;
				scnt = 0;
				if(rcnt>7)
					break;
			}
		}

		if(rcnt>8) rcnt = 8;
		if(scnt>8) scnt = 8;

		if(rcnt){
			put_value(4, out, (16-rcnt), &size);
			for(j=0; j<rcnt; j++)
				put_value(4, out, bmp[i+j], &size);
			i += rcnt;
		}else if(scnt){
			put_value(4, out, scnt-1, &size);
			put_value(4, out, bmp[i], &size);
			i += scnt;
		}
	}

	return size;
}

int find_in_table(F26_PAIRS *table, int total, F26_PAIRS *fp)
{
	int i;

	for(i=0; i<total; i++){
		if(table[i].h==fp->h && table[i].v==fp->v)
			break;
	}

	if(i==total)
		return -1;

	return i;
}

void build_glyph_data(PGF_FONT *pgft)
{
	PGF_GLYPH *glyph;
	u8 *hbuf, *vbuf, *bmp;
	int hsize, vsize, gsize;
	int bmp_len, p, h, v;
	int i;

	bmp = (u8*)malloc(64*64);

	for(i=0; i<65536; i++){
		glyph = pgft->char_glyph[i];
		if(glyph==NULL){
			continue;
		}

		glyph->flag = 0;
		gsize = 40;

		/* build metrics flag */
		glyph->dim_id = find_in_table(pgft->dimension, pgft->ph->dimension_len, &glyph->dimension);
		if(glyph->dim_id>=0){
			glyph->flag |= 0x04;
			gsize -= 7;
		}

		glyph->bx_id  = find_in_table(pgft->bearingX , pgft->ph->bearingX_len , &glyph->bearingX);
		if(glyph->bx_id>=0){
			glyph->flag |= 0x08;
			gsize -= 7;
		}

		glyph->by_id  = find_in_table(pgft->bearingY , pgft->ph->bearingY_len , &glyph->bearingY);
		if(glyph->by_id>=0){
			glyph->flag |= 0x10;
			gsize -= 7;
		}

		glyph->adv_id = find_in_table(pgft->advance  , pgft->ph->advance_len  , &glyph->advance);
		if(glyph->adv_id>=0){
			glyph->flag |= 0x20;
			gsize -= 7;
		}

		glyph->size = gsize;

		/* build rle data */
		bmp_len = glyph->width*glyph->height;
		hbuf = (u8*)malloc(bmp_len+40);
		vbuf = (u8*)malloc(bmp_len+40);
		hsize = 0;
		vsize = 0;

		/* horizontal rle */
		memcpy(bmp, glyph->bmp, bmp_len);
		hsize = rle_data(hbuf+gsize, bmp, bmp_len);

		/* vertical rle */
		p = 0;
		for(h=0; h<glyph->width; h++)
			for(v=0; v<glyph->height; v++)
				bmp[p++] = glyph->bmp[v*glyph->width+h];
		vsize = rle_data(vbuf+gsize, bmp, bmp_len);

		if(hsize<=vsize){
			glyph->flag |= 0x01;
			glyph->size += (hsize+7)/8;
			glyph->data = hbuf;
			free(vbuf);
		}else{
			glyph->flag |= 0x02;
			glyph->size += (vsize+7)/8;
			glyph->data = vbuf;
			free(hbuf);
		}

		/* build glyph header */
		p = 0;
		put_value(14, glyph->data, glyph->size, &p);
		put_value( 7, glyph->data, glyph->width, &p);
		put_value( 7, glyph->data, glyph->height, &p);
		put_value( 7, glyph->data, glyph->left, &p);
		put_value( 7, glyph->data, glyph->top, &p);
		put_value( 6, glyph->data, glyph->flag, &p);
		put_value( 7, glyph->data, glyph->shadow_flag, &p);
		put_value( 9, glyph->data, glyph->shadow_id, &p);

		/* build metrics data */
		p = 8;
		if(glyph->flag&0x04){
			glyph->data[p] = glyph->dim_id;
			p ++;
		}else{
			memcpy(glyph->data+p, &glyph->dimension, 8);
			p += 8;
		}
		if(glyph->flag&0x08){
			glyph->data[p] = glyph->bx_id;
			p ++;
		}else{
			memcpy(glyph->data+p, &glyph->bearingX, 8);
			p += 8;
		}
		if(glyph->flag&0x10){
			glyph->data[p] = glyph->by_id;
			p ++;
		}else{
			memcpy(glyph->data+p, &glyph->bearingY, 8);
			p += 8;
		}
		if(glyph->flag&0x20){
			glyph->data[p] = glyph->adv_id;
			p ++;
		}else{
			memcpy(glyph->data+p, &glyph->advance, 8);
			p += 8;
		}

	}

	free(bmp);
}

/********************************************************************/

void add_to_table(METRICS_TABLE *mt, F26_PAIRS *fp)
{
	int i, p;

	for(i=0; i<mt->total; i++){
		if(mt->table[i].h==fp->h && mt->table[i].v==fp->v){
			mt->freq[i] ++;
			break;
		}
	}

	if(i==mt->total){
		mt->table[i].h = fp->h;
		mt->table[i].v = fp->v;
		mt->freq[i] = 1;
		mt->total = i+1;
		return;
	}

	p = i;
	for(i=0; i<p; i++){
		if(mt->freq[i]<mt->freq[p]){
			F26_PAIRS temp = mt->table[i];
			int tempf = mt->freq[i];
			mt->table[i] = mt->table[p];
			mt->table[p] = temp;
			mt->freq[i] = mt->freq[p];
			mt->freq[p] = tempf;
			return;
		}
	}
}

void build_metrics_table(PGF_FONT *pgft)
{
	PGF_GLYPH *glyph;
	int i, tl;

	METRICS_TABLE *dim = (METRICS_TABLE *)malloc(sizeof(METRICS_TABLE));
	METRICS_TABLE *bx  = (METRICS_TABLE *)malloc(sizeof(METRICS_TABLE));
	METRICS_TABLE *by  = (METRICS_TABLE *)malloc(sizeof(METRICS_TABLE));
	METRICS_TABLE *adv = (METRICS_TABLE *)malloc(sizeof(METRICS_TABLE));

	memset(dim, 0, sizeof(METRICS_TABLE));
	memset(bx , 0, sizeof(METRICS_TABLE));
	memset(by , 0, sizeof(METRICS_TABLE));
	memset(adv, 0, sizeof(METRICS_TABLE));

	for(i=0; i<65536; i++){
		glyph = pgft->char_glyph[i];
		if(glyph==NULL){
			continue;
		}
		add_to_table(dim, &glyph->dimension);
		add_to_table(bx , &glyph->bearingX);
		add_to_table(by , &glyph->bearingY);
		add_to_table(adv, &glyph->advance);
	}

	tl = (dim->total>255)? 255 : dim->total;
	pgft->ph->dimension_len = tl;
	pgft->dimension = (F26_PAIRS *)malloc(tl*sizeof(F26_PAIRS));
	for(i=0; i<tl; i++){
		pgft->dimension[i] = dim->table[i];
	}

	tl = (bx->total>255)? 255 : bx->total;
	pgft->ph->bearingX_len = tl;
	pgft->bearingX = (F26_PAIRS *)malloc(tl*sizeof(F26_PAIRS));
	for(i=0; i<tl; i++){
		pgft->bearingX[i] = bx->table[i];
	}

	tl = (by->total>255)? 255 : by->total;
	pgft->ph->bearingY_len = tl;
	pgft->bearingY = (F26_PAIRS *)malloc(tl*sizeof(F26_PAIRS));
	for(i=0; i<tl; i++){
		pgft->bearingY[i] = by->table[i];
	}

	tl = (adv->total>255)? 255 : adv->total;
	pgft->ph->advance_len = tl;
	pgft->advance = (F26_PAIRS *)malloc(tl*sizeof(F26_PAIRS));
	for(i=0; i<tl; i++){
		pgft->advance[i] = adv->table[i];
	}

}

/********************************************************************/

void find_max_min(PGF_FONT *pgft)
{
	PGF_GLYPH *glyph;
	int i, ucsmin, ucsmax;
	int n_chars;

	int max_h_bearingX  = 0;
	int max_h_bearingY  = 0;
	int min_v_bearingX  = 1000000;
	int max_v_bearingY  = 0;
	int max_h_advance   = 0;
	int max_v_advance   = 0;
	int max_h_dimension = 0;
	int max_v_dimension = 0;
	int max_glyph_w     = 0;
	int max_glyph_h     = 0;

	ucsmin = -1;
	ucsmax = -1;
	n_chars = 0;

	/* find the max & min value */
	for(i=0; i<65536; i++){
		glyph = pgft->char_glyph[i];
		if(glyph==NULL){
			continue;
		}
		if(ucsmin==-1){
			ucsmin = i;
		}
		ucsmax = i;
		n_chars += 1;

		glyph->bearingX.h &= 0xfffffff0;
		if(glyph->bearingX.h>max_h_bearingX)
			max_h_bearingX = glyph->bearingX.h;
		glyph->bearingX.v &= 0xfffffff0;
		if(glyph->bearingX.v<min_v_bearingX)
			min_v_bearingX = glyph->bearingX.v;

		glyph->bearingY.h &= 0xfffffff0;
		if(glyph->bearingY.h>max_h_bearingY)
			max_h_bearingY = glyph->bearingY.h;
		glyph->bearingY.v &= 0xfffffff0;
		if(glyph->bearingY.v>max_v_bearingY)
			max_v_bearingY = glyph->bearingY.v;

		glyph->advance.h &= 0xfffffff0;
		if(glyph->advance.h>max_h_advance)
			max_h_advance = glyph->advance.h;
		glyph->advance.v &= 0xfffffff0;
		if(glyph->advance.v>max_v_advance)
			max_v_advance = glyph->advance.v;

		glyph->dimension.h &= 0xfffffff0;
		if(glyph->dimension.h>max_h_dimension)
			max_h_dimension = glyph->dimension.h;
		glyph->dimension.v &= 0xfffffff0;
		if(glyph->dimension.v>max_v_dimension)
			max_v_dimension = glyph->dimension.v;

		if(glyph->width>max_glyph_w)
			max_glyph_w = glyph->width;
		if(glyph->height>max_glyph_h)
			max_glyph_h = glyph->height;
	}

	pgft->ph->max_h_bearingX  = max_h_bearingX;
	pgft->ph->max_h_bearingY  = max_h_bearingY;
	pgft->ph->min_v_bearingX  = min_v_bearingX;
	pgft->ph->max_v_bearingY  = max_v_bearingY;
	pgft->ph->max_h_advance   = max_h_advance;
	pgft->ph->max_v_advance   = max_v_advance;
	pgft->ph->max_h_dimension = max_h_dimension;
	pgft->ph->max_v_dimension = max_v_dimension;
	pgft->ph->max_glyph_w     = max_glyph_w;
	pgft->ph->max_glyph_h     = max_glyph_h;

	pgft->ph->charptr_len = n_chars;
	pgft->ph->charmap_min = ucsmin;
	pgft->ph->charmap_max = ucsmax;
	pgft->ph->charmap_len = ucsmax-ucsmin+1;

	pgft->ph->shadowmap_len = 0;
}

/********************************************************************/

void release_tables(PGF_FONT *pgft)
{
	/* release all tables first, we will rebuild them. */
	if(pgft->dimension){
		free(pgft->dimension);
		pgft->dimension = 0;
	}
	if(pgft->bearingX){
		free(pgft->bearingX);
		pgft->bearingX = 0;
	}
	if(pgft->bearingY){
		free(pgft->bearingY);
		pgft->bearingY = 0;
	}
	if(pgft->advance){
		free(pgft->advance);
		pgft->advance = 0;
	}
	if(pgft->charptr){
		free(pgft->charptr);
		pgft->charptr = 0;
	}
	if(pgft->glyphdata){
		free(pgft->glyphdata);
		pgft->glyphdata = 0;
	}
}

/********************************************************************/

void write_glyph(PGF_FONT *pgft, FILE *fp)
{
	PGF_GLYPH *glyph;
	int i, gsize;

	for(i=0; i<65536; i++){
		glyph = pgft->char_glyph[i];
		if(glyph==NULL){
			continue;
		}
		gsize = glyph->size;
		gsize = (gsize+3)&0xfffffffc;
		fwrite(glyph->data, gsize, 1, fp);
	}
}

void write_charptr(PGF_FONT *pgft, FILE *fp)
{
	PGF_GLYPH *glyph;
	int i, n_chars, scale, bpe, gsize, p;
	u8 *pbuf;

	n_chars = pgft->ph->charptr_len;
	pgft->charptr = (int*)malloc((n_chars+1)*sizeof(int));
	scale = pgft->ph->charptr_scale;

	p = 0;
	pgft->charptr[0] = 0;
	for(i=0; i<65536; i++){
		glyph = pgft->char_glyph[i];
		if(glyph==NULL){
			continue;
		}
		gsize = glyph->size;
		gsize = (gsize+scale-1)/scale;
		pgft->charptr[p+1] = pgft->charptr[p]+gsize;
		p++;
	}

	bpe = 0;
	gsize = pgft->charptr[n_chars-1];
	while(gsize){
		bpe ++;
		gsize >>= 1;
	}
	pgft->ph->charptr_bpe = bpe;

	gsize = ((n_chars*bpe+31)/32)*4;
	pbuf = (u8*)malloc(gsize);
	p = 0;

	for(i=0; i<n_chars; i++){
		put_value(bpe, pbuf, pgft->charptr[i], &p);
	}

	fwrite(pbuf, gsize, 1, fp);
	free(pbuf);
}

void write_charmap(PGF_FONT *pgft, FILE *fp)
{
	int i, n, bpe, msize;
	int cmin, cmax, p;
	u8 *mbuf;
	int ucs_table[65536];

	for(i=0; i<65536; i++){
		ucs_table[i] = -1;
	}

	p = 0;
	for(i=0; i<65536; i++){
		if(pgft->char_glyph[i]){
			ucs_table[i] = p;
			p++;
		}
	}

	bpe = 0;
	n = pgft->ph->charptr_len;
	while(n){
		bpe ++;
		n >>= 1;
	}
	pgft->ph->charmap_bpe = bpe;

	cmin = pgft->ph->charmap_min;
	cmax = pgft->ph->charmap_max;
	n = cmax-cmin+1;

	msize = ((n*bpe+31)/32)*4;
	mbuf = (u8*)malloc(msize);

	p = 0;
	for(i=cmin; i<=cmax; i++){
		put_value(bpe, mbuf, ucs_table[i], &p);
	}

	fwrite(mbuf, msize, 1, fp);
	free(mbuf);
}

void write_metrics_tables(PGF_FONT *pgft, FILE *fp)
{
	fwrite(pgft->dimension, 8, pgft->ph->dimension_len, fp);
	fwrite(pgft->bearingX , 8, pgft->ph->bearingX_len , fp);
	fwrite(pgft->bearingY , 8, pgft->ph->bearingY_len , fp);
	fwrite(pgft->advance  , 8, pgft->ph->advance_len  , fp);
}


void write_header(PGF_FONT *pgft, FILE *fp)
{
	fseek(fp, 0, SEEK_SET);
	fwrite(pgft->ph, pgft->ph->header_len, 1, fp);
}

/********************************************************************/

int save_pgf(PGF_FONT *pgft, char *font_name)
{
	FILE *fp;

	fp = fopen(font_name, "wb");
	if(fp==NULL){
		printf("Open file %s failed!\n", font_name);
		return -1;
	}

	release_tables(pgft);
	find_max_min(pgft);
	build_metrics_table(pgft);
	build_glyph_data(pgft);

	write_header(pgft, fp);
	write_metrics_tables(pgft, fp);
	write_charmap(pgft, fp);
	write_charptr(pgft, fp);
	write_glyph(pgft, fp);
	write_header(pgft, fp);

	fclose(fp);

	return 0;
}

#if 0
int main(int argc, char *argv[])
{
	PGF_FONT *pgft;

	if(argc==1){
		return -1;
	}

	pgft = load_pgf_font(argv[1]);
	save_pgf(pgft, argv[2]);

	return 0;
}
#endif

