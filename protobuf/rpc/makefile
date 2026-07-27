src = $(wildcard ./*pb.cc)
all : client server
client : $(src) client.cc
	g++ -g -std=c++11 $^ -o $@ -lprotobuf -lgrpc -lgrpc++ -lgpr -labsl_synchronization -pthread
server : $(src) server.cc
	g++ -g -std=c++11 $^ -o $@ -lprotobuf -lgrpc -lgrpc++ -lgpr -labsl_synchronization -pthread