.PHONY: utils database datagraph
all: utils instrument datagraph 

utils:
	${MAKE} -C utils

instrument:
	${MAKE} -C instrument

database:
	${MAKE} -C database

datagraph:
	${MAKE} -C datagraph

clean:
	${MAKE} -C utils clean
	${MAKE} -C instrument clean
	${MAKE} -C datagraph clean
	${MAKE} -C database clean

