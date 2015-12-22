OPT   +=  -DKSPACE_NEUTRINOS_2  # Enable kspace neutrinos

LFLAGS += -lgsl -lgslcblas -lpthread
CFLAGS +=-O2 -ffast-math -g -c -Wall -fopenmp ${OPT}
LFLAGS += -lm -lgomp

OBJS = kspace_neutrinos_2.o kspace_neutrino_background.o kspace_neutrino_load.o kspace_neutrinos_add_pm_functions.o
INCL = kspace_neutrino_const.h kspace_neutrinos_vars.h kspace_neutrinos_func.h Makefile

.PHONY : clean all test

all: ${OBJS}

test: btest
	./$^

%.o: %.c ${INCL}
	$(CC) $(CFLAGS) $< -o $@

test.o: test.cpp ${INCL}
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $< -o $@

btest: test.o ${OBJS}
	${LINK} ${LFLAGS} -lboost_unit_test_framework $^ -o  $@
clean:
	rm -f $(OBJS)
