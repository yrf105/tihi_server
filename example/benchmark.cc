#include "http/http_server.h"
#include "log/log.h"

static tihi::Logger::ptr g_logger = TIHI_LOG_ROOT();

void run() {
    g_logger->set_level(tihi::LogLevel::INFO);
    tihi::Address::ptr addr = tihi::Address::LookUpIPAddress("0.0.0.0:12345");
    if (!addr) {
        TIHI_LOG_INFO(g_logger) << "addr error";
        return ;
    }

    tihi::http::HttpServer::ptr server(new tihi::http::HttpServer);
    if (!server) {
        TIHI_LOG_INFO(g_logger) << "server error";
        return ;
    }

    while (!(server->bind(addr))) {
        TIHI_LOG_INFO(g_logger) << "retry bind";
        sleep(1);
    }

    server->start();
}

int main(int argc, char** argv) {
    tihi::IOManager iom;
    iom.schedule(&run);
    return 0;
}

/*
ab -n 1000000 -c 200 "http://192.168.43.231:12345/"
This is ApacheBench, Version 2.3 <$Revision: 1430300 $>
Copyright 1996 Adam Twiss, Zeus Technology Ltd, http://www.zeustech.net/
Licensed to The Apache Software Foundation, http://www.apache.org/

Benchmarking 192.168.43.231 (be patient)
Completed 100000 requests
Completed 200000 requests
Completed 300000 requests
Completed 400000 requests
Completed 500000 requests
Completed 600000 requests
Completed 700000 requests
Completed 800000 requests
Completed 900000 requests
Completed 1000000 requests
Finished 1000000 requests


Server Software:        
Server Hostname:        192.168.43.231
Server Port:            12345

Document Path:          /
Document Length:        128 bytes

Concurrency Level:      200
Time taken for tests:   292.734 seconds
Complete requests:      1000000
Failed requests:        0
Write errors:           0
Non-2xx responses:      1000000
Total transferred:      216000000 bytes
HTML transferred:       128000000 bytes
Requests per second:    3416.07 [#/sec] (mean)
Time per request:       58.547 [ms] (mean)
Time per request:       0.293 [ms] (mean, across all concurrent requests)
Transfer rate:          720.58 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0   25   3.3     25      82
Processing:    12   33   5.7     32      97
Waiting:        1   25   5.9     23      87
Total:         32   58   5.4     58     124

Percentage of the requests served within a certain time (ms)
  50%     58
  66%     60
  75%     61
  80%     62
  90%     65
  95%     68
  98%     73
  99%     77
 100%    124 (longest request)
*/

/*
ab -n 1000000 -c 200 "http://192.168.43.231/ss"    
This is ApacheBench, Version 2.3 <$Revision: 1430300 $>
Copyright 1996 Adam Twiss, Zeus Technology Ltd, http://www.zeustech.net/
Licensed to The Apache Software Foundation, http://www.apache.org/

Benchmarking 192.168.43.231 (be patient)
Completed 100000 requests
Completed 200000 requests
Completed 300000 requests
Completed 400000 requests
Completed 500000 requests
Completed 600000 requests
Completed 700000 requests
Completed 800000 requests
Completed 900000 requests
Completed 1000000 requests
Finished 1000000 requests


Server Software:        nginx/1.19.9
Server Hostname:        192.168.43.231
Server Port:            80

Document Path:          /ss
Document Length:        153 bytes

Concurrency Level:      200
Time taken for tests:   285.247 seconds
Complete requests:      1000000
Failed requests:        0
Write errors:           0
Non-2xx responses:      1000000
Total transferred:      303000000 bytes
HTML transferred:       153000000 bytes
Requests per second:    3505.73 [#/sec] (mean)
Time per request:       57.049 [ms] (mean)
Time per request:       0.285 [ms] (mean, across all concurrent requests)
Transfer rate:          1037.34 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0   27   2.9     27      60
Processing:    11   30   4.0     29      72
Waiting:        0   22   3.7     22      63
Total:         24   57   3.6     56     104

Percentage of the requests served within a certain time (ms)
  50%     56
  66%     57
  75%     58
  80%     59
  90%     61
  95%     64
  98%     67
  99%     71
 100%    104 (longest request)
*/