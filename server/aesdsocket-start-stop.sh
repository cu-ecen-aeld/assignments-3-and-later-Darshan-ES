#!/bin/sh

SCRIPT_DIR="$(dirname "$(realpath "$0")")"
DAEMON="$SCRIPT_DIR/aesdsocket"
DAEMON_ARG="-d"

find_process() {
    ps aux | grep "$DAEMON" | grep -v grep | awk '{print $2}'
}

case "$1" in
    start)
        echo "Checking for existing aesdsocket process..."
        PID=$(find_process)

        if [ -n "$PID" ]; then
            echo "aesdsocket is already running with PID(s): $PID"
            exit 1
        fi

        echo "Starting aesdsocket..."
        $DAEMON $DAEMON_ARG &

        sleep 1  # Wait to allow process to start

        PID=$(find_process)
        if [ -n "$PID" ]; then
            echo "aesdsocket started with PID(s): $PID"
        else
            echo "Failed to start aesdsocket."
            exit 1
        fi
        ;;

    stop)
        echo "Checking for running aesdsocket process..."
        PID=$(find_process)

        if [ -z "$PID" ]; then
            echo "aesdsocket is not running."
            exit 1
        fi

        echo "Stopping aesdsocket with PID(s): $PID"
        kill -TERM $PID
        sleep 1

        # Ensure process is stopped
        PID=$(find_process)
        if [ -n "$PID" ]; then
            echo "Process did not terminate, kill..."
            kill -KILL $PID
            sleep 1
        fi

        PID=$(find_process)
        if [ -n "$PID" ]; then
            echo "ERROR_LOG: Process is running. Manually intervene."
        else
            echo "aesdsocket stopped ."
        fi
        ;;

    *)
        echo "Usage: $0 {start|stop}"
        exit 1
        ;;
esac

exit 0

