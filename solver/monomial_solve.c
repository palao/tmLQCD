/***********************************************************************
 *
 * Copyright (C) 2014 Florian Burger
 *
 * This file is part of tmLQCD.
 *
 * tmLQCD is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * tmLQCD is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with tmLQCD.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  
 * File: monomial_solve.c
 *
 * solver wrapper for monomials
 *
 * The externally accessible functions are
 *
 *
 *   int solve_degenerate(spinor * const P, spinor * const Q, const int max_iter, 
           double eps_sq, const int rel_prec, const int N, matrix_mult f)
 *   int solve_mms_nd(spinor ** const Pup, spinor ** const Pdn, 
 *                    spinor * const Qup, spinor * const Qdn, 
 *                    solver_params_t * solver_params)  
 *
 **************************************************************************/


#ifdef HAVE_CONFIG_H
# include<config.h>
#endif
#include "global.h"
#include "read_input.h"
#include "linalg/mul_gamma5.h"
#include "linalg/diff.h"
#include "linalg/square_norm.h"
#include "linalg/mul_r_gamma5.h"
#include "gamma.h"
// for the non-degenerate operator normalisation
#include "phmc.h"
#include "solver/solver.h"
#include "solver/solver_field.h"
#include "solver/matrix_mult_typedef.h"
#include "solver/solver_types.h"
#include "solver/solver_params.h"
#include "operator/tm_operators.h"
#include "operator/tm_operators_32.h"
#include "operator/tm_operators_nd.h"
#include "operator/tm_operators_nd_32.h"
#include "operator/clovertm_operators.h"
#include "operator/clovertm_operators_32.h"
#include "misc_types.h"
#include "monomial_solve.h"
#ifdef DDalphaAMG
#include "DDalphaAMG_interface.h"
#endif
#ifdef TM_USE_QPHIX
#include "qphix_interface.h"
#endif

#include <io/params.h>
#include <io/spinor.h>

/* BaKo: this and the stuff below will be removed as soon as everything works
 *       for all operators */       
#define WIP

#ifdef HAVE_GPU
#include"../GPU/cudadefs.h"
extern  int linsolve_eo_gpu (spinor * const P, spinor * const Q, const int max_iter, 
                            double eps, const int rel_prec, const int N, matrix_mult f);
extern int dev_cg_mms_tm_nd(spinor ** const Pup, spinor ** const Pdn, 
		 spinor * const Qup, spinor * const Qdn, 
		 solver_params_t * solver_params);
   #ifdef TEMPORALGAUGE
     #include "../temporalgauge.h" 
   #endif
#include "read_input.h" 
#endif

int solve_degenerate(spinor * const P, spinor * const Q, solver_params_t solver_params,
                     const int max_iter, double eps_sq, const int rel_prec, 
                     const int N, matrix_mult f, int solver_type){
  int iteration_count = 0;
  int use_solver = solver_type;
#ifdef TM_USE_QPHIX
  if(solver_params.external_inverter == QPHIX_INVERTER){
    spinor** temp;
    init_solver_field(&temp, VOLUMEPLUSRAND/2, 1);
    
    // using CG for the HMC, we always want to have the solution of (Q Q^dagger) x = b, which is equivalent to
    // gamma_5 (M M^dagger)^{-1} gamma_5 b
    // FIXME: this needs to be adjusted to also support BICGSTAB
    gamma5(temp[0], Q, VOLUME/2);
    iteration_count = invert_eo_qphix_oneflavour(P, temp[0], max_iter, eps_sq, solver_type, 
                                                 rel_prec, solver_params, solver_params.sloppy_precision, solver_params.compression_type);
    mul_gamma5(P, VOLUME/2);

#ifdef WIP
    f(temp[0], P);
    diff(temp[0], temp[0], Q, VOLUME/2);
    double diffnorm = square_norm(temp[0], VOLUME/2, 1); 
    if( g_proc_id == 0 ){
      printf("# QPhiX residual check: %e\n", diffnorm);
    }
#endif // WIP
    finalize_solver(temp, 1);
    return(iteration_count);
  } else
#endif  
  if(use_solver == MIXEDCG || use_solver == RGMIXEDCG){
    // the default mixed solver is rg_mixed_cg_her
    int (*msolver_fp)(spinor * const, spinor * const, solver_params_t, 
                      const int, double, const int, const int, matrix_mult, matrix_mult32) = rg_mixed_cg_her;

    // but it might be necessary at some point to use the old version
    if(use_solver == MIXEDCG){
      msolver_fp = mixed_cg_her;
    }

    if(usegpu_flag){   
      #ifdef HAVE_GPU     
	      #ifdef TEMPORALGAUGE
          to_temporalgauge(g_gauge_field, Q , P);
        #endif          
        iteration_count = linsolve_eo_gpu(P, Q, max_iter, eps_sq, rel_prec, N, f);
        #ifdef TEMPORALGAUGE
          from_temporalgauge(Q, P);
        #endif
      #endif
      return(iteration_count);
    }
    else{
      if(f==Qtm_pm_psi){   
        iteration_count =  msolver_fp(P, Q, solver_params, max_iter, eps_sq, rel_prec, N, f, &Qtm_pm_psi_32);
        return(iteration_count);
      }
      else if(f==Q_pm_psi){     
	iteration_count =  msolver_fp(P, Q, solver_params, max_iter, eps_sq, rel_prec, N, f, &Q_pm_psi_32);
	return(iteration_count);      
      } else if(f==Qsw_pm_psi){
        copy_32_sw_fields();
        iteration_count = msolver_fp(P, Q, solver_params, max_iter, eps_sq, rel_prec, N, f, &Qsw_pm_psi_32);
        return(iteration_count);
      } else {
        if(g_proc_id==0) printf("Warning: 32 bit matrix not available. Falling back to CG in 64 bit\n"); 
        use_solver = CG;
      }
    }
  } 
  if(use_solver == CG){
    iteration_count =  cg_her(P, Q, max_iter, eps_sq, rel_prec, N, f);
  }
  else if(use_solver == BICGSTAB){
     iteration_count =  bicgstab_complex(P, Q, max_iter, eps_sq, rel_prec, N, f);     
  }
#ifdef DDalphaAMG 
  else if (use_solver == MG)
    iteration_count =  MG_solver(P, Q, eps_sq, max_iter,rel_prec, N , g_gauge_field, f);
#endif     
  else{
    if(g_proc_id==0) printf("Error: solver not allowed for degenerate solve. Aborting...\n");
    exit(2);
  }
#ifdef WIP
    spinor** temp;
    init_solver_field(&temp, VOLUMEPLUSRAND/2, 1);
    f(temp[0], P);
    diff(temp[0], temp[0], Q, VOLUME/2);
    double diffnorm = square_norm(temp[0], VOLUME/2, 1); 
    if( g_proc_id == 0 ){
      printf("# tmLQCD residual check: %e\n", diffnorm);
    }
    finalize_solver(temp, 1);
#endif // WIP
  return(iteration_count);
}

int solve_mshift_oneflavour(spinor ** const P, spinor * const Q, solver_params_t* solver_params){
  int iteration_count = 0;
  spinor ** temp;
  
#ifdef TM_USE_QPHIX
  if( solver_params->external_inverter == QPHIX_INVERTER ){
    spinor** temp;
    init_solver_field(&temp, VOLUMEPLUSRAND/2, 1);
    gamma5(temp[0], Q, VOLUME/2);
    iteration_count = invert_eo_qphix_oneflavour_mshift(P, temp[0],
                                                        solver_params->max_iter, solver_params->squared_solver_prec,
                                                        solver_params->type, solver_params->rel_prec,
                                                        *solver_params,
                                                        solver_params->sloppy_precision,
                                                        solver_params->compression_type);
    for( int shift = 0; shift < solver_params->no_shifts; shift++){
      mul_gamma5(P[shift], VOLUME/2);
    }
#ifdef WIP
    // FIXME: in the shift-by-shift branch, the shifted operator exists explicitly and could be used to 
    // truly check the residual here
    solver_params->M_psi(temp[0], P[0]);
    diff(temp[0], temp[0], Q, VOLUME/2);
    double diffnorm = square_norm(temp[0], VOLUME/2, 1); 
    if( g_proc_id == 0 ){
      printf("# QPhiX residual check: %e\n", diffnorm);
    }
#endif // WIP
    finalize_solver(temp, 1);
    return(iteration_count);
  }
#endif // TM_USE_QPHIX

  double reached_prec = -1.0;
  iteration_count = cg_mms_tm(P, Q, solver_params, &reached_prec);
#ifdef WIP
  init_solver_field(&temp, VOLUMEPLUSRAND/2, 1);
  // FIXME: in the shift-by-shift branch, the shifted operator exists explicitly and could be used to 
  // truly check the residual here
  solver_params->M_psi(temp[0], P[0]);
  diff(temp[0], temp[0], Q, VOLUME/2);
  double diffnorm = square_norm(temp[0], VOLUME/2, 1); 
  if( g_proc_id == 0 ){
    printf("# tmLQCD residual check: %e\n", diffnorm);
  }
  finalize_solver(temp, 1);
#endif // WIP      

  return iteration_count;
}

void write_mms_nd_props(spinor** const Pup, spinor ** const Pdn, solver_params_t * solver_params, const char * name,
                        const int iteration_count ){
  int append = 0;
  char filename[300];
  WRITER * writer = NULL;
  paramsInverterInfo *inverterInfo = NULL;
  paramsPropagatorFormat *propagatorFormat = NULL;
  
  const int num_flavour = 1;
  const int precision = 64;

  sprintf(filename, "%s.cgmms.prop", name);
  
  for(int im = 0; im < solver_params->no_shifts; im++) {
    if(im==0) construct_writer(&writer, filename, 0);
    else construct_writer(&writer, filename, 1);
    
    if (im==0) {
      inverterInfo = construct_paramsInverterInfo(0.0, iteration_count, DBTMWILSON, num_flavour);
      inverterInfo->cgmms_mass = solver_params->shifts[im]/(2 * g_kappa);
      write_spinor_info(writer, 0, inverterInfo, 0);
      free(inverterInfo);
    }
    
    propagatorFormat = construct_paramsPropagatorFormat(precision, num_flavour);
    write_propagator_format(writer, propagatorFormat);
    free(propagatorFormat);
    
    int status = write_spinor(writer, &Pup[im], &Pdn[im], num_flavour, precision);
    if(g_proc_id==0) printf("Writer status: %d\n", status);
    
    destruct_writer(writer);
  }
}

int solve_mms_nd(spinor ** const Pup, spinor ** const Pdn, 
                 spinor * const Qup, spinor * const Qdn, 
                 solver_params_t * solver_params){ 
  int iteration_count = 0; 
    if(solver_params->type==MIXEDCGMMSND){
      if(usegpu_flag){
	#ifdef HAVE_GPU      
	  #ifdef TEMPORALGAUGE
	    to_temporalgauge_mms(g_gauge_field , Qup, Qdn, Pup, Pdn, solver_params->no_shifts);
	  #endif        
	  iteration_count = dev_cg_mms_tm_nd(Pup, Pdn, Qup, Qdn, solver_params);  
	  #ifdef TEMPORALGAUGE
	    from_temporalgauge_mms(Qup, Qdn, Pup, Pdn, solver_params->no_shifts);
	  #endif 
	#endif
      }
      else{
	iteration_count = mixed_cg_mms_tm_nd(Pup, Pdn, Qup, Qdn, solver_params);
      }
    }
    else if (solver_params->type==CGMMSND){
      spinor** temp;
      
      spinor** checkPup;
      spinor** checkPdn;
#ifdef TM_USE_QPHIX
      if( solver_params->external_inverter == QPHIX_INVERTER ){
        init_solver_field(&temp, VOLUMEPLUSRAND/2, 2);
        //  gamma5 (M.M^dagger)^{-1} gamma5 = [ Q(+mu,eps) Q(-mu,eps) ]^{-1}
        gamma5(temp[0], Qup, VOLUME/2);
        gamma5(temp[1], Qdn, VOLUME/2);
        iteration_count = invert_eo_qphix_twoflavour_mshift(Pup, Pdn, temp[0], temp[1],
//                                                             Pdn, Pup, temp[1], temp[0],
                                                            solver_params->max_iter, solver_params->squared_solver_prec,
                                                            solver_params->type, solver_params->rel_prec,
                                                            *solver_params,
                                                            solver_params->sloppy_precision,
                                                            solver_params->compression_type);
        
        // the tmLQCD ND operator used for HMC is normalised by the inverse of the maximum eigenvalue
        // so the inverse of Q^2 is normalised by the square of the maximum eigenvalue
        // or, equivalently, the square of the inverse of the inverse
        // note that in the QPhiX interface, we also correctly normalise the shifts
        const double maxev_sq = (1.0/phmc_invmaxev)*(1.0/phmc_invmaxev);
        for( int shift = 0; shift < solver_params->no_shifts; shift++){
          mul_r_gamma5(Pup[shift], maxev_sq, VOLUME/2);
          mul_r_gamma5(Pdn[shift], maxev_sq, VOLUME/2);
        }
        
//         init_solver_field(&checkPup, VOLUMEPLUSRAND/2, solver_params->no_shifts);
//         init_solver_field(&checkPdn, VOLUMEPLUSRAND/2, solver_params->no_shifts);
//         int tm_iteration_count = cg_mms_tm_nd(checkPup, checkPdn, Qup, Qdn, solver_params);
//         
//         write_mms_nd_props(Pup,Pdn,solver_params,"qphix",iteration_count);
//         write_mms_nd_props(checkPup,checkPdn,solver_params,"tmlqcd",tm_iteration_count);
//         
//         MPI_Barrier(MPI_COMM_WORLD);
//         fflush(stdout);
//         MPI_Barrier(MPI_COMM_WORLD);
//         
//         fatal_error("Debug: stopping here\n","solve_mms_nd");
//         
//         for(int shift = 0; shift < solver_params->no_shifts; shift++){
//           diff(temp[0], checkPup[shift], Pup[shift], VOLUME/2);
//           diff(temp[1], checkPdn[shift], Pdn[shift], VOLUME/2);
//           
//           if(g_proc_id==0){
//             printf("|| Pup[%d] - checkPup[%d] ||^2 = %.6e \n", shift, shift, square_norm(temp[0],VOLUME/2,1));
//             printf("|| Pdn[%d] - checkPdn[%d] ||^2 = %.6e \n", shift, shift, square_norm(temp[1],VOLUME/2,1));
//           }
//         }
//         finalize_solver(checkPup, solver_params->no_shifts);
//         finalize_solver(checkPdn, solver_params->no_shifts);
#ifdef WIP
//         FIXME: in the shift-by-shift branch, the shifted operator exists explicitly and could be used to 
//         truly check the residual here
        solver_params->M_ndpsi(temp[0], temp[1], Pup[0], Pdn[0]);
        diff(temp[0], temp[0], Qup, VOLUME/2);
        diff(temp[1], temp[1], Qdn, VOLUME/2);
        double diffnorm = square_norm(temp[0], VOLUME/2, 1) + square_norm(temp[1], VOLUME/2, 1); 
        if( g_proc_id == 0 ){
          printf("# QPhiX residual check: %e\n", diffnorm);
        }
#endif // WIP
        finalize_solver(temp, 2);
        return(iteration_count);
      }
#endif
      iteration_count = cg_mms_tm_nd(Pup, Pdn, Qup, Qdn, solver_params);
#ifdef WIP
      init_solver_field(&temp, VOLUMEPLUSRAND/2, 2);
      // FIXME: in the shift-by-shift branch, the shifted operator exists explicitly and could be used to 
      // truly check the residual here
      solver_params->M_ndpsi(temp[0], temp[1], Pup[0], Pdn[0]);
      diff(temp[0], temp[0], Qup, VOLUME/2);
      diff(temp[1], temp[1], Qdn, VOLUME/2);
      double diffnorm = square_norm(temp[0], VOLUME/2, 1) + square_norm(temp[1], VOLUME/2, 1); 
      if( g_proc_id == 0 ){
        printf("# tmLQCD residual check: %e\n", diffnorm);
      }
      finalize_solver(temp, 2);
#endif // WIP      
    }
    else{
      if(g_proc_id==0) printf("Error: solver not allowed for ND mms solve. Aborting...\n");
      exit(2);      
    }
  return(iteration_count);
}
