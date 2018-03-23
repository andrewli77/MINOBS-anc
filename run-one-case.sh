scores=$1
file=$2

cmd="./search $scores $file 10 -1 out.txt"
echo "$cmd"
eval "$cmd"