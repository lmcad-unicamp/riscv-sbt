#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "fourier.h"

int main(int argc, char *argv[]) {
	unsigned MAXSIZE;
	unsigned MAXWAVES;
	unsigned i,j;
	double *RealIn;
	double *ImagIn;
	double *RealOut;
	double *ImagOut;
	double *coeff;
	double *amp;
	int invfft=0;

	if (argc<3)
	{
		printf("Usage: fft <waves> <length> -i\n");
		printf("-i performs an inverse fft\n");
		printf("make <waves> random sinusoids");
		printf("<length> is the number of samples\n");
		exit(-1);
	}
	else if (argc==4)
		invfft = !strncmp(argv[3],"-i",2);
	MAXSIZE=atoi(argv[2]);
	MAXWAVES=atoi(argv[1]);
		
 srand(1);

 for (int ind = 0; ind < 20; ++ind) {
 RealIn=(double*)malloc(sizeof(double)*MAXSIZE);
 ImagIn=(double*)malloc(sizeof(double)*MAXSIZE);
 RealOut=(double*)malloc(sizeof(double)*MAXSIZE);
 ImagOut=(double*)malloc(sizeof(double)*MAXSIZE);
 coeff=(double*)malloc(sizeof(double)*MAXWAVES);
 amp=(double*)malloc(sizeof(double)*MAXWAVES);

 /* Makes MAXWAVES waves of random amplitude and period */
	for(i=0;i<MAXWAVES;i++) 
	{
		coeff[i] = rand()%1000;
		amp[i] = rand()%1000;
	}
 for(i=0;i<MAXSIZE;i++) 
 {
   /*   RealIn[i]=rand();*/
	 RealIn[i]=0;
	 for(j=0;j<MAXWAVES;j++) 
	 {
		 /* randomly select sin or cos */
		 if (rand()%2)
		 {
		 		RealIn[i]+=coeff[j]*cos(amp[j]*i);
			}
		 else
		 {
		 	RealIn[i]+=coeff[j]*sin(amp[j]*i);
		 }
  	 ImagIn[i]=0;
	 }
 }
 /* regular*/
 fft_double (MAXSIZE,invfft,RealIn,ImagIn,RealOut,ImagOut);

 printf("RealOut:\n");
 for (i=0;i<MAXSIZE;i++)
   printf("%d %lf \n", 0, RealOut[i]);

printf("ImagOut:\n");
 for (i=0;i<MAXSIZE;i++)
   printf("%d %lf \n", 0, ImagOut[i]);

 free(RealIn);
 free(ImagIn);
 free(RealOut);
 free(ImagOut);
 free(coeff);
 free(amp);
 }
 exit(0);


}
