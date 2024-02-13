FROM ubuntu:23.10 as builder
RUN apt-get update && apt-get install -y gcc build-essential

COPY ./src /app/src
COPY ./makefile /app/makefile

WORKDIR /app

RUN make release

FROM ubuntu:23.10 as final

COPY --from=builder /app/rinha-backend-2024 /app/rinha-backend-2024

WORKDIR /app
