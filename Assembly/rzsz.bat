rem assemble IDAFW sources
tasm -t80 -b -f00 rz_ida.asm rz_ida.com
tasm -t80 -b -f00 sz_ida.asm sz_ida.com
rem
rem assemble YAZD sources
rem tasm -t80 -b -f00 rz_yazd.asm rz_yazd.com
rem tasm -t80 -b -f00 sz_yazd.asm sz_yazd.com

rem compare rz.com to RZ_IDA.COM goes here
fc rz.com RZ_IDA.COM
rem compare sz.com to SZ_IDA.COM goes here
fc sz.com SZ_IDA.COM
