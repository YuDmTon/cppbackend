FROM gcc:11.3

RUN apt update && \
    apt install -y \
      python3-pip \
      cmake \
    && \
    pip3 install conan==1.*

COPY ./src /app/src
COPY ./static /app/static
COPY ./data /app/data
COPY CMakeLists.txt conanfile.txt /app/

RUN mkdir /app/build && cd /app/build && conan install .. -s compiler.libcxx=libstdc++11 && cmake -DCMAKE_BUILD_TYPE=Debug .. && cmake --build .
