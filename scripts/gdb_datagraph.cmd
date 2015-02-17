directory ./src/datagraph/model:./src/datagraph/ldac:./src/datagraph/segmenter:./src/datagraph/svd

set args --corpus_rootdir ./corpus

handle SIGPIPE nostop print
b main
run

