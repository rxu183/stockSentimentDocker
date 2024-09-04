# Use the official Ubuntu image as a base
FROM ubuntu:22.04

# Set the environment variable to prevent prompts
ENV DEBIAN_FRONTEND=noninteractive

# Set the environment variables for vcpkg and cmake
ENV VCPKG_ROOT=/workspace/vcpkg
ENV VCPKG_FORCE_SYSTEM_BINARIES=1
ENV CMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake
ENV CMAKE_PREFIX_PATH=$VCPKG_ROOT/installed/arm64-linux

# Install necessary packages
RUN apt-get update && \
    apt-get install -y \
    build-essential \
    cmake \
    g++ \
    gcc \
    libpq-dev \
    libpqxx-dev \
    git \
    vim \
    libcurl4-openssl-dev \
    zip \
    unzip \
    libhdf5-dev \
    zlib1g-dev \
    tar \
    curl \
    ninja-build \
    python3 \
    python3-pip \
    python3-distutils \
    python3-setuptools \  
    gfortran \ 
    libblas-dev \ 
    liblapack-dev \
    iputils-ping \
    libssl-dev && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# Install Python libraries with pip
RUN pip3 install --no-cache-dir \
    numpy \
    pandas \
    pymc \
    matplotlib \
    scikit-learn \
    psycopg2 \
    python-dotenv \
    theano-pymc \
    boto3

# Clone vcpkg and bootstrap it
RUN git clone https://github.com/microsoft/vcpkg.git $VCPKG_ROOT && \
    $VCPKG_ROOT/bootstrap-vcpkg.sh

# Install necessary libraries using vcpkg for arm64-linux
RUN $VCPKG_ROOT/vcpkg install nlohmann-json:arm64-linux

# RUN $VCPKG_ROOT/vcpkg install aws-sdk-cpp[core,secretsmanager]:arm64-linux

# Clone the repository into a temporary directory and move contents to /workspace
# RUN git clone https://github.com/rxu183/stockSentimentDocker.git /workspace/temp && \
#     cp -r /workspace/temp/* /workspace && \
#     rm -rf /workspace/temp

# Set the working directory
WORKDIR /workspace


# Set the default command to bash, so we can use the terminal
CMD ["/bin/bash"]