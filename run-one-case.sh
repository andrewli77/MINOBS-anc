scores=$1
file=$2

cmd="timeout 12h ./search $scores $file 10 -1 out.txt"
echo "$cmd"
eval "$cmd"