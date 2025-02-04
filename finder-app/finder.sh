#!/bin/sh

#This '$#' was based on content at [ https://www.gnu.org/software/bash/manual/bash.html#Positional-Parameters ] with modifications #[ '$#' Gives values of the Positional arguments in decimal ].
if [ $# -ne 2 ]; then
  echo "Error Message: <directory path> and <String> Required"
  exit 1
fi

filesdir=$1  
searchstr=$2 

if [ ! -d "$filesdir" ]; then
   echo "Error Message: '$filesdir' is not available in directory"
   exit 1
fi


#This Code Block was based on content at [ https://www.geeksforgeeks.org/how-to-count-files-in-directory-recursively-in-linux/ ] #with modifications .

file_count=$(find "$filesdir" -type f | wc -l)

#This Code Block was based on content at [ https://www.geeksforgeeks.org/how-to-recursively-grep-all-directories-and-#subdirectories-in-linux/ ] with modifications[2>/dev/null is used to hide error Message and wc -l is used for word Count] .

match_count=$(grep -r "$searchstr" "$filesdir" 2>/dev/null | wc -l)


echo "The number of files are $file_count and the number of matching lines are $match_count"

exit 0
