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
- The script print out whether every page have incoming links of not.

# Web Server


