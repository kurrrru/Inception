FROM alpine:3.23 AS builder

ARG EMSDK_VERSION=6.0.3

RUN apk add --no-cache python3 git gcompat nodejs

RUN git clone --depth 1 https://github.com/emscripten-core/emsdk.git && \
    ./emsdk/emsdk install ${EMSDK_VERSION} && \
    ./emsdk/emsdk activate ${EMSDK_VERSION} && \
    sed -i 's|NODE_JS = .*|NODE_JS = "/usr/bin/node"|' /emsdk/.emscripten

COPY srcs/cpp/ /srcs/cpp/
COPY srcs/exported_functions.txt /srcs/exported_functions.txt
WORKDIR /srcs
RUN cd /emsdk && . ./emsdk_env.sh && \
    cd /srcs && \
    mkdir -p out && \
    em++ cpp/Board.cpp cpp/playerfunc/ai.cpp cpp/web_api.cpp \
        -O3 -std=c++17 -I. \
        -s WASM=1 \
        -s MODULARIZE=1 \
        -s EXPORT_NAME=createReversiModule \
        -s ALLOW_MEMORY_GROWTH=1 \
        -s EXPORTED_RUNTIME_METHODS='["cwrap","ccall"]' \
        -s EXPORTED_FUNCTIONS=@/srcs/exported_functions.txt \
        -o out/reversi.js

FROM alpine:3.23
RUN apk add --no-cache python3
COPY srcs/html/ /srcs/html/
COPY --from=builder /srcs/out/ /srcs/html/
WORKDIR /srcs/html
CMD ["python3", "-m", "http.server", "8000"]
