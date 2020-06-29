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
    * text_file: is a text file from which random words will be chose to create the web pages. A files like this could be "20000 feet under the sea" (http://www.gutenberg.org/cache/epub/164/pg164.txt)
    * w: number of web sites to be created
    * p: number of pages per web site to be created
