.PHONY: utils instrument kvdb crush database datagraph
all: utils instrument kvdb crush database datagraph

utils:
	${MAKE} -C utils

instrument:
	${MAKE} -C instrument

kvdb:
	${MAKE} -C kvdb

crush:
	${MAKE} -C crush

database:
	${MAKE} -C database

datagraph:
	${MAKE} -C datagraph

clean:
	${MAKE} -C utils clean
	${MAKE} -C instrument clean
	${MAKE} -C kvdb clean
	${MAKE} -C crush clean
	${MAKE} -C datagraph clean
	${MAKE} -C database clean

