==About this code==
This code is an extension for Gadget-3 to allow the cheap inclusion of massive neutrinos.
It implements the linear response method detailed in Ali-Haimoud & Bird 2013 (arxiv:1209.0461)
and the hybrid neutrino method in Bird & Ali-Haimoud 2017 (in prep). It has a full test suite, 
which can be run with 'make test' and aims to be easily portable to a variety of different codes.
You are welcome to use it as desired, but please cite Ali-Haimoud & Bird 2013, and 
Bird & Ali-Haimoud 2017 if you use the hybrid neutrinos.

==Building==
'make all' will make a static library
'make test' will perform the runtime tests.
'make doc' will run doxygen (if installed) and generate html documentation,
which can be browsed by opening doc/html/index.html in a web browser.

==Dependencies==

GSL
The FFT library used in your PM routine.
cmocka for the test suite.

==Parameters of the code==

The neutrino linear response code takes the following parameters, which should 
be set in the parameter file of your N-body code. Parameter file name shows the default key 
as written in the parameter file, while internal variable gives the member of the 
kspace_params structure in interface_common.c . Default values are given where relevant:
Parameter file name         Internal Variable       Default     Description

STRINGS:
KspaceTransferFunction      KspaceTransferFunction    -         File containing CAMB formatted output transfer functions.

FLOATS:
TimeTransfer                TimeTransfer              -         Scale factor at which the CAMB transfer functions were generated.
OmegaBaryonCAMB             OmegaBaryonCAMB           -         OmegaBaryon used for the CAMB transfer functions. 
                                                                If the simulation is DM only, Gadget's OmegaBaryon=0, 
                                                                but CAMB's does not.
InputSpectrum_UnitLength_in_cm   ""                3.085678e24  Units of the CAMB transfer function in cm. By default Mpc.
MNue                        MNu[0]                    -         Mass of the lightest neutrino in eV.
MNum                        MNu[1]                    -         Second neutrino mass in eV.
MNut                        MNu[2]                    -         Third neutrino mass. Note the observed mass splitting is not enforced.
Vcrit                       vcrit                    500        Critical velocity in the Fermi-Dirac distribution below which the neutrinos
                                                                are followed with particles, if hybrid neutrinos are on.
NuPartTime                  nu_crit_time              0.3333    Scale factor at which to 'turn on', ie, make active gravitators, 
                                                                the particle neutrinos, if hybrid neutrinos are on.
INTS:
HybridNeutrinosOn           hybrid_neutrinos_on       0         Whether hybrid neutrinos are enabled.

Note that total_powerspectrum returns a power spectrum which is in units of the box, and unnormalised, 
that is, P(k) * N^2, where N is the number of modes in each bin. After investigation, no attempt 
is made to smooth the power spectrum by averaging neighbouring bins.

==Porting the neutrino library to a new code==

This version of the neutrino integration library is written
to be easy to port to different versions of Gadget, or other 
non-Gadget-based N-body codes. However, due to the wide 
variety of codes in use, some manual adjustment may still be necessary.
This document details the steps to take.

All interfacing between the neutrino integrator and the rest of 
the code goes through the routines in interface_common.c 
and interface_common.h Ideally therefore, you should just 
call these routines at the correct points in your code 
and add the .c files to your Makefile. 
Interfaces specific to Gadget-3, in particular assuming FFTW2 and Gadget's 
parameter reading routines, are found in interface_gadget.[ch]. 
If you are using Gadget-3, you can just include these files.
Note we do not use global Gadget variables, nor Gadget configuration switches.

The three main routines are:
OmegaNu(a): the matter density in neutrinos, should be added to the Hubble function
allocate_kspace_memory(): allocates and sets up the neutrino module. Do it before calling OmegaNu.

add_nu_power_to_rhogrid(): call this inside your PM routine to add the neutrino power to the grid,
Further documentation is provided inside interface_gadget.h

Note that add_nu_power_to_rhogrid assumes the (slab-decomposed) FFTW 2, with a type complex number type fftw_complex,
as this is used in almost all gadget versions. If this does not match your code, the routine 
compute_neutrino_power_from_cdm takes a pre-computed matter power spectrum, and you should adapt the for loop
in add_nu_power_to_rhogrid to your own FFT routines.

The .c files which need to be compiled in are:
delta_pow.c - GSL interpolation for neutrino power spectra
delta_tot_table.c - Core integrator that computes delta_nu given a matter power spectrum.
interface_common.c - Routines to do the messy business of interfacing with Gadget. 
                       Also stores the state for the neutrino code in the form of global variables.
interface_gadget.c - Interface routines which assume FFTW2 and are only suitable for Gadget-3.
                       Also stores the state for the neutrino code in the form of global variables.
powerspectrum.c - Routine to compute the power spectrum of a Fourier-transformed density field, 
                    divided up between processors as by FFTW2.
omega_nu_single.c -  Routines to compute OmegaNu and OmegaR 
transfer_init.c - Routine to read and parse CAMB formatter transfer functions.

Other c files are: 
*_test.c - cmocka tests for each module.
gadget_defines.c - support infrastructure normally in gadget for the tests.

You should also provide a routine to read the parameters required 
by the neutrino integrator from your code's parameter file. 
An example routine for Gadget-3 called set_kspace_vars is provided in
interface_gadget.h

Your code should provide the routines declared in gadget_defines.h.
These are:
mymalloc, myfree - free and allocate memory.
hubble_function - H(a)
terminate - ends the simulation.
In case your code does not provide them, there are simple examples 
defined in gadget_defines.c, primarily for the test suite.
