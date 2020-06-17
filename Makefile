include android-sqlcipher-glue-jar-build.mk

SQLITE_BATCH_CONNECTION_CORE_REMOTE := \
	https://github.com/brodybits/sqlite-fake-crypto-batch-connection-core-pro-free-2020-01

SQLITE_BATCH_CONNECTION_CORE_ROOT := sqlite-fake-crypto-batch-connection-core-pro-free

SQLITE_BATCH_CONNECTION_CORE_COMMIT_ID := \
	f0dd205a61d3a80c7716ca86ee1e10c74605463c

all:
	echo 'all not supported' && exit 1

build: clean fetch-dependencies update-dependencies build-dist-dependencies

prepare-demo: build prepare-demo-plugins

fetch-dependencies:
	git clone $(SQLITE_BATCH_CONNECTION_CORE_REMOTE) $(SQLITE_BATCH_CONNECTION_CORE_ROOT)
	git clone $(ANDROID_SQLCIPHER_JAR_BUILD_REMOTE) $(ANDROID_SQLCIPHER_JAR_BUILD_ROOT)

update-dependencies:
	(cd $(SQLITE_BATCH_CONNECTION_CORE_ROOT) && git checkout $(SQLITE_BATCH_CONNECTION_CORE_COMMIT_ID))
	(cd $(ANDROID_SQLCIPHER_JAR_BUILD_ROOT) && git checkout $(ANDROID_SQLCIPHER_JAR_BUILD_COMMIT))

build-dist-dependencies:
	(cd $(ANDROID_SQLCIPHER_JAR_BUILD_ROOT) && make jar)
	mkdir dist-dependencies
	cp $(ANDROID_SQLCIPHER_JAR_BUILD_ROOT)/*.jar dist-dependencies
	cp $(SQLITE_BATCH_CONNECTION_CORE_ROOT)/android/src/main/java/io/sqlc/SQLiteBatchCore.java dist-dependencies
	cp $(SQLITE_BATCH_CONNECTION_CORE_ROOT)/sqlite-connection-core.[hc] dist-dependencies
	cp $(SQLITE_BATCH_CONNECTION_CORE_ROOT)/ios/SQLiteBatchCore.[hm] dist-dependencies
	cp $(ANDROID_SQLCIPHER_JAR_BUILD_ROOT)/android-database-sqlcipher/src/main/external/sqlcipher/sqlite3.* dist-dependencies

prepare-demo-plugins:
	mkdir -p demo/local-plugin
	cp -r dist-dependencies package.json plugin.xml src www demo/local-plugin
	(cd demo && cordova plugin add ./local-plugin cordova-plugin-file cordova-sqlite-storage-file && cordova plugin ls)
	echo 'use Cordova to add desired platform to the demo before running'

clean: clean-demo
	rm -rf sqlite-amalgamation-* sqlite-batch-connection-* dist-dependencies
	rm -rf android-database-*
	rm -rf sqlite-fake-*

clean-demo:
	rm -rf demo/node_modules demo/package-lock.json demo/local-plugin demo/plugins demo/platforms
