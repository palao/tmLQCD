/* $Id$ */

/*******************************************************************************
 * Generalized minimal residual (GMRES) with a maximal number of restarts.    
 * Solves Q=AP for complex regular matrices A.
 * For details see: Andreas Meister, Numerik linearer Gleichungssysteme        
 *   or the original citation:                                                 
 * Y. Saad, M.H.Schultz in GMRES: A generalized minimal residual algorithm    
 *                         for solving nonsymmetric linear systems.            
 * 			SIAM J. Sci. Stat. Comput., 7: 856-869, 1986           
 *           
 * int gmres(spinor * const P,spinor * const Q, 
 *	   const int m, const int max_restarts,
 *	   const double eps_sq, matrix_mult f)
 *                                                                 
 * Returns the number of iterations needed or -1 if maximal number of restarts  
 * has been reached.                                                           
 *
 * Inout:                                                                      
 *  spinor * P       : guess for the solving spinor                                             
 * Input:                                                                      
 *  spinor * Q       : source spinor
 *  int m            : Maximal dimension of Krylov subspace                                     
 *  int max_restarts : maximal number of restarts                                   
 *  double eps       : stopping criterium                                                     
 *  matrix_mult f    : pointer to a function containing the matrix mult
 *                     for type matrix_mult see matrix_mult_typedef.h
 *
 * Autor: Carsten Urbach <urbach@ifh.de>
 ********************************************************************************/

#ifndef _GMRES_H
#define _GMRES_H

#include"solver/matrix_mult_typedef.h"
#include"su3.h"

int gmres(spinor * const P,spinor * const Q, 
	   const int m, const int max_restarts,
	  const double eps, matrix_mult f);


#endif