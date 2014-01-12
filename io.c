/*************************************************************
Copyright (C) 1990, 1991, 1993 Andy C. Hung, all rights reserved.
PUBLIC DOMAIN LICENSE: Stanford University Portable Video Research
Group. If you use this software, you agree to the following: This
program package is purely experimental, and is licensed "as is".
Permission is granted to use, modify, and distribute this program
without charge for any purpose, provided this license/ disclaimer
notice appears in the copies.  No warranty or maintenance is given,
either expressed or implied.  In no event shall the author(s) be
liable to you or a third party for any special, incidental,
consequential, or other damages, arising out of the use or inability
to use the program for any purpose (or the loss of data), even if we
have been advised of such possibilities.  Any public reference or
advertisement of this source code should refer to it as the Portable
Video Research Group (PVRG) code, and not by any author(s) (or
Stanford University) name.
*************************************************************/
/*
************************************************************
io.c

************************************************************
*/

/*LABEL io.c */

#include <stdlib.h>
#include <stdio.h>
#include "globals.h"

/*PUBLIC*/

extern void MakeIob();
extern void GlobalMC();
extern void SubOverlay();
extern void AddOverlay();
extern void SubCompensate();
extern void AddCompensate();
extern void HalfSubCompensate();
extern void HalfAddCompensate();
extern void LoadFilterMatrix();
extern void SubFCompensate();
extern void AddFCompensate();
extern void HalfSubFCompensate();
extern void HalfAddFCompensate();
extern void ClearIob();
extern void CopyIob2FS();
extern void ClearFS();
extern int ParityFS();
extern void InitFS();
extern void ReadIob();
extern void InstallIob();
extern void InstallFS();
extern void WriteIob();
extern void MoveTo();
extern int Bpos();
extern void ReadBlock();
extern void WriteBlock();
extern void PrintIob();

/*PRIVATE*/

extern IMAGE *CImage;
extern FRAME *CFrame;
extern FSTORE *CFS;

extern int MVDH;
extern int MVDV;

extern int Loud;
extern int ImageType;

IOBUF *Iob=NULL;
int BlockWidth = BLOCKWIDTH;
int BlockHeight = BLOCKHEIGHT;

/* y4m input */
#include "vidinput.h"

int y4mio;
video_input_ycbcr frame;
char tag[5];
video_input vid;

/* y4m output */
#define Y4M__CIFHEADER "YUV4MPEG2 W352 H288 C420jpeg Ip"
#define Y4M_QCIFHEADER "YUV4MPEG2 W176 H144 C420jpeg Ip"
#define Y4M_NTSCHEADER "YUV4MPEG2 W352 H240 C420jpeg Ip"
#define Y4M_4CIFHEADER "YUV4MPEG2 W704 H576 C420jpeg Ip"
FILE *y4mout;

/* 4CIF */
unsigned char *subimagedata[4][3] = {{NULL, NULL, NULL}, {NULL, NULL, NULL}, {NULL, NULL, NULL}, {NULL, NULL, NULL}};
int CIF4;
int subimage;
int UnreliableHiResBit;
unsigned char *outputframedata = NULL;

int CurrentFrame;
int FrameRate;
int FrameRateDiv;

/*START*/

/*BFUNC

MakeIob() constructs an IO structure and assorted book-keeping
instructions for all components of the frame.

EFUNC*/

void MakeIob(flag)
     int flag;
{
  BEGIN("MakeIob");
  int i;

  for(i=0;i<CFrame->NumberComponents;i++)
    {
      if (!(CFrame->Iob[i]=MakeStructure(IOBUF)))
	{
	  WHEREAMI();
	  printf("Cannot make IO structure\n");
	  exit(ERROR_MEMORY);
	}
      CFrame->Iob[i]->flag = flag;
      CFrame->Iob[i]->hpos = 0;
      CFrame->Iob[i]->vpos = 0;
      CFrame->Iob[i]->hor = CFrame->hf[i];
      CFrame->Iob[i]->ver = CFrame->vf[i];
      CFrame->Iob[i]->width = CFrame->Width[i];
      CFrame->Iob[i]->height = CFrame->Height[i];
      CFrame->Iob[i]->mem = MakeMem(CFrame->Width[i],
				    CFrame->Height[i]);
    }      
}

/*BFUNC

GlobalMC() does the motion estimation for the entire buffer.

EFUNC*/

void GlobalMC()
{
  BEGIN("GlobalMC");
  MotionEstimation(Iob->mem,CFrame->Iob[0]->mem);
}

/*BFUNC

SubOverlay() does a subtractive mask of the current matrix with a
corresponding portion of the memory in the IO buffer.

EFUNC*/

void SubOverlay(matrix)
     int *matrix;
{
  BEGIN("SubOverlay");
  int i,j;
  unsigned char *memloc;

  memloc = (Iob->vpos * Iob->width * BlockHeight) + (Iob->hpos * BlockWidth)
    + Iob->mem->data;
  for(i=0;i<BlockHeight;i++)
    {
      for(j=0;j<BlockWidth;j++)
	{
	  *(matrix) =  *(matrix) - *memloc;
	  matrix++;
	  memloc++;
	}
      memloc = memloc-BlockWidth+Iob->width;
    }
}

/*BFUNC

AddOverlay() adds the corresponding portion of the top-level IO
structure to the matrix.

EFUNC*/

void AddOverlay(matrix)
     int *matrix;
{
  BEGIN("AddOverlay");
  int i,j;
  unsigned char *memloc;

  memloc = (Iob->vpos * Iob->width * BlockHeight) + (Iob->hpos * BlockWidth)
    + Iob->mem->data;

  for(i=0;i<BlockHeight;i++)
    {
      for(j=0;j<BlockWidth;j++)
	{
	  *(matrix) =  *(matrix) + *memloc;
	  matrix++;
	  memloc++;
	}
      memloc = memloc - BlockWidth + Iob->width;
    }
}

/*BFUNC

SubCompensate() does a subtractive motion compensation of the input
matrix based on the locations given by the global variables of MVDH
and MVDV.

EFUNC*/

void SubCompensate(matrix)
     int *matrix;
{
  BEGIN("SubCompensate");
  int i,j;
  unsigned char *memloc;

  memloc = (((Iob->vpos *  BlockHeight) + MVDV)*Iob->width)
    + (Iob->hpos * BlockWidth) + MVDH
      + Iob->mem->data;
  for(i=0;i<BlockHeight;i++)
    {
      for(j=0;j<BlockWidth;j++)
	{
	  *(matrix) =  *(matrix) - *memloc;
	  matrix++;
	  memloc++;
	}
      memloc = memloc - BlockWidth + Iob->width;
    }
}

/*BFUNC

AddCompensate() does an additive motion compensation of the input
matrix from the installed IO structure based on the locations given by
the global variables of MVDH and MVDV.

EFUNC*/

void AddCompensate(matrix)
     int *matrix;
{
  BEGIN("AddCompensate");
  int i,j;
  unsigned char *memloc;

  memloc = (((Iob->vpos *  BlockHeight) + MVDV)*Iob->width)
    + (Iob->hpos * BlockWidth) + MVDH
      + Iob->mem->data;

  for(i=0;i<BlockHeight;i++)
    {
      for(j=0;j<BlockWidth;j++)
	{
	  *(matrix) =  *(matrix) + *memloc;
	  matrix++;
	  memloc++;
	}
      memloc = memloc - BlockWidth + Iob->width;
    }
}

/*BFUNC

HalfSubCompensate() does a motion compensated subtraction from the
input matrix from the installed IO structure, but divides the
compensation variable in half.

EFUNC*/

void HalfSubCompensate(matrix)
     int *matrix;
{
  BEGIN("HalfSubCompensate");
  int i,j;
  unsigned char *memloc;

  memloc = (((Iob->vpos *  BlockHeight) + (MVDV/2))*Iob->width)
    + (Iob->hpos * BlockWidth) + (MVDH/2)
      + Iob->mem->data;

  for(i=0;i<BlockHeight;i++)
    {
      for(j=0;j<BlockWidth;j++)
	{
	  *(matrix) =  *(matrix) - *memloc;
	  matrix++;
	  memloc++;
	}
      memloc = memloc - BlockWidth + Iob->width;
    }
}

/*BFUNC

HalfAddCompensate() does a motion compensated addition to the input
matrix from the installed IO structure. It divides the MVDV and MVDH
global input varaible vector in half.

EFUNC*/

void HalfAddCompensate(matrix)
     int *matrix;
{
  BEGIN("HalfAddCompensate");
  int i,j;
  unsigned char *memloc;

  memloc = (((Iob->vpos *  BlockHeight) + (MVDV/2))*Iob->width)
    + (Iob->hpos * BlockWidth) + (MVDH/2)
      + Iob->mem->data;

  for(i=0;i<BlockHeight;i++)
    {
      for(j=0;j<BlockWidth;j++)
	{
	  *(matrix) =  *(matrix) + *memloc;
	  matrix++;
	  memloc++;
	}
      memloc = memloc - BlockWidth + Iob->width;
    }
}

/*BFUNC

LoadFilterMatrix() loads a p64 filtered portion of memory into a
matrix. The correct Iob must be installed before this function is
called.

EFUNC*/

void LoadFilterMatrix(memloc,output)
     unsigned char *memloc;
     int *output;
{
  BEGIN("LoadFilterMatrix");
  int i,j;
  int temp[64];
  int *ptr,*ptr1,*ptr2,*ptr3;

  for(ptr=temp,i=0;i<BlockHeight;i++)
    {
      *(ptr++)=(*(memloc)<<2);
      for(j=1;j<BlockWidth-1;j++,ptr++)
	{
	  *(ptr) =  *(memloc++);
	  *(ptr) +=  (*(memloc++) << 1);
	  *(ptr) +=  *(memloc--);
	}
      memloc++;
      *(ptr++) = (*(memloc++)<<2);
      memloc = memloc-BlockWidth+Iob->width;
    }
  for(ptr=output,ptr1=temp,ptr2=temp,ptr3=temp+(BlockWidth<<1),i=0;
      i<BlockHeight;i++)
    {
      if ((i==0)||(i==7))
	{
	  for(j=0;j<BlockWidth;j++) {*(ptr++) = *(ptr2++);}
	}
      else
	{
	  for(j=0;j<BlockWidth;j++)
	    {
	      *(ptr) = (*(ptr2++)<<1);
	      *(ptr) +=  *(ptr1++);
	      *(ptr) +=  *(ptr3++);
	      *ptr = (*ptr>>2);
	      ptr++;
	    }
	}
    }
  for(ptr=output,i=0;i<BlockHeight;i++)
    {
      for(j=0;j<BlockWidth;j++,ptr++)
	{
	  if (*ptr & 2) {*ptr=(*ptr>>2)+1;}
	  else {*ptr=(*ptr>>2);}
	}
    }
}

/*BFUNC

SubFCompensate() does a subtractive motion compensation on a filtered
version of the input block.

EFUNC*/

void SubFCompensate(matrix)
     int *matrix;
{
  BEGIN("SubFCompensate");
  int i,j;
  unsigned char *memloc;
  int temp[64];
  int *ptr;

  memloc = (((Iob->vpos *  BlockHeight) + MVDV)*Iob->width)
    + (Iob->hpos * BlockWidth) + MVDH
      + Iob->mem->data;
  LoadFilterMatrix(memloc,temp);
  for(ptr=temp,i=0;i<BlockHeight;i++)
    {
      for(j=0;j<BlockWidth;j++)
	{
	  *(matrix) =  *(matrix) - *(ptr++);
	  matrix++;
	}
    }
}


/*BFUNC

AddFCompensate() does a additive motion compensation on a filtered
version of the input block.

EFUNC*/

void AddFCompensate(matrix)
     int *matrix;
{
  BEGIN("AddFCompensate");
  int i,j;
  unsigned char *memloc;
  int temp[64];
  int *ptr;

  memloc = (((Iob->vpos *  BlockHeight) + MVDV)*Iob->width)
    + (Iob->hpos * BlockWidth) + MVDH
      + Iob->mem->data;
  LoadFilterMatrix(memloc,temp);
  for(ptr=temp,i=0;i<BlockHeight;i++)
    {
      for(j=0;j<BlockWidth;j++)
	{
	  *(matrix) =  *(matrix) + *(ptr++);
	  matrix++;
	}
    }
}


/*BFUNC

HalfSubFCompensate() does a subtractive motion compensation on a
filtered version of the input block with the motion vector divided by
a factor of two.

EFUNC*/

void HalfSubFCompensate(matrix)
     int *matrix;
{
  BEGIN("HalfSubFCompensate");
  int i,j;
  unsigned char *memloc;
  int temp[64];
  int *ptr;

  memloc = (((Iob->vpos *  BlockHeight) + (MVDV/2))*Iob->width)
    + (Iob->hpos * BlockWidth) + (MVDH/2)
      + Iob->mem->data;
  LoadFilterMatrix(memloc,temp);
  for(ptr=temp,i=0;i<BlockHeight;i++)
    {
      for(j=0;j<BlockWidth;j++)
	{
	  *(matrix) =  *(matrix) - *(ptr++);
	  matrix++;
	}
    }
}

/*BFUNC

HalfAddFCompensate() does an additive motion compensation on a
filtered version of the input block with the motion vector divided by
two.

EFUNC*/

void HalfAddFCompensate(matrix)
     int *matrix;
{
  BEGIN("HalfAddFCompensate");
  int i,j;
  unsigned char *memloc;
  int temp[64];
  int *ptr;

  memloc = (((Iob->vpos *  BlockHeight) + (MVDV/2))*Iob->width)
    + (Iob->hpos * BlockWidth) + (MVDH/2)
      + Iob->mem->data;
  LoadFilterMatrix(memloc,temp);
  for(ptr=temp,i=0;i<BlockHeight;i++)
    {
      for(j=0;j<BlockWidth;j++)
	{
	  *(matrix) =  *(matrix) + *(ptr++);
	  matrix++;
	}
    }
}

/*BFUNC

ClearIob() clears the all the CFrame Iob memory structures by
resetting it to all zeroes.

EFUNC*/

void ClearIob()
{
  BEGIN("ClearIob");
  int i;

  for(i=0;i<CFrame->NumberComponents;i++)
    {
      ClearMem(CFrame->Iob[i]->mem);
    }
}

/*BFUNC

CopyIob2FS() copies all of the CFrame Iob's to a given frame store.

EFUNC*/

void CopyIob2FS(fs)
     FSTORE *fs;
{
  BEGIN("CopyIob2FS");
  int i;

  for(i=0;i<CFrame->NumberComponents;i++)
    {
      CopyMem(CFrame->Iob[i]->mem,fs->fs[i]->mem);
    }
}

/*BFUNC

ClearFS() clears the entire frame store passed into it.

EFUNC*/

void ClearFS(fs)
     FSTORE *fs;
{
  BEGIN("ClearFS");
  int i;

  for(i=0;i<fs->NumberComponents;i++)
    {
      ClearMem(fs->fs[i]->mem);
    }
}

/*BFUNC

ParityFS() calculates the parity of all the contents of the designated
frame store by exclusive-oring it on individual bit-planes and
returning the aggregate byte.

EFUNC*/

int ParityFS(fs)
     FSTORE *fs;
{
  BEGIN("ParityFS");
  int i,parity;

  for(parity=0,i=0;i<fs->NumberComponents;i++)
    {
      parity ^= ParityMem(fs->fs[i]->mem);
    }
  return(parity);
}

/*BFUNC

InitFS() initializes a frame store that is passed into it. It creates
the IO structures and the memory structures.

EFUNC*/

void InitFS(fs)
     FSTORE *fs;
{
  BEGIN("InitFS");
  int i;

  for(i=0;i<fs->NumberComponents;i++)
    {
      if (!(fs->fs[i]=MakeStructure(IOBUF)))
	{
	  WHEREAMI();
	  printf("Cannot create IO structure.\n");
	  exit(ERROR_MEMORY);
	}
      fs->fs[i]->flag = 0;
      fs->fs[i]->hpos = 0;
      fs->fs[i]->vpos = 0;
      fs->fs[i]->hor = CFrame->hf[i];
      fs->fs[i]->ver = CFrame->vf[i];
      fs->fs[i]->width = CFrame->Width[i];
      fs->fs[i]->height = CFrame->Height[i];
      fs->fs[i]->mem = MakeMem(CFrame->Width[i],
				CFrame->Height[i]);
    }
}

/*BFUNC
 
CreateSubimages() subsamples a frame component into four subimages.

All the Y, Cb, Cr channel samples should be divided over the CIF images equally such that

       Y           Cb       Cr
 _____________   ______   ______
|0303030303030| |030303| |030303|
|1212121212121| |121212| |121212|
|0303030303030| |030303| |030303|
|1212121212121|  ‾‾‾‾‾‾   ‾‾‾‾‾‾
|0303030303030|
|1212121212121|
 ‾‾‾‾‾‾‾‾‾‾‾‾‾
become four h261 encoded CIF images in the stream
 ______ ______ ______ ______
|000000|111111|222222|333333|
|000000|111111|222222|333333|
|000000|111111|222222|333333|
 ‾‾‾‾‾‾ ‾‾‾‾‾‾ ‾‾‾‾‾‾ ‾‾‾‾‾‾
that are, presumably, again reconstructed by a Annex D compatible decoder to
 _____________
|0303030303030|
|1212121212121|
|0303030303030|
|1212121212121|
|0303030303030|
|1212121212121|
 ‾‾‾‾‾‾‾‾‾‾‾‾‾

EFUNC*/

void CreateSubimages(int component, unsigned char *framedata, unsigned int subsize, unsigned int fullwidth)
{
	BEGIN("CreateSubimages");
	
	unsigned int i,j,k;
	if(!subimagedata[0][component])
	{
		subimagedata[0][component] = (unsigned char *)calloc(subsize, sizeof(unsigned char));
		subimagedata[1][component] = (unsigned char *)calloc(subsize, sizeof(unsigned char));
		subimagedata[2][component] = (unsigned char *)calloc(subsize, sizeof(unsigned char));
		subimagedata[3][component] = (unsigned char *)calloc(subsize, sizeof(unsigned char));
	}
	
	for(i=0, j=0, k=fullwidth; i<subsize; j+=fullwidth, k+=2*fullwidth )
	{
		for( ; j<k; j+=2, ++i)
		{
			subimagedata[0][component][i] = framedata[j];
			subimagedata[3][component][i] = framedata[j+1];
			subimagedata[1][component][i] = framedata[j+fullwidth];
			subimagedata[2][component][i] = framedata[j+fullwidth+1];
		}
	}
}

/*BFUNC
 
FreeSubimages() frees allocated subimage buffers.

EFUNC*/

void FreeSubimages()
{
	BEGIN("FreeSubimages");
	int component;
	
	if(subimagedata[0][0])
	{
		for(component = 2; component>=0; --component)
		{
			free( subimagedata[0][component] );
			free( subimagedata[1][component] );
			free( subimagedata[2][component] );
			free( subimagedata[3][component] );
		}
	}
	
	if(outputframedata)
		free(outputframedata);
}

/*BFUNC

ReadIob() loads the memory images from the filenames designated in the
CFrame structure.

EFUNC*/

void ReadIob()
{
  BEGIN("ReadIob");
  int i;

	if(!y4mio)
	{
		/*Read current frame's Y, Cb, Cr components from seperate files*/
		for(i=0;i<CFrame->NumberComponents;i++)
		{
			CFrame->Iob[i]->mem = LoadMem(	CFrame->ComponentFileName[i],
											CFrame->Width[i],
											CFrame->Height[i],
											CFrame->Iob[i]->mem);
		}
	}
	else
	{
		/*Read current frame's Y, Cb, Cr components from single y4m file*/
		if( !CIF4 )
		{
			if( !video_input_fetch_frame(&vid, frame, tag) )
				exit(ERROR_BOUNDS);
			CIF4 = (frame[0].width == 704 && frame[0].height == 576);
			if(CIF4)
				subimage = 0;
		}
		
		for(i=0;i<CFrame->NumberComponents;i++)
		{
			if (!CFrame->Iob[i]->mem)
				CFrame->Iob[i]->mem = MakeStructure(MEM);
			
			if(CIF4)
			{
				CFrame->Iob[i]->mem->width  = frame[i].width  >> 1;
				CFrame->Iob[i]->mem->height = frame[i].height >> 1;
				CFrame->Iob[i]->mem->len = CFrame->Iob[i]->mem->width * CFrame->Iob[i]->mem->height;
				if( subimage == 0)
					CreateSubimages(i, frame[i].data, CFrame->Iob[i]->mem->len, frame[i].width);
				CFrame->Iob[i]->mem->data = subimagedata[subimage][i];
			}
			else
			{
				CFrame->Iob[i]->mem->width = frame[i].width;
				CFrame->Iob[i]->mem->height = frame[i].height;
				CFrame->Iob[i]->mem->len = frame[i].width * frame[i].height;
				CFrame->Iob[i]->mem->data = frame[i].data;
			}
		}
	}
}

/*BFUNC

InstallIob() installs a particular CFrame Iob as the target Iob.

EFUNC*/

void InstallIob(index)
     int index;
{
  BEGIN("InstallIob");

  Iob = CFrame->Iob[index];
}

/*BFUNC

InstallFS() installs a index Iob in the designated frame store to be
the target Iob.

EFUNC*/

void InstallFS(index,fs)
     int index;
     FSTORE *fs;
{
  BEGIN("InstallFS");

  Iob = fs->fs[index];
}

/*BFUNC
 
MergeSubimages() merges the frame components of four subimages to form
a single larger frame component.

EFUNC*/

void MergeSubimages(int component, unsigned char **framedataptr, unsigned int subsize, unsigned int fullwidth)
{
	BEGIN("MergeSubimages");
	
	unsigned int i,j,k;
	unsigned char *framedata;
	if(!*framedataptr)
		*framedataptr = (unsigned char *)calloc(704*576, sizeof(unsigned char));

	framedata = *framedataptr;
	
	for(i=0, j=0, k=fullwidth; i<subsize; j+=fullwidth, k+=2*fullwidth )
	{
		for( ; j<k; j+=2, ++i)
		{
			framedata[j] = subimagedata[0][component][i];
			framedata[j+1] = subimagedata[3][component][i];
			framedata[j+fullwidth] = subimagedata[1][component][i];
			framedata[j+fullwidth+1] = subimagedata[2][component][i];
		}
	}
}

/*BFUNC

WriteIob() writes all the CFrame Iob's out to the filenames designated
in CFrame.

EFUNC*/

void WriteIob()
{
  BEGIN("WriteIob");
  int i;

	if(!y4mio)
	{
		for(i=0;i<CFrame->NumberComponents;i++)
		{
			SaveMem(CFrame->ComponentFileName[i],CFrame->Iob[i]->mem);
		}
	}
	else
	{
		MEM *mem;
		unsigned int len;

		if( (CurrentFrame == 0 && !CIF4) || (CurrentFrame == 4 && CIF4) )
		{
			switch(ImageType) {
				case IT_CIF:
					if(CIF4 && subimage < 4 ) /* TODO: new IT_4CIF ImageType? */
					{
						int component;
						fwrite(Y4M_4CIFHEADER,sizeof(unsigned char),sizeof(Y4M_4CIFHEADER)-1,y4mout);
						fprintf(y4mout," F%i:%i\n", FrameRate, FrameRateDiv);
						fwrite("FRAME\n",sizeof(unsigned char),sizeof("FRAME\n")-1,y4mout);

						for(component=0; component < CFrame->NumberComponents; component++)
						{
							mem = CFrame->Iob[component]->mem;
							len = mem->width * mem->height;
						
							MergeSubimages(i, &outputframedata, len, mem->width * 2 );
							fwrite(outputframedata, sizeof(unsigned char), len * 4, y4mout);
						}
					}
					else
					{
						fwrite(Y4M__CIFHEADER,sizeof(unsigned char),sizeof(Y4M__CIFHEADER)-1,y4mout);
						fprintf(y4mout," F%i:%i\n", FrameRate, FrameRateDiv);
						if(CIF4)
						{
							int component;
							UnreliableHiResBit = 1;
							for(i=0; i<4; i++)
							{
								fwrite("FRAME\n",sizeof(unsigned char),sizeof("FRAME\n")-1,y4mout);
								for(component=0; component < CFrame->NumberComponents; component++)
								{
									mem = CFrame->Iob[component]->mem;
									len = mem->width * mem->height;

									fwrite(subimagedata[i][component], sizeof(unsigned char), len, y4mout);
								}
							}
							CIF4 = 0;
						}
					}
					break;
				case IT_NTSC:
					fwrite(Y4M_NTSCHEADER,sizeof(unsigned char),sizeof(Y4M_NTSCHEADER)-1,y4mout);
					fprintf(y4mout," F%i:%i\n", FrameRate, FrameRateDiv);
					break;
				case IT_QCIF:
				default:
					fwrite(Y4M_QCIFHEADER,sizeof(unsigned char),sizeof(Y4M_QCIFHEADER)-1,y4mout);
					fprintf(y4mout," F%i:%i\n", FrameRate, FrameRateDiv);
			}
		}
		
		if(!CIF4 || (CIF4 && subimage == 0 && CurrentFrame >= 4) )
		{
			fwrite("FRAME\n",sizeof(unsigned char),sizeof("FRAME\n")-1,y4mout);
		}
/* y4m players don't seem to support changing width/height
		else
		{
			if(subimage == 0)
				fwrite("FRAME W704 H576\n",sizeof(unsigned char),sizeof("FRAME W704 H576\n")-1,y4mout);
		}
*/
		for(i=0;i<CFrame->NumberComponents;i++)
		{
			mem = CFrame->Iob[i]->mem;
			len = mem->width * mem->height;
			
			if(CIF4)
			{
				
				if(!subimagedata[0][i])
				{
					subimagedata[0][i] = (unsigned char *)calloc(len, sizeof(unsigned char));
					subimagedata[1][i] = (unsigned char *)calloc(len, sizeof(unsigned char));
					subimagedata[2][i] = (unsigned char *)calloc(len, sizeof(unsigned char));
					subimagedata[3][i] = (unsigned char *)calloc(len, sizeof(unsigned char));
				}

				memcpy(subimagedata[subimage][i], mem->data, len);
				if( subimage == 3 && CurrentFrame >= 4)
				{
					MergeSubimages(i, &outputframedata, len, mem->width << 1 );
					fwrite(outputframedata, sizeof(unsigned char), len<<2, y4mout);
				}
			}
			else
			{
				fwrite(mem->data, sizeof(unsigned char), len, y4mout);
			}
		}
	}
}

/*BFUNC

MoveTo() moves the installed Iob to a given location designated by the
Gob, MDU, and horizontal and vertical offsets.

EFUNC*/

void MoveTo(g,m,h,v)
     int g;
     int m;
     int h;
     int v;
{
  BEGIN("MoveTo");

  /* printf("moveto: IOB: %x  IOB->hor: %d %d %d %d %d\n",
	 Iob,Iob->hor,g,m,h,v);   fflush(stdout); rm, */


  switch (ImageType)
    {
    case IT_QCIF:
      Iob->hpos = (m % 11)*Iob->hor + h;
      Iob->vpos = ((g * 3) + (m / 11))*Iob->ver + v;
      break;
    case IT_CIF:
    case IT_NTSC:
      Iob->hpos = (((g & 1) * 11) + (m % 11))*Iob->hor + h;
      Iob->vpos = (((g >> 1) * 3) + (m / 11))*Iob->ver + v;
      break;
    default:
      WHEREAMI();
      printf("Unknown image type: %d.\n",ImageType);
      break;
    }
}

/*BFUNC

Bpos() returns the designated MDU number inside of the frame of the
installed Iob given by the input gob, mdu, horizontal and vertical
offset. It returns 0 on error.

EFUNC*/

int Bpos(g,m,h,v)
     int g;
     int m;
     int h;
     int v;
{
  BEGIN("Bpos");

  switch (ImageType)
    {
    case IT_QCIF:
      return(((m % 11)*Iob->hor + h) + 
	     ((((g * 3) + (m / 11))*Iob->ver + v)* Iob->width/BlockWidth));
      break;
    case IT_CIF:
    case IT_NTSC:
      return(((((g & 1) * 11) + (m % 11))*Iob->hor + h) + 
	     (((((g >> 1) * 3) + (m / 11))*Iob->ver + v)*
	      Iob->width/BlockWidth));
      break;
    default:
      WHEREAMI();
      printf("Unknown image type: %d.\n",ImageType);
      break;
    }
  return(0);
}


/*BFUNC

ReadBlock() reads a block from the currently installed Iob into a
designated matrix.

EFUNC*/

void ReadBlock(store)
     int *store;
{
  BEGIN("ReadBlock");
  int i,j;
  unsigned char *loc;

  loc = Iob->vpos*Iob->width*BlockHeight
    + Iob->hpos*BlockWidth+Iob->mem->data;
  for(i=0;i<BlockHeight;i++)
    {
      for(j=0;j<BlockWidth;j++) {*(store++) = *(loc++);}
      loc += Iob->width - BlockWidth;
    }
  if ((++Iob->hpos % Iob->hor)==0)
    {
      if ((++Iob->vpos % Iob->ver) == 0)
	{
	  if (Iob->hpos < 
	      ((Iob->width - 1)/(BlockWidth*Iob->hor))*Iob->hor + 1)
	    {
	      Iob->vpos -= Iob->ver;
	    }
	  else {Iob->hpos = 0;}
	}
      else {Iob->hpos -= Iob->hor;}
    }
}

/*BFUNC

WriteBlock() writes a input matrix as a block into the currently
designated IOB structure.

EFUNC*/

void WriteBlock(store)
     int *store;
{
  int i,j;
  unsigned char *loc;

  loc = Iob->vpos*Iob->width*BlockHeight +
    Iob->hpos*BlockWidth+Iob->mem->data;
  for(i=0;i<BlockHeight;i++)
    {
      for(j=0;j<BlockWidth;j++)	{*(loc++) =  *(store++);}
      loc += Iob->width - BlockWidth;
    }
  if ((++Iob->hpos % Iob->hor)==0)
    {
      if ((++Iob->vpos % Iob->ver) == 0)
	{
	  if (Iob->hpos < 
	      ((Iob->width - 1)/(BlockWidth*Iob->hor))*Iob->hor + 1)
	    {
	      Iob->vpos -= Iob->ver;
	    }
	  else {Iob->hpos = 0;}
	}
      else {Iob->hpos -= Iob->hor;}
    }
}

/*BFUNC

PrintIob() prints out the current Iob structure to the standard output
device.

EFUNC*/

void PrintIob()
{
  printf("IOB: %x\n",Iob);
  if (Iob)
    {
      printf("hor: %d  ver: %d  width: %d  height: %d\n",
	     Iob->hor,Iob->ver,Iob->width,Iob->height);
      printf("flag: %d  Memory Structure: %x\n",Iob->flag,Iob->mem);
    }
}

/*END*/

