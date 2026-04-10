sources := $(shell tr '\n' ' ' < order.txt)

informat  := markdown+raw_tex+grid_tables
headerin  := src/Header.md
headerout := build/header.html
docker_image ?= wassign-docs
repo_root := $(abspath ..)

build/doc.html: $(sources) $(headerin) filter/* headerbar.html order.txt style.css Makefile
	mkdir -p ./build ./media
	cp ./style.css ./build/style.css
	pandoc \
		--from=$(informat) \
		--to=html \
		-o $(headerout) \
		$(headerin)
	pandoc \
		--from=$(informat) \
		--to=html \
		-V lang=en \
		-B headerbar.html \
		-B $(headerout) \
		--mathjax=https://cdnjs.cloudflare.com/ajax/libs/mathjax/3.1.2/es5/tex-chtml.min.js \
		--standalone \
		--lua-filter ./filter/diagram-generator.lua \
		--toc \
		--toc-depth=3 \
		--section-divs \
		--number-sections \
		--css=style.css \
		--extract-media=./media \
		-o $@ \
		$(sources)
	rm -rf ./build/media
	mv ./media ./build/media
	rm $(headerout)

.PHONY: docker-image docker-build

docker-image:
	docker build -t $(docker_image) .

docker-build: docker-image
	docker run --rm \
		-u "$$(id -u):$$(id -g)" \
		-v "$(repo_root):/workspace" \
		-w /workspace/doc \
		$(docker_image)
