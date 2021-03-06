REPORTER=dot
BINARY=./lib/leveldb.node

# prefer installed scripts
PATH:=./node_modules/.bin:${PATH}

build:
	if [ ! -d ./build ]; then node-waf configure; fi
	node-waf build

coffee:
	coffee --bare --compile --output lib src/coffee

clean:
	node-waf clean

distclean: clean
	rm -rf lib node_modules

test: coffee
	rm -rf tmp
	mkdir -p tmp
	@mocha --reporter $(REPORTER) test/*-test.coffee

.PHONY: build coffee clean distclean test
