# tfs_ns_server = 127.0.0.1:8108

spawn-fcgi -a 127.0.0.1 -p 9000 -f "./image_process.fcgi 127.0.0.1:8108" -P ./image_process.fcgi.pid -F 10
