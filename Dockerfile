FROM eclipse-temurin:17

WORKDIR /app

# Install gcc and make
RUN apt-get update && apt-get install -y gcc make && rm -rf /var/lib/apt/lists/*

# Copy source and build the JVM binary
COPY . .
RUN make clean && make

ENTRYPOINT ["./jvm"]
