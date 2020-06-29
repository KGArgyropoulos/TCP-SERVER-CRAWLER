all: myhttpd

myhttpd: myhttpd.o httpd_threads.o
	gcc myhttpd.c httpd_threads.c -o myhttpd -pthread

myhttpd.o: myhttpd.c
	gcc -c myhttpd.c

httpd_threads.o: httpd_threads.c
	gcc -c httpd_threads.c

clean:
	rm -rf *o myhttpd
