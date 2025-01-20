#!/bin/bash


#This '$#' was based on content at [ https://www.gnu.org/software/bash/manual/bash.html#Positional-Parameters ] with modifications #[ '$#' Gives values of the Positional arguments in decimal ].

if [ $# -ne 2 ]; then
  echo "Error Message: <directory path> and <String> Required"
  exit 1
fi

WriteFile=$1
Writestr=$2


#This Code was based on content at [ https://www.geeksforgeeks.org/dirname-command-in-linux-with-examples/ ].

filedir=$(dirname "$WriteFile")

mkdir -p "$filedir"

echo "$Writestr" > "$WriteFile"

if [ $? -ne 0 ]; then
   echo "Error Message: Failed to Write to '$WriteFile' " 
   exit 1
fi

exit 0
