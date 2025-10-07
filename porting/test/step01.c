/*
  Porting to architecture `foo`

  * Check if a compiler is build for the native host without errors

        ./make.sh foo compile_native

  * Check if the compiler runs without errors on a valid C input file

        cat porting/test/step01.c | build/punycc_foo.native > exec.foo

  * Check if the output (`exec.or1k`) is always an empty file.

*/

int main()
{
}
