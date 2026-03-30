FROM gcr.io/distroless/nodejs22-debian12:nonroot



COPY --chown=nonroot:nonroot ./server /home/nonroot/server

ENTRYPOINT [ "/home/nonroot/server/Crunch/Binaries/Linux/CrunchServer", "Crunch" ]
