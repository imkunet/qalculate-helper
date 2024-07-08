FROM debian:bookworm AS builder

RUN apt-get update && apt-get install -y \
    build-essential \
    mold \
    libcurl4-openssl-dev \
    libseccomp-dev \
    libcap-ng-dev \
    libqalculate-dev \
    cmake \
    pkg-config

WORKDIR /qalculate-helper

COPY include include
COPY src src
COPY CMakeLists.txt .

WORKDIR /qalculate-helper/build
RUN cmake -DSETUID=True -DSECCOMP=False -DSECCOMP_ALLOW_CLONE=True -DCMAKE_EXE_LINKER_FLAGS="-fuse-ld=mold" -DCMAKE_SHARED_LINKER_FLAGS="-fuse-ld=mold" ..
RUN make
RUN chmod +x qalculate-helper

FROM debian:bookworm

RUN apt-get update && apt-get install -y libcurl4 \
    libmpfr6 \
    libgmp10 \
    libxml2 \
    libqalculate22

COPY --from=builder /qalculate-helper/build/qalculate-helper /qalculate-helper
RUN /qalculate-helper update

ENTRYPOINT [ "/qalculate-helper" ]
