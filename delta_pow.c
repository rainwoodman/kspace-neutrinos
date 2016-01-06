#include <stdio.h>
#include <gsl/gsl_interp.h>
#include <math.h>
#include "delta_pow.h"
#include "gadget_defines.h"

void init_delta_pow(_delta_pow *d_pow, double logkk[], double delta_nu_curr[], double delta_cdm_curr[], int nbins)
{
  /*Set up array pointers*/
  d_pow->logkk = logkk;
  d_pow->delta_nu_curr = delta_nu_curr;
  d_pow->delta_cdm_curr = delta_cdm_curr;
  d_pow->nbins = nbins;
  /*Set up interpolation structures*/
  d_pow->acc = gsl_interp_accel_alloc();
  d_pow->spline_cdm=gsl_interp_alloc(gsl_interp_cspline,nbins);
  gsl_interp_init(d_pow->spline_cdm,d_pow->logkk,d_pow->delta_cdm_curr,nbins);
  d_pow->acc_nu = gsl_interp_accel_alloc();
  d_pow->spline_nu=gsl_interp_alloc(gsl_interp_cspline,nbins);
  gsl_interp_init(d_pow->spline_nu,d_pow->logkk,d_pow->delta_nu_curr,nbins);
}

/*Get the neutrino power spectrum. This will become:
 * \delta_\nu = \delta_{CDM}(\vec{k}) * delta_\nu(k) / delta_{CDM} (k),
 * thus we get the right powerspectrum.
 * @param kk log(k) value to get delta_nu at
 * @param logkk[] vector of k values corresponding to delta_nu_curr
 * @param delta_nu_curr vector of delta_nu
 * @param spline_nu interpolating spline for delta_nu_curr
 * @param acc_nu accelerator for spline_nu
 * @param delta_cdm_curr vector of delta_cdm
 * @param spline_cdm interpolating spline for delta_cdm_curr
 * @param acc accelerator for spline_cdm
 * @param nbins number of bins in delta_nu_curr and delta_cdm_curr
 * @returns delta_nu / delta_CDM
 * */
double get_dnudcdm_powerspec(_delta_pow *d_pow, double kk)
{
        double delta_cdm,delta_nu;
        /* Floating point roundoff and the binning means there may be a mode just beyond the box size.
         * For now we assume P(k) is constant on these large scales.
         * At some point in the future, linear extrapolation could be used.*/
        if(kk < d_pow->logkk[0] && kk > d_pow->logkk[0]-log(2) ){
            kk = d_pow->logkk[0];
        }
        if(kk < d_pow->logkk[0]-log(2)){
                char err[300];
                snprintf(err,300,"trying to extract a k= %g < min stored = %g \n",kk,d_pow->logkk[0]);
                terminate(err);
        }
        /*This is just to guard against floating point roundoff*/
        if( kk > d_pow->logkk[d_pow->nbins-1])
                kk=d_pow->logkk[d_pow->nbins-1];
        delta_cdm=gsl_interp_eval(d_pow->spline_cdm,d_pow->logkk, d_pow->delta_cdm_curr,kk,d_pow->acc);
        delta_nu=gsl_interp_eval(d_pow->spline_nu,d_pow->logkk, d_pow->delta_nu_curr, kk,d_pow->acc_nu);
        if(isnan(delta_cdm) || isnan(delta_nu))
                terminate("delta_nu or delta_cdm is nan\n");
        return delta_nu/delta_cdm;
}

/*Save the neutrino power spectrum to disc*/
void save_nu_power(_delta_pow *d_pow, const double Time, const int snapnum, const char * OutputDir)
{
    FILE *fd;
    int i;
    char nu_fname[1000];
    /*The last underscore in the filename will be just before the snapshot number.
    * This is daft, but works.*/
    snprintf(nu_fname, 1000,"%s/powerspec_nu_%d", OutputDir, snapnum);
    if(!(fd = fopen(nu_fname, "w"))){
        char buf[1000];
        snprintf(buf, 1000, "can't open file `%s` for writing\n", nu_fname);
        terminate(buf);
    }
    fprintf(fd, "%g\n", Time);
    fprintf(fd, "%d\n", d_pow->nbins);
    for(i = 0; i < d_pow->nbins; i++){
        fprintf(fd, "%g %g\n", exp(d_pow->logkk[i]), d_pow->delta_nu_curr[i]*d_pow->delta_nu_curr[i]);
    }
    fclose(fd);
    return;
}

void free_d_pow(_delta_pow * d_pow)
{
  gsl_interp_free(d_pow->spline_nu);
  gsl_interp_accel_free(d_pow->acc_nu);
  gsl_interp_free(d_pow->spline_cdm);
  gsl_interp_accel_free(d_pow->acc);
}
