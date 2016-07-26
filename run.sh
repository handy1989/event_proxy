#!/bin/bash
export LD_LIBRARY_PATH=/opt/local/libevent-2.1.5/lib:/opt/third_party/glog/lib:/opt/third_party/boost/lib:$LD_LIBRARY_PATH
LOG_DIR="log"
mkdir -p $LOG_DIR
export GLOG_log_dir=$LOG_DIR
export GLOG_alsologtostderr=1
export GLOG_logbufsecs=0
export GLOG_max_log_size=100
export GLOG_minloglevel=0
export GLOG_stop_logging_if_full_disk=1
./test
