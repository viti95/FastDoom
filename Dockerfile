# Dockerfile multi-arch para Open Watcom v2
FROM --platform=$BUILDPLATFORM debian:bookworm AS builder

ARG TARGETARCH
RUN apt-get update && apt-get install -y \
    git clang make g++ cmake dosbox \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build
RUN git clone https://github.com/open-watcom/open-watcom-v2.git

WORKDIR /build/open-watcom-v2
# Configuración para diferentes arquitecturas
RUN if [ "$TARGETARCH" = "amd64" ]; then \
    echo "OWTOOLS=GCC" >> setvars.sh; \
    else \
    echo "OWTOOLS=CLANG" >> setvars.sh; \
    fi
RUN echo "export OWDOSBOX=dosbox" >> setvars.sh

RUN ./build.sh
RUN ./build.sh rel os_dos

# Etapa final
FROM debian:bookworm-slim
COPY --from=builder /build/open-watcom-v2/rel /opt/watcom

ENV WATCOM=/opt/watcom \
    PATH="$PATH:/opt/watcom/arml64:/opt/watcom/binl:/opt/watcom/binl64" \
    EDDAT=/opt/watcom/eddat \
    INCLUDE=/opt/watcom/h

RUN apt-get update && apt-get install -y nasm flatpak 
RUN flatpak remote-add --if-not-exists flathub https://dl.flathub.org/repo/flathub.flatpakrepo

RUN flatpak install flathub com.dosbox_x.DOSBox-X -y
# Verificación de instalación
RUN apt-get install -y 7zip
RUN find . -name wcl386
RUN wcl386 -h > /dev/null && echo "Compilador verificado"
RUN rm -rf /var/lib/apt/lists/*
