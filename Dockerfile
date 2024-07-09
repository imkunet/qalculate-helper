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
RUN cmake -DUID=1000 -DSECCOMP=True -DSECCOMP_ALLOW_CLONE=True -DCMAKE_EXE_LINKER_FLAGS="-fuse-ld=mold" -DCMAKE_SHARED_LINKER_FLAGS="-fuse-ld=mold" ..
RUN make
RUN chmod +x qalculate-helper

FROM debian:bookworm

RUN apt-get update && apt-get install -y libcurl4 \
    libmpfr6 \
    libgmp10 \
    libxml2 \
    libcap2-bin \
    libqalculate22

RUN addgroup \
    --gid 1000 \
    qalculate
RUN adduser \
    --disabled-password \
    --gecos "" \
    --home "/nonexistent" \
    --shell "/sbin/nologin" \
    --no-create-home \
    --uid 1000 \
    --gid 1000 \
    qalculate
RUN pwck -q
RUN grpck

COPY --from=builder --chown=1000:1000 /qalculate-helper/build/qalculate-helper /qalculate-helper
RUN chmod 6100 /qalculate-helper
RUN echo "cap_setgid+ep cap_setpcap+ep" | setcap - qalculate-helper
RUN apt-get clean
RUN /qalculate-helper update

ENTRYPOINT [ "/qalculate-helper" ]
