for i in $(seq 1 10); do
    spawn-fcgi -s "/tmp/fcgi/sf1r$i.socket" -f "./sf1r_process.fcgi 180.153.140.110:2181,180.153.140.111:2181,180.153.140.112:2181  10.10.1.112 8765 nginx.log.172.16.6.8:18181 distributed beta >> /tmp/fcgi/sf1r$i.log 2>&1" -P ./sf1r_process.fcgi.pid -F 1
done
