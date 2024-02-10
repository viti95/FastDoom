for dir in \
        "/usr/bin/watcom" \
        "/opt/watcom"
do
    if [ -d "$dir" ]; then
        export WATCOM=$dir
        break
    fi
done

export PATH=$WATCOM/binl:$PATH
export EDPATH=$WATCOM/eddat
export WIPFC=$WATCOM/wipfc
export INCLUDE=$WATCOM/h

echo -e "Watcom folder: $dir"
