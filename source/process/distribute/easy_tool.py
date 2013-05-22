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
import time
import ast
import subprocess
import shlex


replica_set_list = []
replica_set_list += [{ 'loginuser':'ops', 'primary': ['10.10.99.121'], 'replica': ['10.10.99.122', '10.10.99.123']}]
replica_set_list += [{ 'loginuser':'ops', 'primary': ['10.10.99.127'], 'replica': ['10.10.99.128', '10.10.99.129']}]
replica_set_list += [{ 'loginuser':'ops', 'primary': ['10.10.99.131'], 'replica': ['10.10.99.132', '10.10.99.133']}]
replica_set_list += [{ 'loginuser':'lscm', 'primary': ['172.16.5.195'], 'replica': ['172.16.5.192', '172.16.5.194']}]
replica_set_list += [{ 'loginuser':'lscm', 'primary': ['10.10.99.156'], \
        'replica': ['10.10.99.158', '10.10.99.140', '10.10.99.141', '10.10.99.142', \
                    '10.10.99.143', '10.10.99.119', '10.10.99.120', '10.10.99.114', \
                    '10.10.99.115', '10.10.99.116', '10.10.99.117', '10.10.99.118']}]
replica_set_list += [{ 'loginuser':'lscm', 'primary': ['10.10.99.157'], 'replica': ['10.10.99.159', '10.10.99.113']}]

all_primary_host = []
all_replicas_host = []
loginuser = 'ops'

for replica_set in replica_set_list:
    all_primary_host += replica_set['primary']
    all_replicas_host += replica_set['replica']
    print 'replica set hosts : ' 
    print replica_set

cur_replica_set = 0
cur_replica_set = int(raw_input( 'please set the replica set you want to use, -1 for all replica set: '))

if cur_replica_set == -1:
    primary_host = all_primary_host
    replicas_host = all_replicas_host
else:
    primary_host = replica_set_list[cur_replica_set]['primary']
    replicas_host = replica_set_list[cur_replica_set]['replica']
    loginuser = replica_set_list[cur_replica_set]['loginuser']

local_base = os.environ['HOME'] + '/codebase/'
tmp_base = raw_input('please set the base dir for local codebase.(default: ' + local_base + ') : ')
if len(tmp_base) > 0:
    local_base = tmp_base
print 'local code base is : ' + local_base

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

logfile = open('./result.log', 'a')

def printtofile(*objects):
    logfile.writelines(objects)
    logfile.writelines(['\n'])
    logfile.flush()

def run_prog_and_getoutput(args):
    proc = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    (out, err) = proc.communicate()
    return (out, err)

def send_cmd_afterssh(hosts, cmdstr, runstyle=0):
    for host in hosts:
        if runstyle == 1:
            print "sending command to host : " + host + ', cmd:' + cmdstr
            os.system(loginssh + host + ' \'' + cmdstr + ' \'')
        else:
            printtofile ("sending command to host : " + host + ', cmd:' + cmdstr)
            (out, err) = run_prog_and_getoutput(shlex.split(loginssh + host + ' \'' + cmdstr + ' \''))
            printtofile (out,err)

def send_cmd_andstay(hosts, cmdstr, runstyle=0):
    for host in hosts:
        if runstyle==1:
            print "sending command to host : " + host + ', cmd:' + cmdstr
            os.system(loginssh_stay + host + ' \'' + cmdstr + ' \'')
        else:
            printtofile ("sending command to host : " + host + ', cmd:' + cmdstr)
            (out, err) = run_prog_and_getoutput(shlex.split(loginssh_stay + host + ' \'' + cmdstr + ' \''))
            printtofile (out,err)

def syncfiles(args):
    if len(args) < 3:
        printtofile ('args not enough, need: files src_host dest_hosts')
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
            printtofile ('local to remote : ' + fullfilepath)
        else:
            fullfilepath = base_dir + '/' + file
        for dest_host in dest_hosts:
            dest_host_path = loginuser + '@' + dest_host + ':' + fullfilepath
            if src_host == '':
                # using local file
                os.system(scp_local + file + ' ' + dest_host_path)
            else:
                os.system(scp_remote + src_host + ':' + fullfilepath + ' ' + dest_host_path)
    printtofile ('finished.')

def start_all(args):
    if len(args) > 2:
        send_cmd_andstay(args[2:], 'cd ' + sf1r_bin_dir + ';' + start_prog)
    else:
        # start primary first
        send_cmd_andstay(primary_host,  'cd ' + sf1r_bin_dir + ';' + start_prog)
        time.sleep(10)
        send_cmd_afterssh(replicas_host,  'cd ' + sf1r_bin_dir + ';' + start_prog)
    printtofile ('start all finished.')

def stop_all(args):
    # stop replicas first.
    send_cmd_andstay(replicas_host,  'cd ' + sf1r_bin_dir + ';' + stop_prog)
    time.sleep(5)
    send_cmd_afterssh(primary_host,  'cd ' + sf1r_bin_dir + ';' + stop_prog)
    printtofile ('stop all finished.')

def update_src(args):
    if len(args) >= 3:
        target_projects = args[2:]
    else:
        target_projects = all_project
    cmdstr = ''
    for target_project in target_projects:
        cmdstr += ' cd ' + base_dir + '/' + target_project + ';' + update_src_prog + '; '
    print cmdstr
    send_cmd_afterssh(primary_host + replicas_host,  cmdstr, 1)
    print 'update source finished.'

def compile_all(args):
    if len(args) >= 3:
        target_projects = args[2:]
    else:
        target_projects = all_project
    cmdstr = ''
    i = 1
    for target_project in target_projects:
        if i == len(target_projects):
            cmdstr += ' cd ' + base_dir + '/' + target_project + '/build' + ';' + compile_prog + ' &'
        else:
            cmdstr += ' cd ' + base_dir + '/' + target_project + '/build' + ';' + compile_prog + '; '
        i += 1

    print cmdstr
    send_cmd_afterssh(primary_host + replicas_host,  cmdstr, 1)
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
    printtofile ('read cmdstr from file : ' + cmdfile)
    for line in fp:
        cmdstr += line.strip() + ';'
    fp.close()
    if len(args) <= 3:
        send_cmd_andstay(primary_host + replicas_host, cmdstr)
    else:
        send_cmd_andstay(args[3:], cmdstr)
    printtofile('finished')

def check_build_finish(args):
    if len(args) <=2:
        cmdstr = 'tail ' + sf1r_dir + '/build/easy_tool.log'
        send_cmd_afterssh(primary_host + replicas_host, cmdstr, 1)
    else:
        cmdstr = 'tail -f ' + sf1r_dir + '/build/easy_tool.log'
        host = args[2:]
        send_cmd_andstay(host, cmdstr, 1)
    print 'finished.'

def check_running(args):
    logfile_str = '`ls -tr | tail -n 1`'

    if len(args) <= 2:
        cmdstr = ' cd ' + sf1r_bin_dir + '/consolelog; echo ' + logfile_str + '; tail ./' + logfile_str
        send_cmd_afterssh(primary_host + replicas_host, cmdstr, 1)
    else:
        host = args[2:]
        cmdstr = ' cd ' + sf1r_bin_dir + '/consolelog; echo ' + logfile_str + '; tail -f ./' + logfile_str
        send_cmd_andstay(host, cmdstr, 1)
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
        failtype = '1'
        if host in fail_dict.keys():
            failtype = fail_dict[host]
        cmdstr = ' cd ' + sf1r_bin_dir + '; touch ./distribute_test.conf; echo ' + failtype + ' > ./distribute_test.conf'
        send_cmd_andstay([host], cmdstr)

def auto_restart(args):

    pid = os.fork()
    if pid > 0:
        sys.exit(0)

    while True:
        printtofile ('waiting next try')
        time.sleep(60)
        printtofile ('try starting sf1r')
        if len(args) < 3:
            start_all(args)
        else:
            send_cmd_andstay(args[2:],  'cd ' + sf1r_bin_dir + ';' + start_prog)

def simple_update(args):
    if len(args) < 4:
        print 'host and tar file should be given!'
        sys.exit(0)
    tarfile = args[2]
    hosts = args[3:]
    
    for host in hosts:
        print 'transferring file to dest host: ' + host
        (out, error) = run_prog_and_getoutput(["scp", tarfile, loginuser+'@'+host+':/opt'])
        print error
        print out

    cmdstr = 'cd ~/script; ./distribute_tools.sh simple_update /opt/' + tarfile
    send_cmd_afterssh(hosts, cmdstr)

def get_all_status(args):
    col = args[2]
    hosts = []
    if len(args) <= 3:
        print 'try get status from all hosts.'
        hosts = primary_host + replicas_host
    else:
        hosts = args[3:]

    for host in hosts:
        (out, error) = run_prog_and_getoutput([ruby_bin, driver_ruby_tool, host, '18181', driver_ruby_getstate, col])
        printtofile('info : ' + error)
        printtofile(out)

def mv_scd_to_index(args):
    cmdstr = ' cd ' + sf1r_bin_dir + '/collection/b5mp/scd/index/; mv backup/*.SCD .' 
    send_cmd_andstay(args[2:], cmdstr)

def reset_state_and_run():
    logfile.close()
    global logfile
    logfile = open('./result.log', 'a')
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
    time.sleep(10)
    (failed_host,down_host) = check_col(10)

    if len(down_host) > 0:
        time.sleep(10)

    # send index 
    for host in primary_host:
        (out, error) = run_prog_and_getoutput([ruby_bin, driver_ruby_tool, host, '18181', driver_ruby_index])
        printtofile (out)

    (failed_host,down_host) = check_col(20)
    if len(failed_host) > 0 or len(down_host) > 0:
        printtofile ('reset state wrong, data is not consistent.')
        exit(0)
    printtofile ('reset state for cluster finished.')

def check_col(check_interval = 10):
    retry = 0
    failed_host = []
    down_host = []
    while retry < 2:
        while True:
            time.sleep(check_interval)
            allready = True
            for host in primary_host + replicas_host:
                (out, error) = run_prog_and_getoutput([ruby_bin, driver_ruby_tool, host, '18181', driver_ruby_getstate])
                if error.find('Connection refused') != -1:
                    # this host is down.
                    printtofile ('host down : ' + host)
                    continue
                if out.find('\"NodeState\": \"3\"') == -1:
                    printtofile ('not ready, waiting')
                    printtofile (out)
                    allready = False
                    break

            if allready:
                break

        failed_host = []
        down_host = []
        for host in primary_host + replicas_host:
            (out, error) = run_prog_and_getoutput([ruby_bin, driver_ruby_tool, host, '18181', driver_ruby_check])
            if error.find('Connection refused') != -1:
                printtofile ('host down : ' + host)
                down_host += [host]
                continue
            if len(error) > 0:
                printtofile ('data is not consistent after running for host : ' + host)
                failed_host += [host]
                printtofile (error)
        if len(failed_host) > 0 or len(down_host) > 0:
            retry += 1
            printtofile ('check again')
            continue
        else:
            break
    return (failed_host, down_host)

def run_testwrite(testfail_host, testfail_type, test_writereq):
    cmdstr = ' cd ' + sf1r_bin_dir + '; touch ./distribute_test.conf; echo ' + str(testfail_type) + ' > ./distribute_test.conf'
    send_cmd_andstay(testfail_host, cmdstr)
    for host in primary_host:
        (out, error) = run_prog_and_getoutput([ruby_bin, driver_ruby_tool, host, '18181', test_writereq])
        if len(error) > 0:
            printtofile ('run write request error: ' + error + ', host: ' + host)
            return
    # check collection after this request.
    (failed_host,down_host) = check_col()
    if len(failed_host) > 0:
        printtofile ('data not consistent while checking after write.' + time.asctime())
    else:
        printtofile ('after write , test case passed')
    # restart any failed node.
    stop_all([])
    time.sleep(10)
    # failed host should be started last.
    first_start_host = primary_host + replicas_host
    for host in down_host:
        if (host not in testfail_host) and (len(testfail_host) > 0):
            printtofile ('a host down not by expected.' + host)
        first_start_host.remove(host)
    if len(first_start_host) > 0:
        start_all(['',''] + first_start_host)
    time.sleep(10)

    retry = 0;
    while retry < 2:
        if len(down_host) > 0:
            start_all(['',''] + down_host)
        time.sleep(10)
        # check collection again.
        (failed_host, down_host) = check_col()
        if len(failed_host) > 0:
            printtofile ('data not consistent while checking after restart failed node.' + time.asctime())
            sys.exit(0)
            break
        elif len(down_host) > 0 :
            # try again without fail test.
            retry += 1
            cmdstr = ' cd ' + sf1r_bin_dir + '; touch ./distribute_test.conf; echo 1 > ./distribute_test.conf'
            send_cmd_andstay(down_host, cmdstr)
            continue
        else:
            printtofile ('after restarting failed node, test case passed')
            break

    if len(down_host) > 0:
        printtofile ('check failed for down_host after retry.')
        sys.exit(0)

    cmdstr = ' cd ' + sf1r_bin_dir + '; touch ./distribute_test.conf; echo 1 > ./distribute_test.conf'
    send_cmd_andstay(primary_host + replicas_host, cmdstr)

def run_auto_fail_test(args):
    test_writereq_files = []
    special_fail_conf_files = []
    jsonfile = re.compile(r'\.json$', re.IGNORECASE)
    failconf_re = re.compile(r'\.failconf$', re.IGNORECASE)

    for root,dirs,files in os.walk(auto_testfile_dir):
        oldlen = len(test_writereq_files)
        test_writereq_files += filter(lambda x:jsonfile.search(x)<>None, files)
        for i in range(oldlen, len(test_writereq_files)):
            test_writereq_files[i] = root + '/' + test_writereq_files[i]
            print 'test write request file added ' + test_writereq_files[i]

        oldlen = len(special_fail_conf_files)
        special_fail_conf_files += filter(lambda x:failconf_re.search(x)<>None, files)
        for i in range(oldlen, len(special_fail_conf_files)):
            special_fail_conf_files[i] = root + '/' + special_fail_conf_files[i]
            print 'special fail conf added ' + special_fail_conf_files[i]

    pid = os.fork()
    if pid > 0:
        sys.exit(0)

    while True:
        printtofile ('waiting next fail test')
        time.sleep(10)

        reset_state_and_run()
        stop_all([])

        printtofile ('testing for updated config')
        cmdstr = ' cd ' + sf1r_bin_dir + '; touch ./distribute_update_conf.flag'
        send_cmd_andstay(primary_host, cmdstr)
        start_all(['', ''] + primary_host)
        time.sleep(30);
        start_all(['', ''] + replicas_host)
        (failed_host, down_host) = check_col()
        if len(failed_host) > 0 or len(down_host) > 0:
            printtofile ('check failed for updated config test')
            sys.exit(0)

        for test_writereq in test_writereq_files:
            printtofile ('running fail test for write request : ' + test_writereq)

            printtofile ('begin test for special fail conf')
            for special_fail in special_fail_conf_files:
                reset_state_and_run()
                printtofile ('testing for special fail : ' + special_fail)
                set_fail_test_conf(['', '', special_fail])
                run_testwrite([], 0, test_writereq)

            printtofile ('begin test for primary fail')
            for i in range(3, 15) + range(61, 63) + range(71, 73):
                reset_state_and_run()
                printtofile ('testing for primary fail type : ' + str(i))
                run_testwrite(primary_host, i, test_writereq)
                            
            printtofile ('begin test for first replica fail')
            for i in range(31, 42) + range(61, 63) + range(71, 73):
                reset_state_and_run()
                printtofile ('testing for first replica fail type : ' + str(i))
                run_testwrite([replicas_host[0]], i, test_writereq)

            printtofile ('begin test for last replica fail')
            for i in range(31, 42) + range(61, 63) + range(71, 73):
                reset_state_and_run()
                printtofile ('testing for last replica fail type : ' + str(i))
                run_testwrite([replicas_host[len(replicas_host) - 1]], i, test_writereq)

        # test for full write without clean state.
        reset_state_and_run()
        printtofile ('begin test for full write test without clean state')
        for test_writereq in test_writereq_files:
            run_testwrite([], 0, test_writereq)


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
        'simple_update':simple_update,
        'get_all_status':get_all_status,
        'run_auto_fail_test':run_auto_fail_test
        }

args = sys.argv
print args

if len(args) > 1:
    if args[1] in handler_dict.keys():
        print 'begin command: ' + args[1]
        try:
            handler_dict[args[1]](args)
        finally:
            print 'exited'
            logfile.close()
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
print 'send_cmd cmdstr host1 host2 # send cmdstr to given hosts'
print ''
print 'read_cmd_from_file cmdfile  # read cmd from cmdfile and send cmdstr. each cmd in a line.'
print ''
print 'set_fail_test_conf conf_file  # read fail test configure from conf_file and set this configure to all host.'
print ''
print 'auto_restart # period check sf1r and restart it if failed.'
print ''
print 'simple_update sf1r_tar_file host1 host2 # simple update the sf1r on the given host list using the given tar file.'
print ''
print 'get_all_status collection host1 host2 # get the node running status on the given host list from the given collection.'
print ''
print 'auto_restart host1 host2 # make sure the sf1r in host list is running, check in every 60s and try restart if not started.'
print ''

