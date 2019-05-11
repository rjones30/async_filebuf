//
// t.cc - simple example program to demonstrate use of async_filebuf.
//        To use it, replace the string literal in fname with the name
//        of a large data stream input source. This can also be a file
//        on a local filesystem, but in that case the features of the
//        async_filebuf class are largely redundant with the kernel's
//        own memory-mapped file i/o buffering facility.
//
//        As written, this example uses xrootd as the input source. It
//        assumes assumes that the user has the xrootd POSIX library
//        preloaded, as in
//
//           export LD_PRELOAD=/usr/local/lib/libXrdPosixPreload.so
//
// author: richard.t.jones at uconn.edu
// version: may 11, 2019

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

#include "async_filebuf.h"

std::string fname("root://nod29.phys.uconn.edu/Gluex/rawdata/PSskims/Run010593/hd_rawdata_010593_000.ps.evio");

int main()
{
   std::string fname("root://nod29.phys.uconn.edu/Gluex/rawdata/PSskims/Run010593/hd_rawdata_010593_000.ps.evio");
   async_filebuf sb(1000000, 10, 2);
   sb.open(fname, std::ios::in);
   std::ifstream ifs;
   ifs.std::ios::rdbuf(&sb);

   const int bufsize = 100000;
   char buf[bufsize];
   int count=0;
   while (ifs.read(buf, bufsize*sizeof(buf[0]))) {
      std::cout << count << ": got " << ifs.gcount() << " bytes, ";
      int off = (rand() % 300000) - 150000;
      if (! ifs.seekg(off, std::ios::cur)) {
         ifs.seekg(-1, std::ios::end);
      }
      std::cout << "now at offset " << ifs.tellg() << std::endl;
      count += 1;
   }
   std::cout << count << " total blocks read" << std::endl;
   if (ifs.eof()) {
      std::cout << "loop terminated with eof" << std::endl;
   }
   else if (ifs.fail()) {
      std::cout << "loop terminated at input error" << std::endl;
   }
   else if (ifs.bad()) {
      std::cout << "loop terminated at bad input" << std::endl;
   }
   else {
      std::cout << "loop terminated for no apparent reason" << std::endl;
   }
}
