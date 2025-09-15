/*********************************

       >> PCX2BIN.CPP <<

**********************************/
// #include <game13h.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <fcntl.h>
#include <io.h>
#include <dos.h>
#include <malloc.h>

int in_file, out_file;
unsigned char pal_info[256][3];
char* data;


void write_pic_file(FILE* fp, FILE* outfp)
{
    unsigned char count, indata;
    fseek(fp, 0x80, SEEK_SET);

    do {
        if (fread(&indata, 1, 1, fp) == 0) break;
        if ((indata & 0xC0) == 0xC0) {
            count = indata & 0x3F;
            fread(&indata, 1, 1, fp);
            for (int i = 0; i < count; i++) {
                fwrite(&indata, 1, 1, outfp);
            }
        } else {
            fwrite(&indata, 1, 1, outfp);
        }
    } while (1);
}


void write_pal_file(FILE* fp, FILE* outfp)
{
    fseek(fp, -768, SEEK_END);
    fread(pal_info, 3, 256, fp);
    for (int i = 0; i < 256; i++) {
        pal_info[i][0] >>= 2;
        pal_info[i][1] >>= 2;
        pal_info[i][2] >>= 2;
    }
    fwrite(pal_info, 3, 256, outfp);
}


int main(int argc, char* argv[])
{
    char in_filename[13], out_filename[13], pal_filename[13];

    if (argc < 2) {
        cputs("Usage : PCX2BIN <PCX file name>\n\r");
        exit(0);
    }

    strcpy(in_filename, argv[1]);
    strcpy(out_filename, argv[1]);
    strcpy(pal_filename, argv[1]);
    strupr(in_filename);
    strupr(out_filename);
    strupr(pal_filename);
    strcat(in_filename, ".PCX");
    strcat(out_filename, ".BIN");
    strcat(pal_filename, ".PAL");

    printf("%s ==> %s\n", in_filename, out_filename);

    FILE *in_file = fopen(in_filename, "rb");
    if (in_file == NULL) {
        printf("%s file Not Found !!\n", in_filename);
        return -1;
    }
    FILE *out_file = fopen(out_filename, "wb");
    write_pic_file(in_file, out_file);
    fclose(out_file);

    printf("%s ==> %s\n", in_filename, pal_filename);

    FILE *pal_file = fopen(pal_filename, "wb");
    write_pal_file(in_file, pal_file);
    fclose(pal_file);

    fclose(in_file);

    return 0;
}
