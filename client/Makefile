all: mycrawler

mycrawler: mycrawler.o crawler_threads.o jobExecutor.o trie.o connections.o searchMode.o
	gcc mycrawler.c crawler_threads.c jobExecutor.c trie.c connections.c searchMode.c -o mycrawler -pthread

mycrawler.o: mycrawler.c
	gcc -c mycrawler.c

crawler_threads.o: crawler_threads.c
	gcc -c crawler_threads.c

jobExecutor.o: jobExecutor.c
	gcc -c jobExecutor.c

trie.o: trie.c
	gcc -c trie.c

connections.o: connections.c
	gcc -c connections.c

searchMode.o: searchMode.c
	gcc -c searchMode.c

clean:
	rm -rf *o mycrawler