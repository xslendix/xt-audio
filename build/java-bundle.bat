@echo off
cd ..\dist\java\xt\release\target
gpg -ab xt.audio-2.3.pom
gpg -ab xt.audio-2.3.jar
gpg -ab xt.audio-2.3-sources.jar
gpg -ab xt.audio-2.3-javadoc.jar
jar -cvf xt.audio-2.3-bundle.jar xt.audio-2.3.pom xt.audio-2.3.jar xt.audio-2.3-sources.jar xt.audio-2.3-javadoc.jar xt.audio-2.3.pom.asc xt.audio-2.3.jar.asc xt.audio-2.3-sources.jar.asc xt.audio-2.3-javadoc.jar.asc
cd ..\..\..\..\..\build