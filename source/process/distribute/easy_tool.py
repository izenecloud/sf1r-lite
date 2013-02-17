#! /usr/bin/python
# -*- coding: utf8 -*-

# tool : 
# sync configure file between host, 
# batch start/stop sf1r process
# batch compile/update source code.
# send test command to sf1r.

import os,sys
from os import path
import re
import string,re
import time
import ast
import subprocess


local_base = '/home/vincentlee/workspace/sf1/'

loginuser = 'lscm'
base_dir = '/home/' + loginuser + '/codebase'
sf1r_dir = base_dir + '/sf1r-engine'
izenelib_dir = base_dir + '/izenelib'
sf1r_bin_dir = sf1r_dir + '/bin'
start_prog = './sf1-revolution.sh start'
stop_prog = './sf1-revolution.sh stop'
update_src_prog = 'git pull '
driver_ruby_dir = local_base + 'driver-ruby/'
driver_ruby_tool = driver_ruby_dir + 'bin/distribute_tools.rb'
driver_ruby_index = driver_ruby_dir + 'bin/distribute_test/index_col.json'
driver_ruby_check = driver_ruby_dir + 'bin/distribute_test/check_col.json'
driver_ruby_getstate = driver_ruby_dir + 'bin/distribute_test/get_status.json'
ruby_bin = 'ruby1.9.1'
auto_testfile_dir = './auto_test_file/'

setting_path = ' export EXTRA_CMAKE_MODULES_DIRS=~/codebase/cmake/;'
compile_prog = setting_path + ' make -j4 > easy_tool.log 2>&1 '
all_project = ['izenelib', 'icma', 'ijma', 'ilplib', 'imllib', 'idmlib', 'sf1r-engine']

loginssh = 'ssh -n -f ' + loginuser + '@'
loginssh_stay = 'ssh ' + loginuser + '@'

scp_local = 'rsync -av '
scp_remote = 'scp -r ' + loginuser + '@'

primary_host = ['172.16.5.195']
replicas_host = ['172.16.5.192', '172.16.5.194']

def send_cmd_afterssh(hosts, cmdstr):
    for host in hosts:
        os.system(loginssh + host + ' \'' + cmdstr + ' \'')

def send_cmd_andstay(hosts, cmdstr):
    for host in hosts:
        print "sending command to host : " + host + ', cmd:' + cmdstr
        os.system(loginssh_stay + host + ' \'' + cmdstr + ' \'')

def syncfiles(args):
    if len(args) < 3:
        print 'args not enough, need: files src_host dest_hosts'
        return
    dest_hosts = []
    src_host = ''
    files = ast.literal_eval(args[2])
    if len(args) >= 4:
        src_host = args[3]
    if len(args) >= 5:
        dest_hosts = ast.literal_eval(args[4])
    if len(dest_hosts) == 0:
        if len(src_host) == 0:
            dest_hosts = primary_host + replicas_host
        else:
            dest_hosts = replicas_host

    for file in files:
        fullfilepath = ''
        if (src_host == ''):
            fullfilepath = base_dir + '/' + path.abspath(file).replace(path.abspath(local_base), '')
            print 'local to remote : ' + fullfilepath
        else:
            fullfilepath = base_dir + '/' + file
        for dest_host in dest_hosts:
            dest_host_path = loginuser + '@' + dest_host + ':' + fullfilepath
            if src_host == '':
                # using local file
                os.system(scp_local + file + ' ' + dest_host_path)
            else:
                os.system(scp_remote + src_host + ':' + fullfilepath + ' ' + dest_host_path)
    print 'finished.'

def start_all(args):
    # start primary first
    send_cmd_andstay(primary_host,  'cd ' + sf1r_bin_dir + ';' + start_prog)
    time.sleep(10)
    send_cmd_afterssh(replicas_host,  'cd ' + sf1r_bin_dir + ';' + start_prog)
    print 'start all finished.'

def stop_all(args):
    # stop replicas first.
    send_cmd_andstay(replicas_host,  'cd ' + sf1r_bin_dir + ';' + stop_prog)
    time.sleep(5)
    send_cmd_afterssh(primary_host,  'cd ' + sf1r_bin_dir + ';' + stop_prog)
    print 'stop all finished.'

def update_src(args):
    if len(args) >= 3:
        target_projects = args[2:]
    else:
        target_projects = all_project
    cmdstr = ''
    for target_project in target_projects:
        cmdstr += ' cd ' + base_dir + '/' + target_project + ';' + update_src_prog + '; '
    print cmdstr
    send_cmd_afterssh(primary_host + replicas_host,  cmdstr)
    print 'update source finished.'

def compile_all(args):
    if len(args) >= 3:
        target_projects = args[2:]
    else:
        target_projects = all_project
    cmdstr = ''
    for target_project in target_projects:
        cmdstr += ' cd ' + base_dir + '/' + target_project + '/build' + ';' + compile_prog + '; '
    print cmdstr
    send_cmd_afterssh(primary_host + replicas_host,  cmdstr)
    print 'compile command send finished.'

def send_cmd(args):
    cmdstr = args[2]
    if len(args) <= 3:
        send_cmd_andstay(primary_host + replicas_host, cmdstr)
    else:
        send_cmd_andstay(args[3:], cmdstr)
    print 'finished.'

def read_cmd_from_file(args):
    cmdfile = args[2]
    cmdstr = ''
    fp = open(cmdfile, 'r')
    print 'read cmdstr from file : ' + cmdfile
    for line in fp:
        cmdstr += line.strip() + ';'
    fp.close()
    if len(args) <= 3:
        send_cmd_andstay(primary_host + replicas_host, cmdstr)
    else:
        send_cmd_andstay(args[3:], cmdstr)
    print 'finished'

def check_build_finish(args):
    cmdstr = 'tail -f ' + sf1r_dir + '/build/easy_tool.log'
    if len(args) <=2:
        send_cmd_afterssh(primary_host + replicas_host, cmdstr)
    else:
        host = args[2:]
        send_cmd_andstay(host, cmdstr)
    print 'finished.'

def check_running(args):
    logfile = '`ls -tr | tail -n 1`'
    cmdstr = ' cd ' + sf1r_bin_dir + '/consolelog; echo ' + logfile + '; tail -f ./' + logfile

    if len(args) <= 2:
        send_cmd_afterssh(primary_host + replicas_host, cmdstr)
    else:
        host = args[2:]
        send_cmd_andstay(host, cmdstr)
    print 'finished.'

def set_fail_test_conf(args):
    conf_file = args[2]
    fp = open(conf_file, 'r')
    fail_dict = {}
    for line in fp:
        line = line.strip()
        if line[0] == '#' or len(line) < 1 :
            continue
        [ip, failtype] = line.split(' ')
        fail_dict[ip] = failtype
    fp.close()
    for host in primary_host + replicas_host:
        failtype = '0'
        if host in fail_dict.keys():
            failtype = fail_dict[host]
        cmdstr = ' cd ' + sf1r_bin_dir + '; touch ./distribute_test.conf; echo ' + failtype + ' > ./distribute_test.conf'
        send_cmd_andstay([host], cmdstr)

def auto_restart(args):
    while True:
        print 'waiting next try'
        time.sleep(60)
        print 'try starting sf1r'
        if len(args) < 3:
            start_all(args)
        else:
            send_cmd_andstay(args[2:],  'cd ' + sf1r_bin_dir + ';' + start_prog)

def mv_scd_to_index(args):
    cmdstr = ' cd ' + sf1r_bin_dir + '/collection/b5mp/scd/index/; mv backup/*.SCD .' 
    send_cmd_andstay(args[2:], cmdstr)

def run_prog_and_getoutput(args):
    proc = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    (out, err) = proc.communicate()
    return (out, err)

def reset_state_and_run():
    stop_all([])
    time.sleep(10)
    # clean data. move scd file to index on primary host.
    read_cmd_from_file(['','','./cleandata_cmd'])
    mv_scd_to_index(['', ''] + primary_host)

    allhost = primary_host + replicas_host
    # make sure at least one node is configure as NoFail or NoAnyTest.
    cmdstr = ' cd ' + sf1r_bin_dir + '; touch ./distribute_test.conf; echo 1 > ./distribute_test.conf'
    send_cmd_andstay(allhost, cmdstr)
    start_all([])
    time.sleep(30)
    # send index 
    for host in primary_host:
        (out, error) = run_prog_and_getoutput([ruby_bin, driver_ruby_tool, host, '18181', driver_ruby_index])
        print out

    while True:
        time.sleep(60)
        allready = True
        for host in primary_host:
            (out, error) = run_prog_and_getoutput([ruby_bin, driver_ruby_tool, host, '18181', driver_ruby_getstate])
            if out.find('\"NodeState\": \"3\"') == -1:
                print 'not ready, waiting'
                print out
                allready = False
                break
        if allready:
            break

    for host in primary_host:
        (out, error) = run_prog_and_getoutput([ruby_bin, driver_ruby_tool, host, '18181', driver_ruby_check])
        if len(error) > 0:
            print 'reset state wrong, data is not consistent.'
            exit(0)
    print 'reset state for cluster finished.'

def check_col():
    while True:
        time.sleep(10)
        allready = True
        for host in primary_host + replicas_host:
            (out, error) = run_prog_and_getoutput([ruby_bin, driver_ruby_tool, host, '18181', driver_ruby_getstate])
            if error.find('Connection refused') != -1:
                # this host is down.
                print 'host down : ' + host
                continue
            if out.find('\"NodeState\": \"3\"') == -1:
                print 'not ready, waiting'
                print out
                allready = False
                break

        if allready:
            break

    failed_host = []
    for host in primary_host + replicas_host:
        (out, error) = run_prog_and_getoutput([ruby_bin, driver_ruby_tool, host, '18181', driver_ruby_check])
        if error.find('Connection refused') != -1:
            print 'host down : ' + host
            continue
        if len(error) > 0:
            print 'data is not consistent after running for host : ' + host
            failed_host += host
    return failed_host

def run_testwrite(testfail_host, testfail_type, test_writereq):
    cmdstr = ' cd ' + sf1r_bin_dir + '; touch ./distribute_test.conf; echo ' + str(testfail_type) + ' > ./distribute_test.conf'
    send_cmd_andstay(testfail_host, cmdstr)
    for host in primary_host:
        (out, error) = run_prog_and_getoutput([ruby_bin, driver_ruby_tool, host, '18181', test_writereq])
        if len(error) > 0:
            print 'run write request error: ' + error + ', host: ' + host
    # check collection after this request.
    failed_host = check_col()
    if len(failed_host) > 0:
        print 'data not consistent while checking after write.'
    else:
        print 'after write , test case passed'
    # restart any failed node.
    start_all([])
    time.sleep(60)
    # check collection again.
    failed_host = check_col()
    if len(failed_host) > 0:
        print 'data not consistent while checking after restart failed node.'
    else:
        print 'after restarting failed node, test case passed'


def run_auto_fail_test(args):
    test_writereq_files = []
    jsonfile = re.compile(r'\.json$', re.IGNORECASE)
    for root,dirs,files in os.walk(auto_testfile_dir):
        oldlen = len(test_writereq_files)
        test_writereq_files += filter(lambda x:jsonfile.search(x)<>None, files)
        for i in range(oldlen, len(test_writereq_files)):
            test_writereq_files[i] = root + '/' + test_writereq_files[i]

    while True:
        print 'waiting next fail test'
        time.sleep(10)
        for test_writereq in test_writereq_files:
            print 'running fail test for write request : ' + test_writereq

            # test for all kinds of primary fail
            print 'begin test for primary fail'
            for i in range(2, 12):
                reset_state_and_run()
                print 'testing for primary fail type : ' + str(i)
                run_testwrite(primary_host, i, test_writereq)
                            
            print 'begin test for replica fail'
            for i in range(31, 41):
                reset_state_and_run()
                print 'testing for replica fail type : ' + str(i)
                run_testwrite([replicas_host[0]], i, test_writereq)

            print 'begin test for other fail'
            for i in range(61, 62):
                reset_state_and_run()
                print 'testing for other fail type on replica: ' + str(i)
                run_testwrite([replicas_host[0]], i, test_writereq)
                reset_state_and_run()
                print 'testing for other fail type on primary_host: ' + str(i)
                run_testwrite(primary_host, i, test_writereq)

            # test for primary electing fail.
            reset_state_and_run()
            print 'testing for primary electing fail'
            # fail primary.
            cmdstr = ' cd ' + sf1r_bin_dir + '; touch ./distribute_test.conf; echo 3 > ./distribute_test.conf'
            send_cmd_andstay(primary_host, cmdstr)
            # fail the first backup primary to make the first electing fail.
            run_testwrite([replicas_host[0]], 2, test_writereq)

handler_dict = { 'syncfile':syncfiles,
        'start_all':start_all,
        'stop_all':stop_all,
        'update_src':update_src,
        'compile_all':compile_all,
        'check_build':check_build_finish,
        'check_running':check_running,
        'send_cmd':send_cmd,
        'read_cmd_from_file':read_cmd_from_file,
        'set_fail_test_conf':set_fail_test_conf,
        'auto_restart':auto_restart,
        'run_auto_fail_test':run_auto_fail_test
        }

args = sys.argv
print args

if len(args) > 1:
    if args[1] in handler_dict.keys():
        print 'begin command: ' + args[1]
        handler_dict[args[1]](args)
        exit(0)

print 'Usage:'
print ''
print 'start_all '
print ''
print 'stop_all '
print ''
print 'update_src project1 project2. ## if no project specific, all projects will update'
print ''
print 'compile_all project1 project2.  ## if no project specific, all projects will compile.'
print ''
print 'syncfile [\"file1\", \"file2\"] src_host [\"ip1\", \"ip2\"]. ## if no src_host, sync file from local to all remote host.\
        if src_host given, and dest hosts missing, it will sync files from primary_host to all replicas'
print ''
print 'check_build  # check if all build finished by tail the build log on all host.'
print ''
print 'check_running host logfile_name  # check the host running status by tail the running log on host. \
        if no logfile_name, the newest log will be checked.'
print ''
print 'send_cmd cmdstr  # send cmdstr to all hosts'
print ''
print 'read_cmd_from_file cmdfile  # read cmd from cmdfile and send cmdstr. each cmd in a line.'
print ''
print 'set_fail_test_conf conf_file  # read fail test configure from conf_file and set this configure to all host.'
print ''
print 'auto_restart host1 host2 # make sure the sf1r in host list is running, check in every 60s and try restart if not started.'
print ''

