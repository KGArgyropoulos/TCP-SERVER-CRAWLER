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
    * threadPool: the most interesting developing choice is the use o a threadPool (httpd_threads.c). At first, threads notify the main functions that they are ready so that main can continue. Then they block untill main notifies them there's been a new request they need to proceed. Each thread takes a fileDescriptor from the queue, reads the request, answers it and writes it to the appropriate fd. Then it notifies main that the job is done and waits in the pool until it needs to be reused.

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

