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

    arch=$(uname -m)

    if [ "$arch" == "aarch64" ]; then
        export PATH=$WATCOM/arml64:$PATH
    else
        export PATH=$WATCOM/binl:$PATH
    fi

    export PATH=$WATCOM/arml64:$PATH
    export EDPATH=$WATCOM/eddat
    export WIPFC=$WATCOM/wipfc
    export INCLUDE=$WATCOM/h

    echo -e "Using Watcom on $final_dir"
fi
