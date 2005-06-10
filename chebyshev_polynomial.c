/* $Id$ */
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "global.h"
#include "linsolve.h"
#include "linalg_eo.h"
#include "start.h"
#include "tm_operators.h"
#include "chebyshev_polynomial.h"
#include "Nondegenerate_Matrix.h"


#define PI 3.141592653589793

double func(double u){
  return pow(u,0.25);
}

void chebyshev_polynomial(double aa, double bb, double c[], int n){
  int k,j;
  double fac,bpa,bma,*f;
  double inv_n;

  inv_n=1./(double)n;
  f=calloc(n,sizeof(double));/*vector(0,n-1);*/
  bma=0.5*(bb-aa);
  bpa=0.5*(bb+aa);
  for (k=0;k<n;k++) {
    double y=cos(PI*(k+0.5)*inv_n);
    f[k]=func(y*bma+bpa);
  }
  fac=2.0*inv_n;
  for (j=0;j<n;j++) {
    double sum=0.0;
    for (k=0;k<n;k++)
      sum += f[k]*cos(PI*j*(k+0.5)*inv_n);
    c[j]=fac*sum;
  }
  free(f);
}
#undef PI


/****************************************************************************  
 *
 * computation of (Q^dagger Q)^(1/4) on a vector
 *   by using the chebyshev approximation for the function ()^1/4
 * subtraction of low-lying eigenvalues is not yet implemented for this
 *
 **************************************************************************/

void QdaggerQ_onequarter(spinor *R_s, spinor *R_c, double *c, int n, spinor *S_s, spinor *S_c, double minev)
{
  int j;
  double fact1, fact2, temp1, temp2, temp3, temp4, maxev;
  spinor *svs_, *svs, *ds_, *ds, *dds_, *dds, *auxs_, *auxs, *aux3s_, *aux3s;
  spinor *svc_, *svc, *dc_, *dc, *ddc_, *ddc, *auxc_, *auxc, *aux3c_, *aux3c;

  


#if ( defined SSE || defined SSE2 )
   svs_  = calloc(VOLUMEPLUSRAND+1, sizeof(spinor));
   svs   = (spinor *)(((unsigned int)(svs_)+ALIGN_BASE)&~ALIGN_BASE);
   ds_   = calloc(VOLUMEPLUSRAND+1, sizeof(spinor));
   ds    = (spinor *)(((unsigned int)(ds_)+ALIGN_BASE)&~ALIGN_BASE);
   dds_  = calloc(VOLUMEPLUSRAND+1, sizeof(spinor));
   dds   = (spinor *)(((unsigned int)(dds_)+ALIGN_BASE)&~ALIGN_BASE);
   auxs_ = calloc(VOLUMEPLUSRAND+1, sizeof(spinor));
   auxs  = (spinor *)(((unsigned int)(auxs_)+ALIGN_BASE)&~ALIGN_BASE);
   aux3s_= calloc(VOLUMEPLUSRAND+1, sizeof(spinor));
   aux3s = (spinor *)(((unsigned int)(aux3s_)+ALIGN_BASE)&~ALIGN_BASE);
   svc_  = calloc(VOLUMEPLUSRAND+1, sizeof(spinor));
   svc   = (spinor *)(((unsigned int)(svc_)+ALIGN_BASE)&~ALIGN_BASE);
   dc_   = calloc(VOLUMEPLUSRAND+1, sizeof(spinor));
   dc    = (spinor *)(((unsigned int)(dc_)+ALIGN_BASE)&~ALIGN_BASE);
   ddc_  = calloc(VOLUMEPLUSRAND+1, sizeof(spinor));
   ddc   = (spinor *)(((unsigned int)(ddc_)+ALIGN_BASE)&~ALIGN_BASE);
   auxc_ = calloc(VOLUMEPLUSRAND+1, sizeof(spinor));
   auxc  = (spinor *)(((unsigned int)(auxc_)+ALIGN_BASE)&~ALIGN_BASE);
   aux3c_= calloc(VOLUMEPLUSRAND+1, sizeof(spinor));
   aux3c = (spinor *)(((unsigned int)(aux3c_)+ALIGN_BASE)&~ALIGN_BASE);
#else
   svs_=calloc(VOLUMEPLUSRAND, sizeof(spinor));
   svs = sv_;
   ds_=calloc(VOLUMEPLUSRAND, sizeof(spinor));
   ds = d_;
   dds_=calloc(VOLUMEPLUSRAND, sizeof(spinor));
   dds = dd_;
   auxs_=calloc(VOLUMEPLUSRAND, sizeof(spinor));
   auxs = aux_;
   aux3s_=calloc(VOLUMEPLUSRAND, sizeof(spinor));
   aux3s = aux3_;
   svc_=calloc(VOLUMEPLUSRAND, sizeof(spinor));
   svc = sv_;
   dc_=calloc(VOLUMEPLUSRAND, sizeof(spinor));
   dc = d_;
   ddc_=calloc(VOLUMEPLUSRAND, sizeof(spinor));
   ddc = dd_;
   auxc_=calloc(VOLUMEPLUSRAND, sizeof(spinor));
   auxc = aux_;
   aux3c_=calloc(VOLUMEPLUSRAND, sizeof(spinor));
   aux3c = aux3_;
#endif
 
   maxev=1.0;
   
   fact1=4/(maxev-minev);
   fact2=-2*(maxev+minev)/(maxev-minev);
   
   zero_spinor_field(&ds[0],VOLUME/2);
   zero_spinor_field(&dds[0],VOLUME/2); 
   zero_spinor_field(&dc[0],VOLUME/2);
   zero_spinor_field(&ddc[0],VOLUME/2); 
   
/*   sub_low_ev(&aux3[0], &S[0]);  */ 
    assign(&aux3s[0], &S_s[0],VOLUME/2);  
    assign(&aux3c[0], &S_c[0],VOLUME/2);  
   
     /* Use the Clenshaw's recursion for the 
	Chebysheff polynomial 
     */
     for (j=n-1; j>=1; j--) {
       assign(&svs[0],&ds[0],VOLUME/2); 
       assign(&svc[0],&dc[0],VOLUME/2); 
       
/*       if ( (j%10) == 0 ) {
  	 sub_low_ev(&aux[0], &d[0]);
         }
         else {    */
	 assign(&auxs[0], &ds[0], VOLUME/2);
	 assign(&auxc[0], &dc[0], VOLUME/2);
/*       }  */
       
       QdaggerNon_degenerate(&aux3s[0], &aux3c[0], &auxs[0], &auxc[0]);
       QNon_degenerate(&R_s[0], &R_c[0], &aux3s[0], &aux3c[0]);
       temp1=-1.0;
       temp2=c[j];
       assign_mul_add_mul_add_mul_add_mul_r(&ds[0] , &R_s[0], &dds[0], &aux3s[0], fact2, fact1, temp1, temp2,VOLUME/2);
       assign_mul_add_mul_add_mul_add_mul_r(&dc[0] , &R_c[0], &ddc[0], &aux3c[0], fact2, fact1, temp1, temp2,VOLUME/2);
       assign(&dds[0], &svs[0],VOLUME/2);
       assign(&ddc[0], &svc[0],VOLUME/2);
     } 
     
/*     sub_low_ev(&R[0],&d[0]);  */ 
     assign(&R_s[0], &ds[0],VOLUME/2); 
     assign(&R_c[0], &dc[0],VOLUME/2); 
     
     QdaggerNon_degenerate(&aux3s[0], &aux3c[0], &R_s[0], &R_c[0]);
     QNon_degenerate(&auxs[0], &auxc[0], &aux3s[0], &aux3c[0]);

     temp1=-1.0;
     temp2=c[0]/2;
     temp3=fact1/2;
     temp4=fact2/2;
     assign_mul_add_mul_add_mul_add_mul_r(&auxs[0], &ds[0], &dds[0], &aux3s[0], temp3, temp4, temp1, temp2,VOLUME/2);
     assign_mul_add_mul_add_mul_add_mul_r(&auxc[0], &dc[0], &ddc[0], &aux3c[0], temp3, temp4, temp1, temp2,VOLUME/2);
     
/*     addproj_q_invsqrt(&R[0], &S[0]); */
    
#ifndef _SOLVER_OUTPUT
     if(g_proc_id == g_stdio_proc){
       printf("Order of Chebysheff approximation = %d\n",j); 
       fflush( stdout);};
#endif
     
    
   
   free(svs_);
   free(ds_);
   free(dds_);
   free(auxs_);
   free(aux3s_);
   free(svc_);
   free(dc_);
   free(ddc_);
   free(auxc_);
   free(aux3c_);
}
  


/**************************************************************************
 *
 * The externally accessible function is
 *
 *   void degree_of_polynomial(void)
 *     Computation of (QdaggerQ)^1/4
 *     by using the chebyshev approximation for the function ()^1/4  
 *
 * Author: Mauro Papinutto <papinutt@mail.desy.de> Apr 2003
 *         adapted by Ines Wetzorke <Ines.Wetzorke@desy.de> May 2003 
 *         adapted by Karl Jansen <Karl.Jansen@desy.de> June 2005 
 *
*******************************************************************************/

double stopeps=5.0e-16;

int dop_n_cheby=0;
double * dop_cheby_coef;

void degree_of_polynomial(){
  int i;
  double temp;
  double ev_minev,ev_maxev;
  static int ini=0;

  spinor *ss, *ss_, *auxs, *auxs_, *aux3s, *aux3s_;
  spinor *sc, *sc_, *auxc, *auxc_, *aux3c, *aux3c_;



  if(ini==0){
    dop_cheby_coef = calloc(N_CHEBYMAX,sizeof(double));
    ini=1;
  }




#if ( defined SSE || defined SSE2 || defined SSE3)
   ss_   = calloc(VOLUMEPLUSRAND/2+1, sizeof(spinor));
   auxs_ = calloc(VOLUMEPLUSRAND/2+1, sizeof(spinor));
   aux3s_= calloc(VOLUMEPLUSRAND/2+1, sizeof(spinor));
   ss    = (spinor *)(((unsigned int)(ss_)+ALIGN_BASE)&~ALIGN_BASE);
   auxs  = (spinor *)(((unsigned int)(auxs_)+ALIGN_BASE)&~ALIGN_BASE);
   aux3s = (spinor *)(((unsigned int)(aux3s_)+ALIGN_BASE)&~ALIGN_BASE);
   sc_   = calloc(VOLUMEPLUSRAND/2+1, sizeof(spinor));
   auxc_ = calloc(VOLUMEPLUSRAND/2+1, sizeof(spinor));
   aux3c_= calloc(VOLUMEPLUSRAND/2+1, sizeof(spinor));
   sc    = (spinor *)(((unsigned int)(sc_)+ALIGN_BASE)&~ALIGN_BASE);
   auxc  = (spinor *)(((unsigned int)(auxc_)+ALIGN_BASE)&~ALIGN_BASE);
   aux3c = (spinor *)(((unsigned int)(aux3c_)+ALIGN_BASE)&~ALIGN_BASE);
#else
   ss   =calloc(VOLUMEPLUSRAND/2, sizeof(spinor));
   auxs =calloc(VOLUMEPLUSRAND/2, sizeof(spinor));
   aux3s=calloc(VOLUMEPLUSRAND/2, sizeof(spinor));
   sc   =calloc(VOLUMEPLUSRAND/2, sizeof(spinor));
   auxc =calloc(VOLUMEPLUSRAND/2, sizeof(spinor));
   aux3c=calloc(VOLUMEPLUSRAND/2, sizeof(spinor));
#endif

/* For the time being, I set the minimal eigenvalue by hand */
   ev_minev=0.01;
   ev_maxev=1.00;

  if(ini==0){
    dop_cheby_coef = calloc(N_CHEBYMAX,sizeof(double));
    ini=1;
  }

  chebyshev_polynomial(ev_minev, ev_maxev, dop_cheby_coef, N_CHEBYMAX);

  temp=1.0;
  random_spinor_field(ss,VOLUME/2);
  random_spinor_field(sc,VOLUME/2);

  if(g_proc_id == g_stdio_proc) {
    printf("\ndetermine the degree of the polynomial:\n");
  }

  dop_n_cheby=(int)7./sqrt(ev_minev);
  for(i = 0;i < 100 ; i++){
    
    if (dop_n_cheby >= N_CHEBYMAX) {
      if(g_proc_id == g_stdio_proc){
	printf("Error: n_cheby=%d > N_CHEBYMAX=%d\n",dop_n_cheby,N_CHEBYMAX);
	printf("Increase n_chebymax\n");
      }
      errorhandler(35,"degree_of_polynomial");
    }

QdaggerQ_onequarter(&auxs[0], &auxc[0], dop_cheby_coef, dop_n_cheby, &ss[0], &sc[0], ev_minev);


  QdaggerNon_degenerate(&aux3s[0], &aux3c[0], &auxs[0], &auxc[0]);
  QNon_degenerate(&auxs[0], &auxc[0], &aux3c[0], &aux3c[0]);

  QdaggerNon_degenerate(&aux3s[0], &aux3c[0], &auxs[0], &auxc[0]);
  QNon_degenerate(&auxc[0], &auxc[0], &aux3s[0], &aux3c[0]);

  QdaggerNon_degenerate(&aux3s[0], &aux3c[0], &auxs[0], &auxc[0]);
  QNon_degenerate(&auxs[0], &auxc[0], &aux3s[0], &aux3c[0]);

  QdaggerNon_degenerate(&aux3s[0], &aux3c[0], &auxs[0], &auxc[0]);
  QNon_degenerate(&auxs[0], &auxc[0], &aux3s[0], &aux3c[0]);


    diff(&aux3s[0],&auxs[0],&ss[0],VOLUME/2);
    temp=square_norm(&aux3s[0],VOLUME/2)/square_norm(&ss[0],VOLUME)/4.0;
    if(g_proc_id == g_stdio_proc) {
      printf("n_cheby=%d temp_eps=%e\n", dop_n_cheby, temp);
      fflush(stdout);
    diff(&aux3c[0],&auxc[0],&sc[0],VOLUME/2);
    temp=square_norm(&aux3c[0],VOLUME/2)/square_norm(&sc[0],VOLUME)/4.0;
      printf("n_cheby=%d temp_eps=%e\n", dop_n_cheby, temp);
      fflush(stdout);
    }
    if(temp < stopeps ) break;
    dop_n_cheby*=1.05;
  }
  if(g_proc_id == g_stdio_proc) {
    printf("Done!\n");
  }  

  free(ss);
  free(ss_);
  free(sc);
  free(sc_);
}
