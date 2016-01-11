#ifndef TRANSFER_INIT_H
#define TRANSFER_INIT_H

/* Structure to store the initial transfer functions from CAMB.
 * We store transfer functions because we want to use the
 * CDM + Baryon total matter power spectrum from the
 * first timestep of Gadget, so that possible Rayleigh scattering
 * in the initial conditions is included in the neutrino and radiation components. */
struct _transfer_init_table {
    int NPowerTable;
    double *logk;
    /*This is T_nu / (T_not-nu), where T_not-nu is a weighted average of T_cdm and T_baryon*/
    double *T_nu;
};
typedef struct _transfer_init_table _transfer_init_table;

void allocate_transfer_init_table(_transfer_init_table *t_init, const double BoxSize, const double UnitLength_in_cm, const double InputSpectrum_UnitLength_in_cm, const double OmegaBaryonCAMB, const double OmegaNu, const double Omega0, const char * KspaceTransferFunction);

void free_transfer_init_table(_transfer_init_table *t_init);

/*TRANSFER_INIT_H*/
#endif
