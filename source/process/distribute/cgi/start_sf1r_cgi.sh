spawn-fcgi -s "/tmp/fcgi/sf1r1.socket" -f "./sf1r_process.fcgi 180.153.140.110:2181,180.153.140.111:2181,180.153.140.112:2181  distributed >> sf1r1.log 2>&1" -P ./sf1r_process.fcgi.pid -F 1
