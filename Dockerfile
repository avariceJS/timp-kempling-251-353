FROM alpine:3.14

LABEL description="QT TCP Server Container"

RUN apk add --no-cache qt5-qtbase-dev g++ make

WORKDIR /home/server
COPY server/ .

RUN QMAKE="$(command -v qmake-qt5 || command -v qmake || echo /usr/lib/qt5/bin/qmake)" && \
    "$QMAKE" qt_tcp_server.pro && \
    make

EXPOSE 5555

CMD ["./qt_tcp_server"]
