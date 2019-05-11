# async_filebuf
Plug-in replacement for std::filebuf providing asynchronous read-ahead and buffered quasi-random access.

# what
This project was created to meet a need for an ifstream class that provides a flexible scheme for read access to a file-oriented stream coming from a high-latency source. In my use case, this high-latency source is xrootd (see http://xrootd.org) which aims at giving high performance, scalable fault tolerant access to data repositories of many kinds. There are multiple ways to access data over xrootd, but the one most interesting to me is the userspace preload library libXrdPosixPreload.so which allows an application to issue an ordinary open on an ifstream to a url like

```
root://nod29.phys.uconn.edu/Gluex/rawdata/PSskims/Run010593/hd_rawdata_010593_000.ps.evio
```

and use ordinary ifstream semantics like read, <<, seekg, etc., to access the data as if they were contained in a file on a locally mounted filesystem. The preload library translates these file operations into network requests that are transmitted over a control tcp channel to the xrootd server, which streams the requested data back to the client over one or more tcp data channels. The xrootd protocol is well optimized for random access to remote data over high latency connections (e.g. between continents) and the preload library mechanism allows it to be used with an existing executable without changing the code or recompiling. But when I tried it with my particular Big Data application, I found the latency was killing my performance for access to files served from remote xrootd servers. This async_filebuf class completely fixed the problem, giving the same performance on remote files as is obtained on files stored on a locally mounted filesystem, e.g. nfs.

# why
Of course I could copy the file to local storage first, and then access it from there. Or I could install and use the xrootd cacheing daemon that is designed to cover use cases like the one I describe above. There are pros and cons to each of these approaches, but both of them would require me to have enough local storage to contain at least one input file on each of my worker nodes, which limits the nodes I can run on. Furthermore, it is completely unnecessary as these data consist of unrelated blocks of size less than 10MB, contained in an input file that may be several hundred GB in size. The total volume of the data being analyzed is tens of PB, hosted at multiple sites around North America. Live-streaming the data during analysis rather than pre-staging avoids the extra local data movement to/from local disk, and gives a more smooth network traffic pattern on my servers.

# how

Suppose I have an application class that opens an input file as follows, where my_input_data_file may be any valid file on a locally mounted filesystem, or an xrootd-style url like the one listed above.

```
#include <fstream>
...
std::ifstream ifs("my_input_data_file");
```

This code should be modified as follows to use the async_filebuf read-ahead buffering class. No other changes to the application code are needed. The g++ compiler needs the options "--std=c++11" and "-pthread" to build any application using async_filebuf.

```
#include <fstream>
#include <async_filebuf.h>
...
async_filebuf sb(1000000, 5, 1);
async_filebuf.open("my_input_data_file");
std::ifstream ifs;
ifs.std::ios::rdbuf(&sb);
```

The arguments to the async_filebuf constructor are as follows.

1. **chunk_size** - individual reads to the input source will be issued with this byte count, should be adapted to the characteristics of your source, 1000000 in the above example.

2. **chunks_ahead** - maximum number of chunks to be held in memory at a time, 5 in the above example.

3. **chunks_lookback** -- number of chunks behind the present read position that should be kept in memory before being recycled for filling by the read-ahead algorithm, 1 in the above example.

Constructing an async_filebuf with chunks_lookback=0 effectively disables buffering, providing a convenient means to compare the performance of your application with and without the read-ahead and buffering functionality of async_filebuf.

# example program
An example program t.cc is provided. The distributed code tries to open an input file over xrootd, so if you do not have the xrootd preload library installed in your shell environment you should edit t.cc and change the name of the input file before running the test as follows.

```
$ make
$ ./t
```

If the build was successful and the input file could be opened, you should see a sequence of output lines describing the sequence of reads from the input file, with interspersed seeks. 

The chunks_loopback parameter should be set to the maximum size of a relative movement forward or backward on the stream (e.g. using ifs.seekg) without interrupting the look-ahead streaming loop. Steps forward or backward larger than chunk_size\*chunks_lookback will trigger a flush of the read-ahead buffer, to be restarted on the next read request. Normally, unless you are jumping ahead in large steps you want the read loop to stream continuously while the reads jump around within the look-back window.

# expected improvement
In the case of the specific xrootd-sourced application described above, I measured a speed-up of a factor 10 when using async_filebuf in the place of std::filebuf. Unsurprisingly, the performance of this same application is unchanged between async_filebuf and std::filebuf when reading from the same file on a local disk. Basically what async_filebuf did was to allow the application to achieve the same data processing rate on an given input when its is live-streamed from a remote site as would be seen if it were read from a local file.

Might a similar improvement be seen for other data sources, e.g. http? The answer to this question depends largely on the answers to two related questions.

1. Is it **latency** that is currently preventing your application from getting higher cpu occupancy, or is it bandwidth or some other factor? Unless the answer is latency, the algorithm employed in async_filebuf is unlikely to improve things and may actually hurt your performance.

2. Is your input source handled through the **kernel's memory-mapped i/o cache**? The kernel employs its own internal cacheing scheme which may have some read-ahead capability. This comes into play whenever a stream is opened to a file that resides on a mounted filesystem. Remote nfs filesystems are included in this. The user has limited ability to control what the kernel is doing in this regard, but you may very well find that async_filebuf gives you very little speed-up over what the kernel already provides *if* your input file is on a mounted filesystem. The only way to know is to try it and see.
