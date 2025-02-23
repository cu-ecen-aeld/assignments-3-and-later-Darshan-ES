#!/bin/sh

DAEMON=/usr/bin/aesdsocket   
DAEMON_ARG="-d"
FILE_PID=/var/run/aesdsocket.pid

case "$1" in
    start)
        echo "aesdsocket Starting."
        
        start-stop-daemon --start --background --quiet --make-pidfile --pidfile $FILE_PID \
            --exec $DAEMON -- $DAEMON_ARG
            
        if [ -f "$FILE_PID" ]; then
        echo "Aesd Socket Started with PID $(cat $FILE_PID)"
	else
        echo "Failed to start Aesdsocket."
	fi
        ;;
    
    stop)
        echo "aesdsocket Stopping."
        
        if [ -f "$FILE_PID" ]; then
            start-stop-daemon --stop --quiet --pidfile $FILE_PID --signal SIGTERM
            rm -f $FILE_PID
            echo "aesdsocket stopped."
        else
            echo "aesdsocket is not running."
        fi
        ;;
    
    *)
        echo "arguments: $0 {start|stop}"
        exit 1
        ;;
esac

exit 0

