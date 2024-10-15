pushd ../
ninja -C out/debug_x64 pdfium_jni
popd
javac com/kinggrid/krc/KGDocument.java
