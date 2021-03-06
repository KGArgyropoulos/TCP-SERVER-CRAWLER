# TCP-SERVER-CRAWLER

There are three cooperating applications implemented for the purposes of this project.
- Web Site Creator
    * Shell script creating an amount of web sites at the disk.
- TCP Web Server
    * Receiving HTTP requests for specific web pages.
- TCP Web Crawler
    * Downloads all web sites from the Web Server and executes search queries.

# Web site creator - webcreator.sh

- Execution command: ./webcreator.sh root_directory text_file w p
    * root_directory: is a directory which SHOULD BE CREATED in the same directory as webcreator.sh and inside of which, the web sites will be created.
    * text_file: is a text file from which random words will be chose to create the web pages. A files like this could be "20000 feet under the sea" (http://www.gutenberg.org/cache/epub/164/pg164.txt) !!Note: text_file should have at least 10000 lines
    * w: number of web sites to be created
    * p: number of pages per web site to be created

- The content of each web page will be created randomly, having text_file as input:
    * We choose a random k:[1,#linesInText_file-2000]
    * We choose a random m:[1000,2000]
    * We create a set of size f = (p / 2) + 1 with f names of random web pages (except from the same one) of the same site. (Internal links)
    * We create a set of size q = (w / 2) + 1 with q names of random web pages (except from the same one) of different sites. (External links)
    * We add HTML starting headers
    * Starting from line k of text_file, we copy m / (f + q) lines to the file of the web page we created and write one of the f+q links to HTML.
    * Repeat the previous step for the rest m / (f+q) lines and the next link, until every line and link is written.
    * We add HTML ending headers.
- The script purges the root_directory if it's not empty.
- The script print out whether every page have incoming links or not.

# Web Server

- Compile (inside server folder) : make
- Clear object files: make clean
- Execution command: ./myhttpd -p serving_port -c command_port -t num_of_threads -d root_dir
    * serving_port : is the listening port, at which the server returns web pages
    * command_port : is the listening port, at which the server waits for commands
    * num_of_threads : number of threads the server will create in order to manage the incoming requests. These threads are all created at the beginning and are located at a threadpool, where the server picks and uses them. When a thread terminates, server starts a new one.
    * root_dir : is the directory containing the web sites created from webcreator.sh.
- Serving Port requests: GET /webSite/webPage.html HTTP/1.1
    * The request is assigned to some thread form the thread pool. Then the thread returns file root_dir/webSite/webPage.html back to the user who requested it. 
    * These HTTP requests can be sent via **Telnet**
    * Or by using **Firefox Browser** and typing: **localhost:server_port/root_directory/page_name**
- Command Port commands: User commands executed directly by the server (not the threads)
    * STATS: server's running time, served pages and bytes read.
    * SHUTDOWN: free allocated memory and stop the server
    * Command requests can be sent via **Telnet**, by typing: **telnet localhost command_port**
    * Connection closes after sending a request.
- Implementation:
    * What we described above, meaning the communication between server and client, is achieved over sockets. There are 2 socket connections, working at the command and the serving port.
    * threadPool: the most interesting developing choice is the use o a threadPool (httpd_threads.c). At first, threads notify the main functions that they are ready so that main can continue. Then they block untill main notifies them there's been a new request they need to proceed. Each thread takes a fileDescriptor from the queue, reads the request, answers it and writes it to the appropriate fd. Then it notifies main that the job is done and waits in the pool until it needs to be re-used.

# Web Crawler

- Compile (inside server folder) : make
- Clear object files: make clean
- Execution command: ./mycrawler -h host_or_IP -p port -c command_port -t num_of_threads -d save_dir starting_URL
    * host_or_ip : is the name of the machine or server's IP
    * port : server's request listening port
    * command_port : server's command listening port
    * num_of_threads : number of threads running at the crawler. In case some threads terminates, the crawler has to create a new one.
    * save_dir : is the directory in which the crawler stores the pages it downloads. After the end of the crawler's execution (supposed that every page is accessible with an internal or external link) save_dir should be exact copy of the root_dir.
    * starting_URL : is the url, the crawler starts with.
- Crawler's Functionality:
    * Starts by creating a threadPool with num_of_threads threads. Threads are re-usable. Then creates a queue and stores the up-to-now found links, adding the starting_URL in the queue.
    * Some thread takes the URL from the queue and requests it from the server. The URL is in the form: http://host_or_ip:port/siteX/pageX_Y.html .
    * Downloads the file and saves it in save_dir.
    * Analyzes the downloaded file, finds more links and adds them to the queue as well.
    * Repeats step 2 with all threads working in parallel until there are no more links in the queue.
    * At the command port, crawler receives the commands that processes itself, without assigning them to threads. Such commands are:
        * STATS: works the same way as at the server but for the crawler.
        * SEARCH w1,w2,...,wn: If the crawling is done (otherwise there's an appropriate message printed) the crawler calls jobExecutor [github link for more on this project: (https://github.com/KGArgyropoulos/Job-Executor) ].
        * SHUTDOWN: works the same way as at the server but for the crawler.
- Requests and commands can either be sent via **Telnet** or by **Firefox Browser**, similarly to the Web Server case.
- Implementation:
    * mycrawler.c: Crawler's driver. Some interesting developing choices are explained below.
        * TaskQueue, used by threads, takes the files' urls as keys to explore.
        * search command triggers addLinksToFile function, which has a connected list with the paths to the crawled files and writes them to paths.txt file, which is given as argument to jobExecutor. It also passes the socket's fd as well as the search queries. jobExecutor now works with 5 workers by default and there's a new function,called findEachQuery, which returns the number of search queries. Also, there's no deadline and no statistics written to logFiles.
    * crawler_threads.c: Threads' implementation, using struct threadTask. Some interesting developing choices are explained below.
        * At first they notice main function they're ready, so that they can start their execution. When external variable called blocked is equal to the number of threads, that means that every thread is blocked and the process is complete. An external variable, called unblocked makes the threads wait. This variable gets equal to 1, when a thread has completed its job and therefore has addes new elements ready to be processed to the queue. Another external variable called word_done is increased whenever a thread exits, so that we know if crawling is done. (threadTask function)
        * Crawler-client creates a socket connection with the server, to whom it sends a request for some url from the queue and reads the responce through function appendFile. This opens or creates the appropriate directory-site in save_dir and writes the server's answer. Function analyzeIt is then called,which opens the file, reads its contents and sends the links to the TaskQueue. threadTask checks if there are duplicate analyzed links and decides whether to insert them or continue with the next element of the list. (contentProcess)

