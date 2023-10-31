#!/usr/bin/env bash

# Exit immediately if any command below fails.
set -e

make


echo "Generating a test_files directory.."
mkdir -p test_files
rm -f test_files/*


echo "Generating test files.."

## EMPTY
printf "" > test_files/empty.input
## LOCKED FILES
printf "Hemmelighed" > test_files/hemmelig_fil.input
chmod -r test_files/hemmelig_fil.input
### ASCII
printf "Hello, World!\n" > test_files/ascii_01.input
printf "Hello, World!" > test_files/ascii_02.input
printf "greriojgr 4444" > test_files/ascii_03.input
printf "Wild grey goose 2210+" > test_files/ascii_04.input
printf "Lorem ipsum dolor sit amet, consectetur adipiscing elit.\n" > test_files/ascii_05.input
printf "Nulla cursus libero eget tortor interdum, quis fermentum orci volutpat!!@" > test_files/ascii_06.input
printf "_____________" > test_files/ascii_07.input
printf "_-----___" > test_files/ascii_08.input
printf "2342342uhkjsdddj" > test_files/ascii_09.input
printf "Hello^World!" > test_files/ascii_10.input
printf "Hello~World!" > test_files/ascii_11.input
printf "Hello, |World!\n" > test_files/ascii_12.input
## ISO
printf "Hello, \xae World!\n" > test_files/iso-8859-1_01.input # &#174; 	AE 	®
printf "Hello, \xd0 World!\n" > test_files/iso-8859-1_02.input # &#208; 	D0 	Ð
printf "Hello, \xd6 World!\n" > test_files/iso-8859-1_03.input # &#214; 	D6 	Ö
printf "Hello, \xe4 World!\n" > test_files/iso-8859-1_04.input # &#228; 	E4 	ä
printf "Hello, \xef World!\n" > test_files/iso-8859-1_05.input # &#239; 	EF 	ï
printf "Hello, \xf8 World!\n" > test_files/iso-8859-1_06.input # &#248; 	F8 	ø
printf "Hello, \xff World!\n" > test_files/iso-8859-1_07.input # &#255; 	FF 	ÿ 	
printf "Hello, \xba World!\n" > test_files/iso-8859-1_08.input # &#186; 	BA 	º
printf "Hello, \xab World!\n" > test_files/iso-8859-1_09.input # &#171; 	AB 	«
printf "Hello, \xb1 World!\n" > test_files/iso-8859-1_10.input # &#177; 	B1 	±
printf "Hello, \xbc World!\n" > test_files/iso-8859-1_11.input # &#188; 	BC 	¼
printf "Hello, \xb6 World!\n" > test_files/iso-8859-1_12.input # &#182; 	B6 	¶
## UTF8
printf "Hello, \xE3\x81\xAE World!\n" > test_files/utf-8_01.input # UTF-8 file with の symbol (U+306E / decimal 227 129 174 / e3 81 ae)
printf "Hello, \xe1\xa8\xa5 World!\n" > test_files/utf-8_02.input # U+1A25	ᨥ	0xe1 0xa8 0xa5
printf "Hello, \xe1\xaa\x84 World!\n" > test_files/utf-8_03.input # U+1A84	᪄	0xe1 0xaa 0x84
printf "Hello, \xd8\xb6 World!\n" > test_files/utf-8_04.input # U+0854	ࡔ	0xe0 0xa1 0x94
printf "Hello, \xf0\x96\xbc\x88 World!\n" > test_files/utf-8_05.input # U+16F08	𖼈	0xf0 0x96 0xbc 0x88
printf "Hello, \xf0\x96\xa0\xb0 World!\n" > test_files/utf-8_06.input #U+16830	𖠰	0xf0 0x96 0xa0 0xb0
printf "Hello, \xc9\x90 World!\n" > test_files/utf-8_07.input # U+0250	ɐ	0xc9 0x90
printf "Hello, \xca\xa6 World!\n" > test_files/utf-8_08.input # U+02A6	ʦ	0xca 0xa6
printf "Hello, \xe0\xa7\xaf World!\n" > test_files/utf-8_09.input # U+09EF	৯	0xe0 0xa7 0xaf
printf "Hello, \xe1\x84\x9d World!\n" > test_files/utf-8_10.input # U+111D	ᄝ	0xe1 0x84 0x9d
printf "Hello, \xcd\xbc World!\n" > test_files/utf-8_11.input # U+037C	ͼ	0xcd 0xbc
printf "Hello, \xd4\xb6 World!\n" > test_files/utf-8_12.input # U+0536	Զ	0xd4 0xb6
## DATA
printf "Hello, \x00 World!\n" > test_files/data_01.input
printf "Hello, \x01 World!\n" > test_files/data_02.input
printf "Hello, \x02 World!\n" > test_files/data_03.input
printf "Hello, \x03 World!\n" > test_files/data_04.input
printf "Hello, \x04 World!\n" > test_files/data_05.input
printf "Hello, \x05 World!\n" > test_files/data_06.input
printf "Hello, \x06 World!\n" > test_files/data_07.input
printf "Hello, \x00F34623 World!\n" > test_files/data_08.input
printf "Hello, \xFF228A World!\n" > test_files/data_09.input
printf "Hello, \xAE44E World!\n" > test_files/data_10.input
printf "Hello, \x00AAAA World!\n" > test_files/data_11.input
printf "Hello, \x00FFFF World!\n" > test_files/data_12.input

echo "Running the tests.."
exitcode=0
for f in test_files/*.input
do
  echo ">>> Testing ${f}.."
  file    ${f} | sed -e 's/ASCII text.*/ASCII text/' \
                         -e 's/UTF-8 Unicode text.*/UTF-8 Unicode text/' \
                         -e 's/ISO-8859 text.*/ISO-8859 text/' \
                         -e 's/writable, regular file, no read permission/cannot determine (Permission denied)/' \
                         > "${f}.expected"
  ./file  "${f}" > "${f}.actual"

  if ! diff -u "${f}.expected" "${f}.actual"
  then
    echo ">>> Failed :-("
    exitcode=1
  else
    echo ">>> Success :-)"
  fi
done
exit $exitcode
