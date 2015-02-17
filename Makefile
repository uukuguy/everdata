all: bin
	${MAKE} -C src

clean:
	${MAKE} -C src clean

bin:
	mkdir -p ../../bin

