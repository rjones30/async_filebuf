t: t.cc async_filebuf.cc async_filebuf.h
	g++ -g -pthread --std=c++11 -I. -o $@ $< async_filebuf.cc
