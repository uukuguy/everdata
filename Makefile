all: deps bin
	${MAKE} -C src

clean:
	${MAKE} -C src clean

bin:
	mkdir -p ../../bin

deps:
	${MAKE} -C deps

.PHONY: data
data:
	mkdir -p data/samples
	dd if=/dev/urandom of=data/samples/320.dat bs=320 count=1
	dd if=/dev/urandom of=data/samples/640.dat bs=640 count=1
	dd if=/dev/urandom of=data/samples/1K.dat bs=1000 count=1
	dd if=/dev/urandom of=data/samples/4K.dat bs=1000 count=4
	dd if=/dev/urandom of=data/samples/8K.dat bs=1000 count=8
	dd if=/dev/urandom of=data/samples/16K.dat bs=1000 count=16
	dd if=/dev/urandom of=data/samples/32K.dat bs=1000 count=32
	dd if=/dev/urandom of=data/samples/64K.dat bs=1000 count=64
	dd if=/dev/urandom of=data/samples/100K.dat bs=1000 count=100
	dd if=/dev/urandom of=data/samples/320K.dat bs=1000 count=320
	dd if=/dev/urandom of=data/samples/640K.dat bs=1000 count=640
	dd if=/dev/urandom of=data/samples/1M.dat bs=1000000 count=1
	dd if=/dev/urandom of=data/samples/2M.dat bs=1000000 count=2

