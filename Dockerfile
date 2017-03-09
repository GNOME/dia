FROM ubuntu:latest

RUN apt-get update
RUN apt-get -y install libpango1.0-dev autotools-dev libfreetype6-dev\
    automake autoconf libtool intltool libtool-bin\
    libgtk2.0-dev libxml2-dev

CMD /bin/bash



