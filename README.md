# c-async-disk-api
C API using select for async socket connections. 

Uses a few bin files as disk storage

## profiling
pyenv local 3.10.9
gprof -f handleRequest out | gprof2dot | dot -Tpng -o profiling/output.png