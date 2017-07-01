
Here is my additional remarks
-------

Install
-------
```sh
git clone https://github.com/liangguohuan/tiny-web-server.git
make install
```
Usage
-------
tinyserver [dir] [port] ...

New Feature
-------
- auto add slash if request is dir
- auto index file index.html 
- use dataTables to show files friendly if request is dir
- more commands in Makefile
  - `check`  
    check compile error
  - `install`  
    command `tinyserver` will be installed in `/usr/local/bin`
  - `uninstall`  
    remove command `tinyserver` and config files
- cache dir list if exec overtime 3 seconds
- open file with xdg-open and send status 403 if mime type not support

Change Logs
-------
* 2017-06-30 the standard codes for clang  
* 2017-06-29 limit for parsing request header when loop rio_readinitb()  
* 2017-06-29 fixed strange thing return empty call rio_readlineb()
* 2017-06-28 perfect tiny.c and add new feature cache dir list  
* 2017-06-28 replace c99 into c11 and more commands in Makefile  
* 2017-06-27 use dataTables to show nicely in handle_directory_request  
* 2017-06-27 video mime type not support call system program  
* 2017-06-25 Disabled warning: implicit-function-declaration  
* 2017-06-25 fixed mp4 stream range request  
* 2017-06-25 auto add slash if request is dir  
* 2017-06-25 add auto index file feture  

-------
-------
-------

A tiny web server in C
======================

I am reading
[Computer Systems: A Programmer's Perspective](http://csapp.cs.cmu.edu/).
It teachers me how to write a tiny web server in C.

I have written another
[tiny web server](https://github.com/shenfeng/nio-httpserver) in JAVA.

And another one [http-kit](https://github.com/http-kit/http-kit), http-kit is full featured, with websocket and async support

And few others on my github page.

Features
--------

1. Basic MIME mapping
2. Very basic directory listing
3. Low resource usage
4. [sendfile(2)](http://kernel.org/doc/man-pages/online/pages/man2/sendfile.2.html)
5. Support Accept-Ranges: bytes (for in browser MP4 playing)
6. Concurrency by pre-fork

Non-features
------------

1. No security check

Usage
-----

`tiny <port>`, opens a server in the current directory, port
default to 9999, just like `python -m SimpleHTTPServer`

I use it as a lightweight File Browser.


TODO
----

1. Write a epoll version


License
-------

The code is free to use under the terms of the MIT license.

