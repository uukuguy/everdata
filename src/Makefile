.PHONY: utils instrument kvdb database datagraph
all: utils instrument kvdb database datagraph

utils:
	${MAKE} -C utils

instrument:
	${MAKE} -C instrument

kvdb:
	${MAKE} -C kvdb

database:
	${MAKE} -C database

datagraph:
	${MAKE} -C datagraph

clean:
	${MAKE} -C utils clean
	${MAKE} -C instrument clean
	${MAKE} -C kvdb clean
	${MAKE} -C datagraph clean
	${MAKE} -C database clean

