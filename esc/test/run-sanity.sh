function run {
echo compiling $1
cd ../build
#../build/esc1-r.sh ../test/sanity/tests/$1
cp ../test/sanity/base/$1.ast ../test/sanity/tests
../build/esc2-t.sh ../test/sanity/tests/$1
../build/esc3-t.sh ../test/sanity/tests/$1
../bin/shell -log ../test/sanity/tests/$1.abc
cd ../test
}


rm sanity/tests/*.es.ast
rm sanity/tests/*.es.asm
rm sanity/tests/*.es.abc
rm sanity/tests/*.es.log
rm sanity/tests/*.es.abc

rm sanity/log/*.es.ast
rm sanity/log/*.es.asm
rm sanity/log/*.es.abc
rm sanity/log/*.es.log

run arrayliteral.es
run cls.es
run continue.es
run expr.es
run fib.es
run for.es
run hello.es
run if.es
run numbers.es
run objectliteral.es
run prime.es
run print.es
run switch.es
run switchType.es
run try.es
run vars.es
run while.es
run logic.es
run destruct.es

cp sanity/tests/*.es.ast sanity/log
cp sanity/tests/*.es.asm sanity/log
cp sanity/tests/*.es.log sanity/log

diff -s sanity/base sanity/log
