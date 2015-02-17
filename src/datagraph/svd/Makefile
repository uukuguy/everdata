TARGET = libsvd.a
REDSVD = redsvd-0.2.0

OBJS = svd.cc.o \
	   svd_svdlibc.cc.o \
	   svd_redsvd.cc.o \
	   svd_eigen.cc.o \
	   svd_armadillo.cc.o \
	   #svd_octave.cc.o

include ../../Makefile.common

FINAL_CXXFLAGS += -I../model -I../../utils -I/opt/local/include/libxml2 -I/opt/local/include/octave-3.8.2/octave

all: ${TARGET}

${TARGET}: redsvd ${OBJS}
	${AR} cruv ${TARGET} ${OBJS}

clean:
	rm -fr ${TARGET} ${OBJS} ${REDSVD} redsvd

# ---------------- redsvd ----------------

.PHONY: redsvd

redsvd: ${REDSVD}
	${MAKE} -C ${REDSVD}

${REDSVD}:
	tar zxvf ${REDSVD}.tar.gz && \
	ln -s ${REDSVD}/src redsvd

