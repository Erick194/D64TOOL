
#include "D64Tool.h"
#include "Lzlib.h"
#include "DeflateN64.h"
#include "Decode.h"

using namespace std;

HANDLE hConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
COORD getcoord;

COORD GetConsoleCursorPosition(HANDLE hConsoleOutput)
{
    CONSOLE_SCREEN_BUFFER_INFO cbsi;
    if (GetConsoleScreenBufferInfo(hConsoleOutput, &cbsi))
    {
        return cbsi.dwCursorPosition;
    }
    else
    {
        // The function failed. Call GetLastError() for details.
        COORD invalid = { 0, 0 };
        return invalid;
    }
}

void setcolor (int color)
{
    SetConsoleTextAttribute(hConsoleOutput, color);
}

#define CLAMP(value, min, max) (((value) >(max)) ? (max) : (((value) <(min)) ? (min) : (value)))

void PrintfPorcentaje(int count,int Prc, bool draw, int ypos, const char *s, ...)
{
    //setcolor(0x0A);
    //HANDLE hConsoleOutput; COORD coord;     hConsoleOutput=GetStdHandle(STD_OUTPUT_HANDLE);
    COORD coord;
     
    char msg[1024];
    va_list v;

    va_start(v, s);
    vsprintf(msg, s, v);
    va_end(v);

    coord.X=0;coord.Y=ypos;
    SetConsoleCursorPosition(hConsoleOutput,coord);
    
    float prc = (float) CLAMP(((float)((count))) /(Prc), 0.0, 1.0);
    if(draw)
    {
        setcolor(0x07);
        printf("%s", msg);
        setcolor(0x0A);
        printf("(%%%.2f)        \n", prc*100);
    }
}

void Error(const char *s,...)
{
    setcolor(0x0C);
    va_list args;
    va_start(args,s);
    vfprintf(stderr,s,args);
    fprintf(stderr,"\n");
    va_end(args);
    setcolor(0x07);
    system("PAUSE");
    exit(0);
}

static int T_START = 0;
static int T_END = 0;

char Textures[2048][9] ={};
char Flats[2048][9] ={};

int GetFlatNum(char *name)
{
    int i = 0;
    
    for(i = 0; i < 2048; i++)
    {
            if(!strcmp(name, Flats[i]))
                break;
    }
    
    //printf("Flat (%s -> ,%d)\n",Flats[i],i);getch();
    return i;
}

int GetTextureNum(char *name)
{
    int i = 0;
    
    for(i = 0; i < 2048; i++)
    {
          if(!strcmp(name, Textures[i]))
                break;
    }
    
    //printf("Texture (%s -> ,%d)\n",Textures[i],i);getch();
    return i;
}

//
// P_InitTextureHashTable
//

static unsigned short        texturehashlist[2][2048];

/*static void P_InitTextureHashTable(void) {
    int i;
    //int t = W_GetNumForName("T_START") + 1;

    //texturehashlist[0]  = //Z_Alloca(numtextures * sizeof(word));
    //texturehashlist[1]  = //Z_Alloca(numtextures * sizeof(word));

    for(i = 0; i < numtextures; i++) {
        texturehashlist[0][i] = W_HashLumpName(lumpinfo[t + i].name) % 65536;
        texturehashlist[1][i] = i;
    }
}*/

//
// W_HashLumpName
//

//
// P_GetTextureHashKey
//

static unsigned short P_GetTextureHashKey(int hash) {
    int i;

    for(i = 0; i < 2048/*numtextures*/; i++) {
        if(texturehashlist[0][i] == hash) {
            return texturehashlist[1][i];
        }
    }

    return 0;
}

unsigned int W_HashLumpName(const char* str) {
    unsigned int hash = 1315423911;
    unsigned int i    = 0;

    for(i = 0; i < 8 && *str != '\0'; str++, i++) {
        hash ^= ((hash << 5) + toupper((int)*str) + (hash >> 2));
    }

    return hash;
}

enum {LABEL,THINGS,LINEDEFS,SIDEDEFS,VERTEXES,SEGS,SSECTORS,NODES,SECTORS,REJECT,BLOCKMAP,LEAFS,LIGHTS,MACROS,SCRIPTS};

FILE *mapin;
FILE *mapout;

static int mapdatacnt = 0;
static int mapdatasize = 0;

typedef struct
{
   int  filepos; // also texture_t * for comp lumps
   int  size;
   char name[8+1];
} lumpinfo_t;

typedef struct
{
   char identification[4]; // should be IWAD
   int  numlumps;
   int  infotableofs;
} wadinfo_t;

wadinfo_t wadfile;

lumpinfo_t *lumpinfo; // points directly to rom image
int         numlumps;

int offcnt = 12;

void CopyLump(int lump, bool Decode = false, bool Encode = false)
{
    unsigned char *input, *output;
    unsigned char data;
    int i;
    int size = 0;
    int pow = 0;
    int ilen = 0;
    int olen = 0;
    bool DecodeJAG = true;
    bool EncodeJAG = true;

    PrintfPorcentaje(lump, numlumps - 1, true, getcoord.Y, "Convirtiendo Mapa Formato Doom 64 EX a Doom 64 N64...\t", lump);
     
    fseek(mapin,lumpinfo[lump].filepos,SEEK_SET);
     
    //setnew filepos
    lumpinfo[lump].filepos = offcnt;
     
    if(lumpinfo[lump].size == 0) {
        Decode = false;
        Encode = false;
    }
     
    if(Decode) {
        if(lumpinfo[lump].name[0] & 0x80) // compressed
            Decode = true;
        else
            Decode = false;
    }

    if(Encode)
    {
        if (strncmp (lumpinfo[lump].name, "MAP", 3) == 0) { EncodeJAG = false; printf("encode %s lump -> %d\n", lumpinfo[lump].name, lump); /*getch();*/}
        if (strncmp (lumpinfo[lump].name, "DEMO", 4) == 0) { EncodeJAG = false; printf("encode %s lump -> %d\n", lumpinfo[lump].name, lump); /*getch();*/}
        if (lump >= T_START && lump <= T_END){ EncodeJAG = false; printf("encode texture %s %d                \n", lumpinfo[lump].name, lump); /*getch();*/}

        ilen = lumpinfo[lump].size;
        //printf("ilen %d\n",ilen);getch();

        input = (unsigned char*)malloc(ilen);
        fread(input, sizeof(byte), ilen, mapin);
        size += ilen;

        if(EncodeJAG) {
            output = encode(input, ilen, &olen);
        }
        else {
            output = EncodeD64(input, ilen, &olen);
        }

        // La compresion es mas grande que el archivo original
        if (olen > ilen) {
            olen = ilen;
            fwrite(input, sizeof(byte), olen, mapout);
        }
        else {
            fwrite(output, sizeof(byte), olen, mapout);
            lumpinfo[lump].name[0] |= (0x80);
        }

        // Alinea los datos
        pow = olen % 4;
        //printf("size %d pow4 %d\n",olen, pow);
        if (pow != 0) {
            //printf("Add\n");
            for (i = 0; i < 4 - pow; i++) {
                fputc(0x00, mapout);
                olen++;
            }
        }

        free(input);
        free(output);
    }
    else if(Decode)
    {
        //printf("%s\n",lumpinfo[lump].name);//getch();

        lumpinfo[lump].name[0] &= ~(0x80);

        if (strncmp (lumpinfo[lump].name, "MAP", 3) == 0) { DecodeJAG = false; printf("decode  %s lump -> %d\n", lumpinfo[lump].name, lump);}
        if (strncmp (lumpinfo[lump].name, "DEMO", 4) == 0) { DecodeJAG = false; printf("decode  %s lump -> %d\n", lumpinfo[lump].name, lump);}
        if (lump >= T_START && lump <= T_END){ DecodeJAG = false; printf("decode texture %s %d\n", lumpinfo[lump].name, lump);}
         
        ilen = lumpinfo[lump].size;
         
        input = (unsigned char*)malloc(ilen);
        fread(input, sizeof(unsigned char), ilen, mapin);
         
        if(DecodeJAG) {
            olen = decodedsize(input);
        }
        else {
            olen = ilen;
        }
        output = (unsigned char*)malloc(olen);

        if(DecodeJAG) {
            decode(input, output);
        }
        else{
            //Deflate_Decompress(input, output);
            DecodeD64(input, output);
        }

        fwrite(output, sizeof(byte), olen, mapout);
        size += olen;

        free(input);
        free(output);
    }
    else
    {
        for(i = 0; i < lumpinfo[lump].size; i++) {
            fread(&data, sizeof(unsigned char), 1 ,mapin);
            fputc(data, mapout);
            size++;
            olen++;
        }

        pow = lumpinfo[lump].size % 4;
        if(pow != 0) {
            for (i=0 ; i<4-pow; i++) {
                fputc(0x00, mapout);
                olen++;
            }
        }
    }
     
    lumpinfo[lump].size = size;
    offcnt += olen;
}

void ConvertSidefs(int lump)
{
    PrintfPorcentaje(lump, numlumps-1, true, getcoord.Y, "Convirtiendo Mapa Formato Doom 64 EX a Doom 64 N64...\t", lump);
          
    int size = 0;
    char nulltex[9] = {"-"};
     
    short xoffset;
    short yoffset;
    char upper[9] = { 0 };
    char lower[9] = { 0 };
    char middle[9] = { 0 };
    short faces;
    
    unsigned short tex_up = 0;
    unsigned short tex_low = 0;
    unsigned short tex_mid = 0;

    unsigned short tex_up2;
    unsigned short tex_low2;
    unsigned short tex_mid2;

    short texnull = -1;
     
    int numsidefs = 0;
     
    fseek(mapin,lumpinfo[lump].filepos,SEEK_SET);
     
    //setnew filepos
    lumpinfo[lump].filepos = offcnt;
     
    numsidefs = lumpinfo[lump].size/12;
         
    for(int i = 0; i < numsidefs; i++)
    {
        fread (&xoffset,sizeof(short),1,mapin);
        fread (&yoffset,sizeof(short),1,mapin);
        fread (&tex_up,sizeof(short),1,mapin);
        fread (&tex_low,sizeof(short),1,mapin);
        fread (&tex_mid,sizeof(short),1,mapin);
        fread (&faces,sizeof(short),1,mapin);
            
        //write lump

        fwrite (&xoffset,sizeof(short),1,mapout);
        fwrite (&yoffset,sizeof(short),1,mapout);

        tex_up2 = P_GetTextureHashKey(tex_up);
        fwrite (&tex_up2,sizeof(short), 1 ,mapout);

        tex_low2 = P_GetTextureHashKey(tex_low);
        fwrite (&tex_low2,sizeof(short), 1 ,mapout);

        tex_mid2 = P_GetTextureHashKey(tex_mid);
        fwrite (&tex_mid2,sizeof(short), 1 ,mapout);
               
        fwrite (&faces,sizeof(short),1,mapout);
        size += 12;
    }
     
    //printf("size %d\n",size);
    //getch();
    lumpinfo[lump].size = size;

    int olen = size;
    int pow = lumpinfo[lump].size % 4;
    if(pow != 0)
    {
        for (int i=0 ; i<4-pow; i++)
        {
            fputc(0x00, mapout);
            olen++;
        }
    }
     
    offcnt+=olen;
}

void ConvertSectors(int lump)
{
    PrintfPorcentaje(lump, numlumps - 1, true, getcoord.Y, "Convirtiendo Mapa Formato Doom 64 EX a Doom 64 N64...\t", lump);
          
    int size = 0;
    char nulltex[9] = {"-"};
    short texnull = -1;
    short floorz = 0;
    short ceilz = 0;
    char floortex[9] = { 0 };
    char ceiltex[9] = { 0 };
    short light = 0;
    short light2 = 0;
    short special = 0;
    short tag = 0;
    short flags = 0;

    /*unsigned short color1;
    unsigned short color2;
    unsigned short color3;
    unsigned short color4;
    unsigned short color5;*/
    unsigned short color[5];
	
    unsigned short tex_floor = 0;
    unsigned short tex_ceil = 0;

    unsigned short tex_floor2;
    unsigned short tex_ceil2;
	
    int numsectors = 0;
	
    fseek(mapin,lumpinfo[lump].filepos,SEEK_SET);

    //setnew filepos
    lumpinfo[lump].filepos = offcnt;
    numsectors = lumpinfo[lump].size/24;
         
    for(int i = 0; i <numsectors; i++)
    {
        fread (&floorz,sizeof(short),1,mapin);
        fread (&ceilz,sizeof(short),1,mapin);

        fread (&tex_floor,sizeof(short),1,mapin);
        fread (&tex_ceil,sizeof(short),1,mapin);
            
        /*fread (&color1,sizeof(short),1,mapin);
        fread (&color2,sizeof(short),1,mapin);
        fread (&color3,sizeof(short),1,mapin);
        fread (&color4,sizeof(short),1,mapin);
        fread (&color5,sizeof(short),1,mapin);*/
        fread (&color,sizeof(color),1,mapin);

        fread (&special,sizeof(short),1,mapin);
        fread (&tag,sizeof(short),1,mapin);
        fread (&flags,sizeof(short),1,mapin);
            
        //write lump
            
        fwrite (&floorz,sizeof(short),1,mapout);
        fwrite (&ceilz,sizeof(short),1,mapout);
 
        tex_floor2 = P_GetTextureHashKey(tex_floor);
        fwrite (&tex_floor2,sizeof(short), 1 ,mapout);

        tex_ceil2 = P_GetTextureHashKey(tex_ceil);
        fwrite (&tex_ceil2,sizeof(short), 1 ,mapout);

        /*fwrite (&color1,sizeof(short),1,mapout);
        fwrite (&color2,sizeof(short),1,mapout);
        fwrite (&color3,sizeof(short),1,mapout);
        fwrite (&color4,sizeof(short),1,mapout);
        fwrite (&color5,sizeof(short),1,mapout);*/
        fwrite (&color,sizeof(color),1,mapout);

        fwrite (&special,sizeof(short),1,mapout);
        fwrite (&tag,sizeof(short),1,mapout);
        fwrite (&flags,sizeof(short),1,mapout);

        size += 24;
    }
     
    lumpinfo[lump].size = size;
     
    int olen = size;
    int pow = lumpinfo[lump].size % 4;
    if(pow != 0)
    {
        for (int i=0 ; i<4-pow; i++)
        {
            fputc(0x00, mapout);
            olen++;
        }
    }

    offcnt+=olen;
}

void Read_MapWad(char *name)
{
    int i;
    bool haveScripts = false;
    if ((mapin = fopen(name, "r+b")) == 0)
    {
        Error("No encuentra el archivo %s\n",name);
    }
     
    //Change To IWAD
    /*fread (&data, sizeof(unsigned char), 1, mapin);//'P'
    fseek(mapin, 0, SEEK_SET);
    data = 'I';
    fwrite (&data, sizeof(unsigned char), 1, mapin);//'I'
    fseek(mapin, 0, SEEK_SET);*/
    
    fread (&wadfile,sizeof(wadfile), 1 ,mapin);
    wadfile.identification[0] = 'I';
     
    mapdatacnt = 0;
    mapdatasize = numlumps;
     
    //printf("%s\n",wadfile.identification);
    //printf("%d\n",wadfile.numlumps);
    //printf("%d\n\n",wadfile.infotableofs);
     
    numlumps = wadfile.numlumps;
    lumpinfo = (lumpinfo_t*)malloc(numlumps*sizeof(lumpinfo_t));
     
    fseek(mapin,wadfile.infotableofs,SEEK_SET);
     
    for(i = 0; i < numlumps; i++)
    {
        //fread (&lumpinfo[i],sizeof(lumpinfo_t), 1 ,file);
        fread (&lumpinfo[i].filepos,sizeof(unsigned int), 1 ,mapin);
        fread (&lumpinfo[i].size,sizeof(unsigned int), 1 ,mapin);
        fread (&lumpinfo[i].name,sizeof(unsigned int)*2, 1 ,mapin);

        if(!strcmp(lumpinfo[i].name,"T_START")){T_START = i+1;}
        if(!strcmp(lumpinfo[i].name,"T_END")){T_END = i;}
        if (!strcmp(lumpinfo[i].name, "SCRIPTS")) { haveScripts = true; }
    }
    for(i = 0; i < numlumps; i++)
    {
        //printf("%d\n",lumpinfo[i].filepos);
        //printf("%d\n",lumpinfo[i].size);
        //printf("%s\n",lumpinfo[i].name);
        //getch();
    }
    //fclose(file);

    if (haveScripts) {
        numlumps -= 1;
    }
}

void info()
{
    //printf("     #######################(ERICK194)########################\n");
    setcolor(0x07);printf("\n     #######################");
    setcolor(0x0A);printf("(ERICK194)");
    setcolor(0x07);printf("#########################\n");
    printf("     #                       D64 TOOL                         #\n");
    printf("     #             CREADO POR ERICK VASQUEZ GARCIA            #\n");
    printf("     #                   VERSION 1.1 (2023)                   #\n");
    printf("     #                                                        #\n");
    printf("     # MODO DE USO:                                           #\n");
    printf("     # (-D64_EX_TO_N64) Convierte Mapa Doom64 EX a Doom64 N64 #\n");
    printf("     # (-ENCODE) Encodifica todos los lumps                   #\n");
    printf("     # (-DECODE) Decodifica todos los lumps                   #\n");
    printf("     #                                                        #\n");
    printf("     # D64TOOL MODO MAPIN MAPOUT                              #\n");
    printf("     ##########################################################\n");
    printf("\n");

    setcolor(0x07);
    system("PAUSE");
    exit(0);
}

int main(int argc, char *argv[])
{
    int i;
    char Leafs[9] = {"LEAFS"};
    char Endofwad[9] = {"MACROS"};
    int  LumpPos = 0x0C;
    int  LumpSize = 0x00;

    FILE *file;
    if (argc != 4)
    {
        info();
    }
    else
    {
        //info();
        // skip program name itself
        argv++, argc--;

        //Carga TEXTURES.txt y FLATS.txt
        for(i = 0; i < 2048; i++)
        {
              strncpy(Textures[i], "-", 8);
              strncpy(Flats[i], "-", 8);
        }
        
        if ((file = fopen("TEXTURES.txt", "r")) == 0)
    	{
            Error("No encuentra el archivo TEXTURES.txt\n");
    	}
    	else
    	{
            for(i = 0; i < 2048; i++)
            {
                  if (feof(file))
                     break;
                  fscanf (file,"%s\n",&Textures[i]);

			      texturehashlist[0][i] = W_HashLumpName(Textures[i]) % 65536;
			      texturehashlist[1][i] = i;
            }
            fclose(file);
        }
        
		if(!strcmp(argv[0],"-ENCODE"))
        {
             //printf("Comprimiendo %s...\n", argv[2]);
			 //InitCountTable();
             Read_MapWad(argv[1]);
             
             mapout = fopen(argv[2], "wb");
             fwrite (&wadfile,sizeof(wadfile), 1 ,mapout);
             
             getcoord = GetConsoleCursorPosition(hConsoleOutput);
             for(i = 0; i < numlumps; i++)
             {
                   PrintfPorcentaje(i, numlumps-1, true, getcoord.Y, "Comprimiendo %s...\t",argv[2], i);
                   CopyLump(i, false, true);
             }
        }
        else if(!strcmp(argv[0],"-DECODE"))
        {
             //printf("Decomprimiendo %s...\n", argv[1]);
             Read_MapWad(argv[1]);
             
             mapout = fopen(argv[2], "wb");
             fwrite (&wadfile,sizeof(wadfile), 1 ,mapout);
             
             getcoord = GetConsoleCursorPosition(hConsoleOutput);
             for(i = 0; i < numlumps; i++)
             {
                   PrintfPorcentaje(i, numlumps-1, true, getcoord.Y, "Decomprimiendo %s...\t",argv[2], i);
                   CopyLump(i, true);
             }
        }
		else if(!strcmp(argv[0],"-D64_EX_TO_N64"))
        {
             Read_MapWad(argv[1]);
             
             mapout = fopen(argv[2], "wb");
             fwrite (&wadfile,sizeof(wadfile), 1 ,mapout);

             getcoord = GetConsoleCursorPosition(hConsoleOutput);
             
             CopyLump(LABEL);
             CopyLump(THINGS);
             CopyLump(LINEDEFS);
             ConvertSidefs(SIDEDEFS);
             CopyLump(VERTEXES);
             CopyLump(SEGS);
             CopyLump(SSECTORS);
             CopyLump(NODES);
             ConvertSectors(SECTORS);
             CopyLump(REJECT);
             CopyLump(BLOCKMAP);
			 CopyLump(LEAFS);
             CopyLump(LIGHTS);
             CopyLump(MACROS);
             //CopyLump(SCRIPTS);
        }
        else { Error("Desconocido modo %s\n", argv[0]); }
    }

    if(mapout)
    {
        for(i = 0; i < numlumps; i++)
        {
             fwrite (&lumpinfo[i].filepos,sizeof(unsigned int), 1 ,mapout);
             fwrite (&lumpinfo[i].size,sizeof(unsigned int), 1 ,mapout);
             fwrite (&lumpinfo[i].name,sizeof(unsigned int)*2, 1 ,mapout);
        }

        fseek(mapout,0,SEEK_SET);
        wadfile.numlumps = numlumps;
        wadfile.infotableofs = offcnt;
        fwrite (&wadfile,sizeof(wadfile), 1 ,mapout);
        
        fclose(mapout);
    }

    free(lumpinfo);
    
    setcolor(0x07);
    system("PAUSE");
    return EXIT_SUCCESS;
}
