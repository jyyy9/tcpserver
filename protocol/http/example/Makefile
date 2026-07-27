NET_SRC=$(wildcard ../../../net/*.cc)
HTTP_SRC=$(wildcard ../*.cc)
server : server.cc $(NET_SRC) $(HTTP_SRC)
	g++ -g -std=c++17 $^ -o $@ -pthread