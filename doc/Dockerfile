FROM pandoc/latex:3.9.0.2-ubuntu

# Keep the image small and avoid installing a full desktop stack.
# pandoc/latex already provides pandoc plus a minimal TeX Live install.
RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        ghostscript \
        inkscape \
        make \
    && rm -rf /var/lib/apt/lists/*

RUN tlmgr install standalone adjustbox

WORKDIR /workspace/doc

ENTRYPOINT ["make"]
