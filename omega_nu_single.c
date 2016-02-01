#include "omega_nu_single.h"

#include "kspace_neutrino_const.h"
#include "gadget_defines.h"

#include <math.h>
#include <gsl/gsl_integration.h>
#include <gsl/gsl_errno.h>
#include <string.h>

#define HBAR    6.582119e-16  /*hbar in units of eV s*/
#define STEFAN_BOLTZMANN 5.670373e-5
/*Size of matter density tables*/
#define NRHOTAB 200

void init_omega_nu(_omega_nu * omnu, const double MNu[], const double a0, const double HubbleParam)
{
    int mi;
    /*Store conversion between rho and omega*/
    omnu->rhocrit = (3 * HUBBLE * HubbleParam * HUBBLE * HubbleParam)/ (8 * M_PI * GRAVITY);
    /*First compute which neutrinos are degenerate with each other*/
    for(mi=0; mi<NUSPECIES; mi++){
        int mmi;
        omnu->nu_degeneracies[mi]=0;
        for(mmi=0; mmi<mi; mmi++){
            if(fabs(MNu[mi] -MNu[mmi]) < FLOAT){
                omnu->nu_degeneracies[mmi]+=1;
                break;
            }
        }
        if(mmi==mi) {
            omnu->nu_degeneracies[mi]=1;
        }
    }
    /*Now allocate a table for the species we want*/
    for(mi=0; mi<NUSPECIES; mi++){
        if(omnu->nu_degeneracies[mi]) {
            omnu->RhoNuTab[mi] = (_rho_nu_single *) mymalloc("RhoNuTab", sizeof(_rho_nu_single));
            rho_nu_init(omnu->RhoNuTab[mi], a0, MNu[mi], HubbleParam);
        }
        else {
            omnu->RhoNuTab[mi] = NULL;
        }
    }
}

/* Return the total matter density in neutrinos.
 * rho_nu and friends are not externally callable*/
double get_omega_nu(const _omega_nu * const omnu, const double a)
{
        double rhonu=0;
        int mi;
        for(mi=0; mi<NUSPECIES; mi++) {
            if(omnu->nu_degeneracies[mi] > 0){
                 rhonu += omnu->nu_degeneracies[mi] * rho_nu(omnu->RhoNuTab[mi], a);
            }
        }
        return rhonu/omnu->rhocrit;
}


/* Return the total matter density in neutrinos, excluding that in active particles.*/
double get_omega_nu_nopart(const _omega_nu * const omnu, const double a)
{
        double rhonu=0;
        int mi;
        for(mi=0; mi<NUSPECIES; mi++) {
            if(omnu->nu_degeneracies[mi] > 0){
                 double rhonu_s = omnu->nu_degeneracies[mi] * rho_nu(omnu->RhoNuTab[mi], a);
#ifdef HYBRID_NEUTRINOS
                 rhonu_s *= (1-particle_nu_fraction(&omnu->hybnu, a, mi));
#endif
                 rhonu+=rhonu_s;
            }
        }
        return rhonu/omnu->rhocrit;
}

/*Return the photon density*/
double get_omegag(const _omega_nu * const omnu, const double a)
{
    const double omegag = 4*STEFAN_BOLTZMANN/(LIGHTCGS*LIGHTCGS*LIGHTCGS)*pow(T_CMB0,4)/omnu->rhocrit;
    return omegag/pow(a,4);
}

/* Value of kT/aM_nu on which to switch from the
 * analytic expansion to the numerical integration*/
#define NU_SW 100

/*Note q carries units of eV/c. kT/c has units of eV/c.
 * M_nu has units of eV  Here c=1. */
double rho_nu_int(double q, void * params)
{
        double amnu = *((double *)params);
        double epsilon = sqrt(q*q+amnu*amnu);
        double f0 = 1./(exp(q/(BOLEVK*TNU))+1);
        return q*q*epsilon*f0;
}

/*Get the conversion factor to go from (eV/c)^4 to g/cm^3
 * for a **single** neutrino species. */
double get_rho_nu_conversion()
{
        /*q has units of eV/c, so rho_nu now has units of (eV/c)^4*/
        double convert=4*M_PI*2; /* The factor of two is for antineutrinos*/
        /*rho_nu_val now has units of eV^4*/
        /*To get units of density, divide by (c*hbar)**3 in eV s and cm/s */
        const double chbar=1./(2*M_PI*LIGHTCGS*HBAR);
        convert*=(chbar*chbar*chbar);
        /*Now has units of (eV)/(cm^3)*/
        /* 1 eV = 1.60217646 × 10-12 g cm^2 s^(-2) */
        /* So 1eV/c^2 = 1.7826909604927859e-33 g*/
        /*So this is rho_nu_val in g /cm^3*/
        convert*=(1.60217646e-12/LIGHTCGS/LIGHTCGS);
        return convert;
}

/*Seed a pre-computed table of rho_nu values for speed*/
void rho_nu_init(_rho_nu_single * const rho_nu_tab, const double a0, const double mnu, const double HubbleParam)
{
     int i;
     double abserr;
     /*Make the table over a slightly wider range than requested, in case there is roundoff error*/
     const double logA0=log(a0)-log(1.2);
     const double logaf=log(NU_SW*BOLEVK*TNU/mnu)+log(1.2);
     gsl_function F;
     gsl_integration_workspace * w = gsl_integration_workspace_alloc (GSL_VAL);
     F.function = &rho_nu_int;
     /*Initialise constants*/
     rho_nu_tab->mnu = mnu;
     /*Shortcircuit if we don't need to do the integration*/
     if(mnu < 1e-6*BOLEVK*TNU || logaf < logA0)
         return;

     /*Allocate memory for arrays*/
     rho_nu_tab->loga = mymalloc("rho_nu_table",2*NRHOTAB*sizeof(double));
     rho_nu_tab->rhonu = rho_nu_tab->loga+NRHOTAB;
     rho_nu_tab->acc = gsl_interp_accel_alloc();
     rho_nu_tab->interp=gsl_interp_alloc(gsl_interp_cspline,NRHOTAB);
     if(!rho_nu_tab->interp || !rho_nu_tab->acc || !rho_nu_tab->loga)
         terminate("Could not initialise tables for neutrino matter density\n");

     for(i=0; i< NRHOTAB; i++){
        double param;
        rho_nu_tab->loga[i]=logA0+i*(logaf-logA0)/(NRHOTAB-1);
        param=mnu*exp(rho_nu_tab->loga[i]);
        F.params = &param;
        gsl_integration_qag (&F, 0, 500*BOLEVK*TNU,0 , 1e-9,GSL_VAL,6,w,&(rho_nu_tab->rhonu[i]), &abserr);
        rho_nu_tab->rhonu[i]=rho_nu_tab->rhonu[i]/pow(exp(rho_nu_tab->loga[i]),4)*get_rho_nu_conversion();
     }
     gsl_integration_workspace_free (w);
     gsl_interp_init(rho_nu_tab->interp,rho_nu_tab->loga,rho_nu_tab->rhonu,NRHOTAB);
     return;
}


/*Finds the physical density in neutrinos for a single neutrino species
  1.878 82(24) x 10-29 h02 g/cm3 = 1.053 94(13) x 104 h02 eV/cm3*/
double rho_nu(_rho_nu_single * rho_nu_tab, double a)
{
        double rho_nu_val;
        double amnu=a*rho_nu_tab->mnu;
        const double kT=BOLEVK*TNU;
        const double kTamnu2=(kT*kT/amnu/amnu);
        /*Do it analytically if we are in a regime where we can
         * The next term is 141682 (kT/amnu)^8.
         * At kT/amnu = 8, higher terms are larger and the series stops converging.
         * Don't go lower than 50 here. */
        if(NU_SW*NU_SW*kTamnu2 < 1){
            /*Heavily non-relativistic*/
            /*The constants are Riemann zetas: 3,5,7 respectively*/
            rho_nu_val=amnu*(kT*kT*kT)/(a*a*a*a)*
            (1.5*1.202056903159594+kTamnu2*45./4.*1.0369277551433704+2835./32.*kTamnu2*kTamnu2*1.0083492773819229+80325/32.*kTamnu2*kTamnu2*kTamnu2*1.0020083928260826)*get_rho_nu_conversion();
        }
        else if(amnu < 1e-6*kT){
            /*Heavily relativistic: we could be more accurate here,
             * but in practice this will only be called for massless neutrinos, so don't bother.*/
            rho_nu_val=7*pow(M_PI*kT/a,4)/120.*get_rho_nu_conversion();
        }
        else{
            rho_nu_val=gsl_interp_eval(rho_nu_tab->interp,rho_nu_tab->loga,rho_nu_tab->rhonu,log(a),rho_nu_tab->acc);
        }
        return rho_nu_val;
}

#ifdef HYBRID_NEUTRINOS

/*Fermi-Dirac kernel for below*/
double fermi_dirac_kernel(double x, void * params)
{
  return x * x / (exp(x) + 1);
}

/* Fraction of neutrinos not followed analytically
 * This is integral f_0(q) q^2 dq between 0 and qc to compute the fraction of OmegaNu which is in particles.*/
double nufrac_low(const double qc)
{
    /*These functions are so smooth that we don't need much space*/
    gsl_integration_workspace * w = gsl_integration_workspace_alloc (100);
    double abserr;
    gsl_function F;
    F.function = &fermi_dirac_kernel;
    F.params = NULL;
    double total_fd;
    gsl_integration_qag (&F, 0, qc, 0, 1e-6,100,GSL_INTEG_GAUSS61, w,&(total_fd), &abserr);
    /*divided by the total F-D probability (which is 3 Zeta(3)/2 ~ 1.8 if MAX_FERMI_DIRAC is large enough*/
    total_fd /= 1.5*1.202056903159594;
    gsl_integration_workspace_free (w);
    return total_fd;
}

void init_hybrid_nu(_hybrid_nu * const hybnu, const double mnu[], const double vcrit, const double light, const double nu_crit_time)
{
    int i;
    hybnu->nu_crit_time = nu_crit_time;
    hybnu->vcrit = vcrit / light;
    for(i=0; i< NUSPECIES; i++) {
        const double qc = mnu[i] * vcrit / light / (BOLEVK*TNU);
        hybnu->nufrac_low[i] = nufrac_low(qc);
    }
}

/* Returns the fraction of neutrinos currently traced by particles.
 * When neutrinos are fully analytic at early times, returns 0.
 * Last argument: neutrino species to use.
 */
double particle_nu_fraction(const _hybrid_nu * const hybnu, const double a, const int i)
{
    /*Just use a redshift cut for now. Really we want something more sophisticated,
     * based on the shot noise and average overdensity.*/
    if (a > hybnu->nu_crit_time){
        return hybnu->nufrac_low[i];
    }
    return 0;
}

#endif

/* Return the matter density in neutrino species i.*/
double omega_nu_single(const _omega_nu * const omnu, const double a, int i)
{
    /*Deal with case where we want a species degenerate with another one*/
    if(omnu->nu_degeneracies[i] == 0) {
        int j;
        for(j=i; j >=0; j--)
            if(omnu->nu_degeneracies[j]){
                i = j;
                break;
            }
    }
    double omega_nu = rho_nu(omnu->RhoNuTab[i], a)/omnu->rhocrit;
#ifdef HYBRID_NEUTRINOS
    omega_nu *= (1-particle_nu_fraction(&omnu->hybnu, a, i));
#endif
    return omega_nu;

}
