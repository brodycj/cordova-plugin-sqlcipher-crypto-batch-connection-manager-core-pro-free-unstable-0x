all:
	echo 'all not supported' && exit 1

prepare-demo: clean-demo
	mkdir -p demo/plugin
	cp -r package.json plugin.xml src www demo/plugin
	(cd demo && cordova plugin add ./plugin && cordova plugin ls)
	echo 'use Cordova to add desired platform to the demo before running'

clean-demo:
	rm -rf demo/node_modules demo/package-lock.json demo/plugin demo/plugins demo/platforms
