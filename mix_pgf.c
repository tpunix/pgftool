
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pgf.h"
#include "bmp.h"

void mix_pgf(PGF_FONT *dpgf, PGF_FONT *spgf)
{
	PGF_GLYPH *glyph;
	int i;

	for(i=0; i<65536; i++){
		glyph = spgf->char_glyph[i];
		if(glyph==NULL)
			continue;
		dpgf->char_glyph[i] = glyph;
	}
}

int main(int argc, char *argv[])
{
	PGF_FONT *spgf, *dpgf;

	if(argc<2){
		printf("mix_pgf <target pgf> <source pgf>\n");
		return -1;
	}

	dpgf = load_pgf_font(argv[1]);
	if(dpgf==NULL){
		return -1;
	}

	spgf = load_pgf_font(argv[2]);
	if(dpgf==NULL){
		return -1;
	}

	mix_pgf(dpgf, spgf);

	save_pgf(dpgf, argv[1]);

	return 0;
}

