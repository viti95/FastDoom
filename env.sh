final_dir=""

for dir in \
        "/usr/bin/watcom" \
        "/opt/watcom"
do
    if [ -d "$dir" ]; then
        final_dir=$dir
        break
    fi
done

if [[ -z "$final_dir" ]]; then
    echo "Watcom install not found."
else
    export WATCOM=$final_dir
    export PATH=$WATCOM/binl:$PATH
    export EDPATH=$WATCOM/eddat
    export WIPFC=$WATCOM/wipfc
    export INCLUDE=$WATCOM/h

    echo -e "Using Watcom on $final_dir"
fi
