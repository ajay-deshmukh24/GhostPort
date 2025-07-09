<h1>Multi Threaded Proxy Server with Cache</h1>

This project is implemented using `C` and Parsing of HTTP referred from <a href = "https://github.com/vaibhavnaagar/proxy-server"> Proxy Server </a>

## Index

- [Project Theory](https://github.com/ajay-deshmukh24/GhostPort#project-theory)
- [How to Run](https://github.com/ajay-deshmukh24/GhostPort#How-to-Run)
- [Demo](https://github.com/ajay-deshmukh24/GhostPort#Demo)
- [Contributing](https://github.com/ajay-deshmukh24/GhostPort#contributing)

## Project Theory

[[Back to top]](https://github.com/ajay-deshmukh24/GhostPort#index)

##### Introduction

##### Basic Working Flow of the Proxy Server:

![](https://github.com/ajay-deshmukh24/GhostPort/blob/main/pics/UML.JPG)

##### How did we implement Multi-threading?

- Used Semaphore instead of Condition Variables and pthread_join() and pthread_exit() function.
- pthread_join() requires us to pass the thread id of the the thread to wait for.
- Semaphore’s sem_wait() and sem_post() doesn’t need any parameter. So it is a better option.

##### Motivation/Need of Project

- To Understand →
  - The working of requests from our local computer to the server.
  - The handling of multiple client requests from various clients.
  - Locking procedure for concurrency.
  - The concept of cache and its different functions that might be used by browsers.
- Proxy Server do →
  - It speeds up the process and reduces the traffic on the server side.
  - It can be used to restrict user from accessing specific websites.
  - A good proxy will change the IP such that the server wouldn’t know about the client who sent the request.
  - Changes can be made in Proxy to encrypt the requests, to stop anyone accessing the request illegally from your client.

##### OS Component Used ​

- Threading
- Locks
- Semaphore
- Cache (LRU algorithm is used in it)

##### Limitations ​

- If a URL opens multiple clients itself, then our cache will store each client’s response as a separate element in the linked list. So, during retrieval from the cache, only a chunk of response will be send and the website will not open
- Fixed size of cache element, so big websites may not be stored in cache.

##### How this project can be extended? ​

- This code can be implemented using multiprocessing that can speed up the process with parallelism.
- We can decide which type of websites should be allowed by extending the code.
- We can implement requests like POST with this code.

# Note :-

- Code is well commented. For any doubt you can refer to the comments.
- Code is only for Linux systems.
- If you have windows system then you should install Linux enviornment and run using curl from terminal and if you want to run from browser use IP of your ubuntu enviornment in plcae of localhost in test url.
- Use Icognito window in both linux and windows system so that browser's cache will not interrupt.

## How to Run

```bash
$ git clone https://github.com/ajay-deshmukh24/GhostPort.git
$ cd GhostPort
$ make
$ ./proxy <port no.>
```

```markdown
- For Linux system open through browser
  `http://localhost:port/https://www.cs.princeton.edu/`

- For Windows system with linux envoirnment installed open  
  `http://<Ubuntu_IP>:port/https://www.cs.princeton.edu/`
```

## Demo

![](https://github.com/ajay-deshmukh24/GhostPort/blob/main/pics/Screenshot%202025-05-12%20214359.png)

![](https://github.com/ajay-deshmukh24/GhostPort/blob/main/pics/Screenshot%202025-05-12%20214320.png)

- When website is opened for the first time (`url not found`) then cache will be miss.
- Then if you again open that website again then `Data is retrieved from the cache` will be printed.

## Contributing

[[Back to top]](https://github.com/Lovepreet-Singh-LPSK/MultiThreadedProxyServerClient#index)

Feel free to add some useful. You can see `How this code can be extended`. Use ideas from there and feel free to fork and CHANGE.

#### Enjoy CODE and pull requests are highly appreciated.
