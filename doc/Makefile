sources := $(shell xargs -a order.txt echo)

informat  := markdown+raw_tex
headerin  := src/Header.md
headerout := build/header.html

build/doc.html: $(sources) $(headerin) filter/* order.txt style.css Makefile
	mkdir -p ./build
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
		-B $(headerout) \
		--mathjax=https://cdnjs.cloudflare.com/ajax/libs/mathjax/3.1.2/es5/tex-chtml.min.js \
		--standalone \
		--lua-filter ./filter/diagram-generator.lua \
		--toc \
		--section-divs \
		--number-sections \
		--css=style.css \
		--extract-media=./media\
		-o $@ \
		$(sources)
	rm -rf ./build/media
	mv ./media ./build/media
	rm $(headerout)