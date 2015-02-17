.PHONY: utils database datagraph
all: utils datagraph 

utils:
	${MAKE} -C utils
database:
	${MAKE} -C database

datagraph:
	${MAKE} -C datagraph

clean:
	${MAKE} -C datagraph clean
	${MAKE} -C database clean
	${MAKE} -C utils clean

