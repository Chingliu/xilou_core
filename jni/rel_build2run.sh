pushd ../
ninja -C out/rel_x64 pdfium_jni
popd
javac com/kinggrid/krc/KGDocument.java
cp ../out/rel_x64/pdfium_jni.dll /e/
java com.kinggrid.krc.KGDocument
