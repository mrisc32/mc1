# HDL source code

## RTL

The [rtl](./rtl) folder contains the RTL of the system.


## Test benches

The tests are located in the [test](./test) folder and use
[VUnit](https://vunit.github.io/):

```bash
$ pip3 install --user vunit_hdl
$ ./run.py
```

In order to run the tests, you need a VHDL simulator. A good, open source
VHDL simulator is [GHDL](http://ghdl.free.fr/).
