FROM alpine:3.12.0 as builder

WORKDIR /root

RUN apk update \
	&& apk upgrade \
	&& apk add --no-cache bash boost-dev boost-static build-base cmake git clang 

RUN ln -sf /usr/bin/clang /usr/bin/cc \
	&& ln -sf /usr/bin/clang++ /usr/bin/c++ \
	&& ls -l /usr/bin/cc /usr/bin/c++ /usr/bin/clang /usr/bin/clang++

COPY . .

RUN git submodule update --init --recursive \
	&& bash build.sh

FROM alpine:3.12.0

RUN apk update \
	&& apk upgrade \
	&& apk add --no-cache bash \
	bash-doc \
	bash-completion \
	ca-certificates \
	clang \
	&& rm -rf /var/cache/apk/* \
	&& /bin/bash

WORKDIR /root

COPY --from=builder /root/build/solppc/solppc .
ENTRYPOINT ["./solppc"] 
