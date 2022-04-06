# GDFilestore

## flags

* @Follow:{x} {detial}		mean there is something need follow up with change of x
* @follow definition of {xx}
* @Todo: {x}  todo
* @Contract{x} {detail}		contract for code,use,etc
* @new						mean something new
* @dataflow {data}			for trace dataflow , eg: dataflow journal create

## bugs

1. serialize only design for non-const class,mean when you call Write(buf,const char *) will cause comile error


## tips 
1. search a object in a filestore,maybe need ignore the generation for the fuzzy search