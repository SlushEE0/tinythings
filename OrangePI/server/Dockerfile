FROM oven/bun:1.0.26

WORKDIR /src

COPY package*.json ./

RUN bun install

COPY . . 

ENV PORT=8080

EXPOSE 8080

CMD ["bun", "run", "./src/index.ts"]