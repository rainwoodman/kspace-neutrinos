#ifndef KSPACE_NEUTRINO_VARS
#define KSPACE_NEUTRINO_VARS

//Global variables that need to be set from a parameter file
struct __kspace_vars {
  double OmegaNu;
  char	KspaceTransferFunction[500];
  double TimeTransfer;
  double OmegaBaryonCAMB;
  double InputSpectrum_UnitLength_in_cm;
  /*Allow for three massive neutrino species:
   * Could be made configurable at some point
   * Neutrino masses are in eV*/
  #define NUSPECIES 3
  double MNu[NUSPECIES];
#endif
#if defined HYBRID_NEUTRINOS
    /*Two parameters for the hybrid neutrinos.
    If this is true, then we proceed using the analytic method for all neutrinos.
    If this is false, then we cut off the analytic method at q < qcrit (specified using vcrit, below) and use
    particles for the slower neutrinos.*/
    int slow_neutrinos_analytic;
    /*Critical velocity above which to treat neutrinos with particles.
    Note this is unperturbed velocity *TODAY*
    To get velocity at redshift z, multiply by (1+z)*/
    double vcrit;
    //Time at which to turn on the particle neutrinos.
    //Ultimately we want something better than this.
    double nu_crit_time;
#endif
} kspace_vars;

#endif
