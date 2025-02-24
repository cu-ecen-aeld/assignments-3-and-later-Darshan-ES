#!/bin/sh

# Startup script for aesdsocket daemon

DAEMON_NAME="aesdsocket"
DAEMON_LOC="/usr/bin/aesdsocket"
DAEMON_ARGS="-d"

start() {
    echo "Starting running $DAEMON_NAME..."
    
    if [ ! -x "$DAEMON_LOC" ]; then
        echo "Error: $DAEMON_LOC not found ."
        exit 1
    fi
   
    start-stop-daemon --start --quiet --background --exec "$DAEMON_LOC" -- $DAEMON_ARGS
    if [ $? -eq 0 ]; then
        echo "$DAEMON_NAME started successfully."
    else
        echo "Error: Failed to start $DAEMON_NAME."
        exit 1
    fi
}

stop() {
    echo "Stopping  $DAEMON_NAME..."
    start-stop-daemon --stop --quiet --name "$DAEMON_NAME"
}

case "$1" in
    start)
        start
        ;;
    stop)
        stop
        ;;
    *)
        echo "Usage: $0 {start|stop}"
        exit 1
        ;;
esac

exit 0

